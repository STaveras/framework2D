// File: RendererMTL.mm
// Metal backend for macOS and iOS (see RendererMTL.h). Each frame is built on
// the CPU first, mirroring RendererGL: sprites and font pixels become
// world-space quads, batched while the texture and render-list transform stay
// the same. The batches are then encoded into a single render pass.
#ifdef __APPLE__
#if !__has_feature(objc_arc)
#error "RendererMTL.mm must be compiled with -fobjc-arc"
#endif

#include "RendererMTL.h"

#include "Camera.h"
#include "CollisionDebugDraw.h"
#include "CollisionSystem.h"
#include "Debug.h"
#include "Engine2D.h"
#include "Font.h"
#include "FramePacer.h"
#include "Game.h"
#include "GameState.h"
#include "Renderer.h"
#include "Sprite.h"
#include "TextureMTL.h"
#include "Window.h"

#include "stb/stb_image_write.h"

#import <QuartzCore/CAMetalLayer.h>
#if FRAMEWORK_IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int kMaxFramesInFlight = 3;
constexpr NSUInteger kMinVertexBufferBytes = 256 * 1024;

// Matches Vertex in kShaderSource.
struct Vertex
{
	float x, y, u, v;
	uint8_t r, g, b, a;
};
static_assert(sizeof(Vertex) == 20, "Vertex must match the shader's packed layout");

// Clip-space transform, column-major like Metal's float4x4.
struct Transform
{
	float m[16];
};

const char* const kShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct Vertex {
	packed_float2 position;
	packed_float2 uv;
	packed_uchar4 color;
};

struct Fragment {
	float4 position [[position]];
	float2 uv;
	float4 color;
};

vertex Fragment spriteVertex(uint vertexID [[vertex_id]],
                             const device Vertex* vertices [[buffer(0)]],
                             constant float4x4& transform [[buffer(1)]])
{
	const Vertex v = vertices[vertexID];
	Fragment out;
	out.position = transform * float4(float2(v.position), 0.0, 1.0);
	out.uv = float2(v.uv);
	out.color = float4(uchar4(v.color)) / 255.0;
	return out;
}

fragment float4 spriteFragment(Fragment in [[stage_in]], texture2d<float> image [[texture(0)]])
{
	constexpr sampler nearest(coord::normalized, filter::nearest, address::clamp_to_edge);
	return image.sample(nearest, in.uv) * in.color;
}
)";

// Maps screen = A * p + t, with the logical screen (0,0)-(width,height) and
// y pointing down, to clip space.
Transform makeTransform(float width, float height, float a00, float a01, float a10, float a11, float tx, float ty)
{
	const float sx = 2.0f / width;
	const float sy = -2.0f / height;
	return Transform{ {
		sx * a00, sy * a10, 0.0f, 0.0f,
		sx * a01, sy * a11, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		sx * tx - 1.0f, sy * ty + 1.0f, 0.0f, 1.0f } };
}

Transform screenTransform(float width, float height)
{
	return makeTransform(width, height, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

// Same mapping as IRenderer::_worldToScreen and RendererGL's camera transform.
Transform cameraTransform(Camera* camera, const vector2& cameraPosition, float width, float height)
{
	const float zoom = (camera->getZoom() > 0.0f) ? camera->getZoom() : 1.0f;
	const float c = std::cos(camera->getRotation());
	const float s = std::sin(camera->getRotation());
	const float a00 = zoom * c, a01 = -zoom * s, a10 = zoom * s, a11 = zoom * c;
	const vector2 center = camera->getCenter();

	// TargetCenter: screen = center + A (world - camera)
	// legacy:       screen = A (world - (camera - center))
	const bool targetCenter = camera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter;
	vector2 origin = cameraPosition;
	if (!targetCenter) {
		origin = cameraPosition - center;
	}
	float tx = -(a00 * origin.x + a01 * origin.y);
	float ty = -(a10 * origin.x + a11 * origin.y);
	if (targetCenter) {
		tx += center.x;
		ty += center.y;
	}
	return makeTransform(width, height, a00, a01, a10, a11, tx, ty);
}

const CollisionSystem* getActiveCollisionSystem()
{
	Game* game = Engine2D::getGame();
	if (!game || game->empty()) {
		return nullptr;
	}

	GameState* gameState = dynamic_cast<GameState*>(game->top());
	return gameState ? gameState->getCollisionSystem() : nullptr;
}

bool writePNG(const unsigned char* bgra, NSUInteger width, NSUInteger height, const std::string& path)
{
	std::vector<unsigned char> rgba((size_t)width * height * 4);
	for (size_t i = 0; i < rgba.size(); i += 4) {
		rgba[i + 0] = bgra[i + 2];
		rgba[i + 1] = bgra[i + 1];
		rgba[i + 2] = bgra[i + 0];
		rgba[i + 3] = 255; // the layer is opaque
	}
	stbi_flip_vertically_on_write(0);
	return stbi_write_png(path.c_str(), (int)width, (int)height, 4, rgba.data(), (int)width * 4) != 0;
}
}

struct RendererMTL::Impl
{
	struct Draw
	{
		id<MTLTexture> texture;
		MTLPrimitiveType primitive;
		bool opaque;       // drawn without blending
		size_t transform;  // index into transforms
		size_t start;
		size_t count;
	};

	id<MTLDevice> device = nil;
	id<MTLCommandQueue> queue = nil;
	id<MTLRenderPipelineState> blended = nil;
	id<MTLRenderPipelineState> opaque = nil;
	CAMetalLayer* layer = nil;
#if FRAMEWORK_IOS
	UIView* view = nil;
#else
	NSView* view = nil;
#endif
	dispatch_semaphore_t framesInFlight = nil;
	id<MTLBuffer> vertexBuffers[kMaxFramesInFlight] = {};
	int frameIndex = 0;
	std::unique_ptr<TextureMTL> white; // 1x1, for fonts, lines and the background

	// The frame being built
	std::vector<Vertex> vertices;
	std::vector<Draw> draws;
	std::vector<Transform> transforms;
	vector2 viewMin, viewMax; // world-space cull rectangle of the current list

	void setTransform(const Transform& transform)
	{
		if (transforms.empty() || std::memcmp(&transforms.back(), &transform, sizeof(Transform)) != 0) {
			transforms.push_back(transform);
		}
	}

	void add(id<MTLTexture> texture, MTLPrimitiveType primitive, const Vertex* first, size_t count, bool isOpaque = false)
	{
		const size_t transform = transforms.empty() ? 0 : transforms.size() - 1;
		if (draws.empty() || draws.back().texture != texture || draws.back().primitive != primitive ||
			draws.back().opaque != isOpaque || draws.back().transform != transform) {
			draws.push_back(Draw{ texture, primitive, isOpaque, transform, vertices.size(), 0 });
		}
		vertices.insert(vertices.end(), first, first + count);
		draws.back().count += count;
	}

	// Two triangles from corners in order top-left, top-right, bottom-right, bottom-left.
	void addQuad(id<MTLTexture> texture, const Vertex corners[4], bool isOpaque = false)
	{
		const Vertex triangles[6] = { corners[0], corners[1], corners[2], corners[0], corners[2], corners[3] };
		add(texture, MTLPrimitiveTypeTriangle, triangles, 6, isOpaque);
	}

	void addRect(float left, float top, float right, float bottom, Color color, bool isOpaque = false)
	{
		const Vertex corners[4] = {
			{ left, top, 0.5f, 0.5f, color.r, color.g, color.b, color.a },
			{ right, top, 0.5f, 0.5f, color.r, color.g, color.b, color.a },
			{ right, bottom, 0.5f, 0.5f, color.r, color.g, color.b, color.a },
			{ left, bottom, 0.5f, 0.5f, color.r, color.g, color.b, color.a },
		};
		addQuad(white->getTexture(), corners, isOpaque);
	}

	void addLine(const vector2& start, const vector2& end, float r, float g, float b, float a)
	{
		const auto channel = [](float value) { return (uint8_t)std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f); };
		const uint8_t cr = channel(r), cg = channel(g), cb = channel(b), ca = channel(a);
		const Vertex line[2] = {
			{ start.x, start.y, 0.5f, 0.5f, cr, cg, cb, ca },
			{ end.x, end.y, 0.5f, 0.5f, cr, cg, cb, ca },
		};
		add(white->getTexture(), MTLPrimitiveTypeLine, line, 2);
	}

	// Keep the drawable at the view's size in pixels.
	void updateDrawableSize(void)
	{
		if (!layer || !view) {
			return;
		}
#if FRAMEWORK_IOS
		const CGFloat scale = view.contentScaleFactor;
		const CGSize size = CGSizeMake(std::round(view.bounds.size.width * scale), std::round(view.bounds.size.height * scale));
#else
		const CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;
		const NSSize size = [view convertSizeToBacking:view.bounds.size];
#endif
		if (layer.contentsScale != scale) {
			layer.contentsScale = scale;
		}
		if (size.width >= 1.0 && size.height >= 1.0 && !CGSizeEqualToSize(layer.drawableSize, size)) {
			layer.drawableSize = size;
		}
	}
};

RendererMTL::RendererMTL(Window* window)
	: IRenderer(RENDERER_TYPE_MTL, window ? window->getWidth() : 0, window ? window->getHeight() : 0),
	  _impl(new Impl()),
	  _window(window)
{
	Impl& impl = *_impl;
	impl.device = MTLCreateSystemDefaultDevice();
	if (!impl.device) {
		throw std::runtime_error("Metal is not supported on this device");
	}
	impl.queue = [impl.device newCommandQueue];
	impl.framesInFlight = dispatch_semaphore_create(kMaxFramesInFlight);

	const unsigned char whitePixel[4] = { 255, 255, 255, 255 };
	impl.white.reset(new TextureMTL("<white>", impl.device, 1, 1, whitePixel));

	NSError* error = nil;
	id<MTLLibrary> library = [impl.device newLibraryWithSource:@(kShaderSource) options:nil error:&error];
	if (!library) {
		throw std::runtime_error(std::string("Metal shader compilation failed: ") + error.localizedDescription.UTF8String);
	}

	MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
	descriptor.label = @"Sprites";
	descriptor.vertexFunction = [library newFunctionWithName:@"spriteVertex"];
	descriptor.fragmentFunction = [library newFunctionWithName:@"spriteFragment"];
	MTLRenderPipelineColorAttachmentDescriptor* color = descriptor.colorAttachments[0];
	color.pixelFormat = MTLPixelFormatBGRA8Unorm;
	impl.opaque = [impl.device newRenderPipelineStateWithDescriptor:descriptor error:&error];

	// Same blending as RendererGL: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
	color.blendingEnabled = YES;
	color.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	color.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	impl.blended = [impl.device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	if (!impl.opaque || !impl.blended) {
		throw std::runtime_error(std::string("Metal pipeline creation failed: ") +
			(error ? error.localizedDescription.UTF8String : "unknown error"));
	}

	if (!window) {
		return;
	}

#if FRAMEWORK_IOS
	UIView* view = (__bridge UIView*)window->getNativeView();
	if (![view.layer isKindOfClass:[CAMetalLayer class]]) {
		throw std::runtime_error("The iOS game view must be backed by a CAMetalLayer");
	}
	impl.layer = (CAMetalLayer*)view.layer;
#else
	// GLFW windows created with GLFW_NO_API host the layer in their content view.
	NSWindow* nsWindow = glfwGetCocoaWindow(window->getUnderlyingWindow());
	NSView* view = nsWindow.contentView;
	if (!view) {
		throw std::runtime_error("The Metal renderer needs a GLFW window created with Window::ClientAPI::None");
	}
	impl.layer = [CAMetalLayer layer];
	[view setLayer:impl.layer];
	[view setWantsLayer:YES];
	impl.layer.displaySyncEnabled = m_bVerticalSync ? YES : NO;
#endif
	impl.view = view;
	impl.layer.device = impl.device;
	impl.layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	impl.layer.opaque = YES;
	// Frame capture (AUTO_SCREENSHOT_*) copies the drawable back.
	impl.layer.framebufferOnly = NO;
	impl.updateDrawableSize();
}

RendererMTL::~RendererMTL(void)
{
	shutdown();
}

ITexture* RendererMTL::createTexture(const char* szFilename, Color colorKey)
{
	ITexture* texture = _textureExists(szFilename, colorKey);
	if (!texture) {
		texture = new TextureMTL(szFilename, _impl->device, colorKey);
		texture->setKeyColor(colorKey);
		m_Textures.store(texture);
	}
	return texture;
}

bool RendererMTL::destroyTexture(const ITexture* texture)
{
	// Command buffers retain the textures they use, so frames still in flight are unaffected.
	return texture && IRenderer::destroyTexture(texture);
}

void RendererMTL::initialize(void)
{
	// The device, pipelines and layer live as long as the renderer: games may
	// shut it down and initialize it again after resizing the window
	// (FantasySideScroller::begin does).
	_impl->updateDrawableSize();
}

void RendererMTL::shutdown(void)
{
	// Wait for the frames in flight, then hand their slots back.
	if (!_impl->framesInFlight) {
		return;
	}
	for (int i = 0; i < kMaxFramesInFlight; ++i) {
		dispatch_semaphore_wait(_impl->framesInFlight, DISPATCH_TIME_FOREVER);
	}
	for (int i = 0; i < kMaxFramesInFlight; ++i) {
		dispatch_semaphore_signal(_impl->framesInFlight);
	}
}

void RendererMTL::setVerticalSync(bool vsyncEnabled)
{
	IRenderer::setVerticalSync(vsyncEnabled);
#if !FRAMEWORK_IOS
	// iOS always presents in sync with the display.
	if (_impl->layer) {
		_impl->layer.displaySyncEnabled = vsyncEnabled ? YES : NO;
	}
#endif
}

void RendererMTL::_beginRenderList(const RenderList& renderList)
{
	Impl& impl = *_impl;
	if (renderList.screenSpace || !m_pCamera) {
		impl.setTransform(screenTransform((float)m_nWidth, (float)m_nHeight));
		impl.viewMin = vector2(-INFINITY, -INFINITY);
		impl.viewMax = vector2(INFINITY, INFINITY);
		return;
	}

	const vector2 cameraPosition = _parallaxCameraPosition(renderList);
	impl.setTransform(cameraTransform(m_pCamera, cameraPosition, (float)m_nWidth, (float)m_nHeight));
	_viewBounds(cameraPosition, impl.viewMin, impl.viewMax);
}

void RendererMTL::_endRenderList(const RenderList& renderList)
{
}

void RendererMTL::_renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawImage(sprite, tint, offset, renderList.screenSpace);
}

void RendererMTL::_renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawFont(font, tint, offset);
}

void RendererMTL::_drawImage(Sprite* sprite, Color tint, const vector2& offset, bool screenSpace)
{
	if (!sprite) {
		return;
	}

	const TextureMTL* texture = static_cast<const TextureMTL*>(sprite->getTexture());
	if (!texture) {
		return;
	}

	const RECT& srcRect = sprite->getSrcRect();
	const float srcWidth = static_cast<float>(srcRect.right - srcRect.left);
	const float srcHeight = static_cast<float>(srcRect.bottom - srcRect.top);
	const float texWidth = static_cast<float>(texture->getWidth());
	const float texHeight = static_cast<float>(texture->getHeight());
	if (srcWidth <= 0.0f || srcHeight <= 0.0f || texWidth <= 0.0f || texHeight <= 0.0f) {
		return;
	}

	// Half-texel inset on sub-rectangles, as in RendererGL, so atlas
	// neighbours never bleed in.
	const bool isSubRect =
		srcRect.left > 0 ||
		srcRect.top > 0 ||
		srcRect.right < static_cast<int>(texWidth) ||
		srcRect.bottom < static_cast<int>(texHeight);
	float uInset = 0.0f;
	float vInset = 0.0f;
	if (isSubRect && srcWidth > 1.0f && srcHeight > 1.0f) {
		uInset = 0.5f / texWidth;
		vInset = 0.5f / texHeight;
	}

	const float u0 = (srcRect.left / texWidth) + uInset;
	const float v0 = (srcRect.top / texHeight) + vInset;
	const float u1 = (srcRect.right / texWidth) - uInset;
	const float v1 = (srcRect.bottom / texHeight) - vInset;

	const vector2 position = sprite->getPosition() + offset;
	const vector2 center = sprite->getCenter();
	const vector2 scale = sprite->getScale();
	const float c = std::cos(sprite->getRotation());
	const float sn = std::sin(sprite->getRotation());
	const float xs[4] = { -center.x, srcWidth - center.x, srcWidth - center.x, -center.x };
	const float ys[4] = { -center.y, -center.y, srcHeight - center.y, srcHeight - center.y };
	const float us[4] = { u0, u1, u1, u0 };
	const float vs[4] = { v0, v0, v1, v1 };

	Vertex quad[4];
	vector2 lo(INFINITY, INFINITY), hi(-INFINITY, -INFINITY);
	for (int i = 0; i < 4; ++i) {
		const float x = xs[i] * scale.x;
		const float y = ys[i] * scale.y;
		quad[i] = { position.x + c * x - sn * y, position.y + sn * x + c * y,
			us[i], vs[i], tint.r, tint.g, tint.b, tint.a };
		lo.x = std::min(lo.x, quad[i].x); lo.y = std::min(lo.y, quad[i].y);
		hi.x = std::max(hi.x, quad[i].x); hi.y = std::max(hi.y, quad[i].y);
	}

	const Impl& impl = *_impl;
	if (!screenSpace && (hi.x < impl.viewMin.x || lo.x > impl.viewMax.x || hi.y < impl.viewMin.y || lo.y > impl.viewMax.y)) {
		return;
	}
	_impl->addQuad(texture->getTexture(), quad);
}

void RendererMTL::_drawFont(Font* font, Color tint, const vector2& offset)
{
	if (!font) {
		return;
	}

	const std::string& text = font->getText();
	const int fontHeight = font->getHeight();
	if (text.empty() || fontHeight <= 0) {
		return;
	}

	// Each lit bitmap pixel is a quad, placed like RendererGL's
	// translate(position) * rotate(rotation) * scale(scale).
	const vector2 position = font->getPosition() + offset;
	const vector2 center = font->getCenter();
	const vector2 scale = font->getScale();
	const float c = std::cos(font->getRotation());
	const float sn = std::sin(font->getRotation());
	const auto place = [&](float x, float y, float u, float v) {
		x *= scale.x;
		y *= scale.y;
		return Vertex{ position.x + c * x - sn * y, position.y + sn * x + c * y, u, v, tint.r, tint.g, tint.b, tint.a };
	};

	id<MTLTexture> white = _impl->white->getTexture();
	float penX = -center.x;
	float penY = -center.y;
	const float lineAdvance = static_cast<float>(fontHeight + 1);
	for (char ch : text) {
		if (ch == '\n') {
			penX = -center.x;
			penY += lineAdvance;
			continue;
		}

		const std::vector<int>& bitmap = font->getBitmap(ch);
		const int glyphWidth = std::max(font->getWidth(ch), 0);
		const int glyphAdvance = std::max(font->getBitmapWidth(), 1);
		for (int row = 0; row < static_cast<int>(bitmap.size()); ++row) {
			const int rowBits = bitmap[row];
			for (int column = 0; column < glyphWidth; ++column) {
				if (((rowBits >> column) & 1) == 0) {
					continue;
				}
				const float left = penX + static_cast<float>(column);
				const float top = penY + static_cast<float>(row);
				const Vertex corners[4] = {
					place(left, top, 0.5f, 0.5f),
					place(left + 1.0f, top, 0.5f, 0.5f),
					place(left + 1.0f, top + 1.0f, 0.5f, 0.5f),
					place(left, top + 1.0f, 0.5f, 0.5f),
				};
				_impl->addQuad(white, corners);
			}
		}

		penX += static_cast<float>(glyphAdvance + 1);
	}
}

void RendererMTL::render(void)
{
	Impl& impl = *_impl;
	if (!impl.layer || m_nWidth <= 0 || m_nHeight <= 0) {
		return;
	}

	IRenderer::render();
	impl.updateDrawableSize();
	const CGSize drawableSize = impl.layer.drawableSize;
	if (drawableSize.width < 1.0 || drawableSize.height < 1.0) {
		return;
	}

	@autoreleasepool {
		// Build the frame. The background is the clear color over the logical
		// screen; anything outside it (letterboxing) stays black.
		impl.vertices.clear();
		impl.draws.clear();
		impl.transforms.clear();
		impl.setTransform(screenTransform((float)m_nWidth, (float)m_nHeight));
		impl.addRect(0.0f, 0.0f, (float)m_nWidth, (float)m_nHeight, m_ClearColor, true);

		_drawRenderLists(false);

		if (DEBUGGING && m_pCamera) {
			if (const CollisionSystem* collisionSystem = getActiveCollisionSystem()) {
				impl.setTransform(cameraTransform(m_pCamera, m_pCamera->getRenderPosition(), (float)m_nWidth, (float)m_nHeight));
				CollisionDebugDraw::draw(*collisionSystem, [&impl](const vector2& start, const vector2& end, float r, float g, float b, float a) {
					impl.addLine(start, end, r, g, b, a);
				});
			}
		}

		_drawRenderLists(true);

		// Scale the logical screen to fit the drawable, centred on whole pixels.
		const double scale = std::min(drawableSize.width / m_nWidth, drawableSize.height / m_nHeight);
		const double viewportWidth = std::max(1.0, std::round(m_nWidth * scale));
		const double viewportHeight = std::max(1.0, std::round(m_nHeight * scale));
		const double viewportX = std::floor((drawableSize.width - viewportWidth) * 0.5);
		const double viewportY = std::floor((drawableSize.height - viewportHeight) * 0.5);

		dispatch_semaphore_wait(impl.framesInFlight, DISPATCH_TIME_FOREVER);
		impl.frameIndex = (impl.frameIndex + 1) % kMaxFramesInFlight;
		id<MTLBuffer> vertexBuffer = impl.vertexBuffers[impl.frameIndex];
		const NSUInteger vertexBytes = (NSUInteger)(impl.vertices.size() * sizeof(Vertex));
		if (!vertexBuffer || vertexBuffer.length < vertexBytes) {
			vertexBuffer = [impl.device newBufferWithLength:std::max(kMinVertexBufferBytes, vertexBytes + vertexBytes / 2)
			                                        options:MTLResourceStorageModeShared];
			vertexBuffer.label = @"Sprite vertices";
			impl.vertexBuffers[impl.frameIndex] = vertexBuffer;
		}
		std::memcpy(vertexBuffer.contents, impl.vertices.data(), vertexBytes);

		id<CAMetalDrawable> drawable = [impl.layer nextDrawable];
		if (!drawable) {
			dispatch_semaphore_signal(impl.framesInFlight);
			return;
		}

		id<MTLCommandBuffer> commandBuffer = [impl.queue commandBuffer];
		dispatch_semaphore_t framesInFlight = impl.framesInFlight;
		[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
			dispatch_semaphore_signal(framesInFlight);
		}];

		MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
		pass.colorAttachments[0].texture = drawable.texture;
		pass.colorAttachments[0].loadAction = MTLLoadActionClear;
		pass.colorAttachments[0].storeAction = MTLStoreActionStore;
		pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

		id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
		encoder.label = @"Frame";
		[encoder setViewport:(MTLViewport){ viewportX, viewportY, viewportWidth, viewportHeight, 0.0, 1.0 }];
		[encoder setScissorRect:(MTLScissorRect){ (NSUInteger)viewportX, (NSUInteger)viewportY,
			(NSUInteger)viewportWidth, (NSUInteger)viewportHeight }];
		[encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];

		id<MTLRenderPipelineState> boundPipeline = nil;
		id<MTLTexture> boundTexture = nil;
		size_t boundTransform = (size_t)-1;
		for (const Impl::Draw& draw : impl.draws) {
			id<MTLRenderPipelineState> pipeline = draw.opaque ? impl.opaque : impl.blended;
			if (pipeline != boundPipeline) {
				[encoder setRenderPipelineState:pipeline];
				boundPipeline = pipeline;
			}
			if (draw.transform != boundTransform) {
				[encoder setVertexBytes:&impl.transforms[draw.transform] length:sizeof(Transform) atIndex:1];
				boundTransform = draw.transform;
			}
			if (draw.texture != boundTexture) {
				[encoder setFragmentTexture:draw.texture atIndex:0];
				boundTexture = draw.texture;
			}
			[encoder drawPrimitives:draw.primitive vertexStart:draw.start vertexCount:draw.count];
		}
		[encoder endEncoding];

		// Let the window capture this frame if asked; that needs the finished image.
		bool committed = false;
		if (_window) {
			vector2 viewMin(0.0f, 0.0f), viewMax(0.0f, 0.0f);
			if (m_pCamera) {
				_viewBounds(m_pCamera->getRenderPosition(), viewMin, viewMax);
			}
			_window->onFrameRendered(viewMin, viewMax, [&](const std::string& path) {
				id<MTLTexture> image = drawable.texture;
				const NSUInteger width = image.width;
				const NSUInteger height = image.height;
				id<MTLBuffer> readback = [impl.device newBufferWithLength:width * height * 4 options:MTLResourceStorageModeShared];
				id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
				[blit copyFromTexture:image sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
				           sourceSize:MTLSizeMake(width, height, 1) toBuffer:readback destinationOffset:0
				destinationBytesPerRow:width * 4 destinationBytesPerImage:width * height * 4];
				[blit endEncoding];
				[commandBuffer commit];
				[commandBuffer waitUntilCompleted];
				committed = true;
				return writePNG((const unsigned char*)readback.contents, width, height, path);
			});
		}

		FramePacer::presentBegin();
		if (committed) {
			id<MTLCommandBuffer> presentBuffer = [impl.queue commandBuffer];
			[presentBuffer presentDrawable:drawable];
			[presentBuffer commit];
		}
		else {
			[commandBuffer presentDrawable:drawable];
			[commandBuffer commit];
		}
		// When pacing, include the GPU work in the measured frame time (as RendererGL's glFinish).
		if (FramePacer::wantsHardSync()) {
			[commandBuffer waitUntilCompleted];
		}
		FramePacer::presentEnd();
	}
}

#endif //__APPLE__
