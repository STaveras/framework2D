// File: IRenderer.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 4/1/2022

#include "IRenderer.h"
#include "Animation.h"
#include "Camera.h"
#include "Font.h"
#include "Frame.h"
#include "Sprite.h"
#include "Timer.h"

#include "Engine2D.h"
#include "RuntimeProfile.h"
#include <algorithm>
#include <cmath>

IRenderer::~IRenderer() {
	m_Textures.clear();
}

// NOTE: Initial test demo
void IRenderer::_backgroundColorShift(void)
{
	if (m_bStaticBG || m_BackgroundColorPoints.size() < 2) {
		return;
	}

	Timer* timer = Engine2D::getTimer();
	if (!timer) {
		return;
	}

	m_BackgroundColorProgress += timer->getDeltaTime();
	while (m_BackgroundColorProgress >= 1.0f)
	{
		m_BackgroundColorProgress -= 1.0f;
		m_BackgroundColorStage = (m_BackgroundColorStage + 1) % m_BackgroundColorPoints.size();
	}

	const size_t nextStage = (m_BackgroundColorStage + 1) % m_BackgroundColorPoints.size();
	const Color startColor = m_BackgroundColorPoints[m_BackgroundColorStage];
	const Color endColor = m_BackgroundColorPoints[nextStage];

	const float t = std::clamp(m_BackgroundColorProgress, 0.0f, 1.0f);
	Color interpolatedColor = m_ClearColor;

	const auto lerpChannel = [t](byte start, byte end) -> byte {
		return static_cast<byte>(static_cast<float>(start) + (static_cast<float>(end) - static_cast<float>(start)) * t);
	};

	interpolatedColor.r = lerpChannel(startColor.r, endColor.r);
	interpolatedColor.g = lerpChannel(startColor.g, endColor.g);
	interpolatedColor.b = lerpChannel(startColor.b, endColor.b);
	interpolatedColor.a = lerpChannel(startColor.a, endColor.a);

	m_ClearColor = interpolatedColor;
}

ITexture* IRenderer::_textureExists(const char* szFilename)
{
	Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();
	for (; itr != m_Textures.end(); ++itr) {
		if (!strcmp((*itr)->getFileName(), szFilename)) {
			return (*itr);
		}
	}
	return NULL;
}

ITexture* IRenderer::_textureExists(const char* szFilename, Color colorKey)
{
	Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();

	for (; itr != m_Textures.end(); itr++)
	{
		if (!strcmp((*itr)->getFileName(), szFilename) && (*itr)->getKeyColor()._color == colorKey._color)
			return (*itr);
	}

	return NULL;
}

void IRenderer::setClearColor(Color clearColor)
{
	m_ClearColor = clearColor;
	//m_bStaticBG = true;
}

void IRenderer::setBackgroundColorPoints(const std::vector<Color>& points)
{
	if (points.size() < 2) {
		return;
	}

	m_BackgroundColorPoints = points;
	resetBackgroundColorShift();
}

void IRenderer::resetBackgroundColorShift(void)
{
	m_BackgroundColorStage = 0;
	m_BackgroundColorProgress = 0.0f;
	m_ClearColor = m_BackgroundColorPoints.front();
}

void IRenderer::setCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
	
	if (pCamera) 
	{
		m_pCamera->setScreenWidth(m_nWidth);
		m_pCamera->setScreenHeight(m_nHeight);
	}
}

bool IRenderer::destroyTexture(const ITexture* pTexture)
{
	if (pTexture)
	{
		Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();

		for(;itr != m_Textures.end(); itr++)
		{
			if(pTexture == (*itr))
			{
				m_Textures.erase(itr);
				return true;
			}
		}
	}

	return false;
}

void IRenderer::render(void) {
	_backgroundColorShift();
}

namespace {
// Axis-aligned bounds of the quad every backend draws for sprite: its source rect
// around its center, scaled, rotated, at position + offset. The backends cull with
// these same bounds, so testing them first skips exactly the sprites they would.
void spriteQuadBounds(const Sprite* sprite, const vector2& offset, vector2& lo, vector2& hi)
{
	const RECT& srcRect = sprite->getSrcRect();
	const vector2 position = sprite->getPosition() + offset;
	const vector2 center = sprite->getCenter();
	const vector2 scale = sprite->getScale();
	const float rotationRadians = sprite->getRotation();
	const float x0 = -center.x * scale.x;
	const float x1 = (static_cast<float>(srcRect.right - srcRect.left) - center.x) * scale.x;
	const float y0 = -center.y * scale.y;
	const float y1 = (static_cast<float>(srcRect.bottom - srcRect.top) - center.y) * scale.y;

	lo = vector2(position.x + std::min(x0, x1), position.y + std::min(y0, y1));
	hi = vector2(position.x + std::max(x0, x1), position.y + std::max(y0, y1));
	if (rotationRadians != 0.0f) {
		const float c = std::cos(rotationRadians), sn = std::sin(rotationRadians);
		const float xs[4] = { x0, x1, x1, x0 };
		const float ys[4] = { y0, y0, y1, y1 };
		lo = vector2(INFINITY, INFINITY);
		hi = vector2(-INFINITY, -INFINITY);
		for (int i = 0; i < 4; ++i) {
			const float x = position.x + c * xs[i] - sn * ys[i];
			const float y = position.y + sn * xs[i] + c * ys[i];
			lo.x = std::min(lo.x, x); lo.y = std::min(lo.y, y);
			hi.x = std::max(hi.x, x); hi.y = std::max(hi.y, y);
		}
	}
}

bool overlapsView(const vector2& lo, const vector2& hi, const vector2& viewMin, const vector2& viewMax)
{
	return !(hi.x < viewMin.x || lo.x > viewMax.x || hi.y < viewMin.y || lo.y > viewMax.y);
}

// Grows [min, max] to cover [lo, hi]. A NaN bound sticks, so a chunk holding a NaN
// quad is never culled, just as the quad itself never is.
void growBounds(vector2& min, vector2& max, const vector2& lo, const vector2& hi)
{
	if (min.x == min.x && !(lo.x >= min.x)) min.x = lo.x;
	if (min.y == min.y && !(lo.y >= min.y)) min.y = lo.y;
	if (max.x == max.x && !(hi.x <= max.x)) max.x = hi.x;
	if (max.y == max.y && !(hi.y <= max.y)) max.y = hi.y;
}

void measureChunk(RenderCulling::Segment& segment, const std::vector<Renderable*>& items)
{
	segment.min = vector2(INFINITY, INFINITY);
	segment.max = vector2(-INFINITY, -INFINITY);
	for (std::uint32_t i = segment.begin; i < segment.end; ++i) {
		const Sprite* sprite = static_cast<const Sprite*>(items[i]);
		vector2 lo, hi;
		spriteQuadBounds(sprite, sprite->getOffset(), lo, hi);
		growBounds(segment.min, segment.max, lo, hi);
	}
}

// A chunk stops growing at this many sprites or this much of the world, so one
// straddling the view's edge costs few wasted visits.
constexpr std::uint32_t kMaxChunkSprites = 64;
constexpr float kMaxChunkExtent = 256.0f;
}

void IRenderer::_groupForCulling(RenderList& renderList)
{
	RenderCulling::ListCache& culling = renderList.culling;
	culling.release();
	culling.items.assign(renderList.begin(), renderList.end());
	culling.revision = renderList.revision;
	culling.built = true;

	constexpr std::uint32_t kNone = ~0u;
	std::uint32_t open = kNone; // the chunk segment still taking sprites
	for (std::uint32_t i = 0; i < culling.items.size(); ++i) {
		Renderable* renderable = culling.items[i];
		// Animations change frames without telling anyone, and a sprite reports to
		// only one chunk (it may also sit in another list, or twice in this one).
		// Visit those every frame.
		if (!renderable || renderable->getRenderableType() != RENDERABLE_TYPE_SPRITE ||
			RenderCulling::reportsToChunk(renderable, renderable->_cullChunk.id)) {
			culling.segments.push_back(RenderCulling::Segment{ i, i + 1, 0 });
			open = kNone;
			continue;
		}

		vector2 lo, hi;
		spriteQuadBounds(static_cast<const Sprite*>(renderable), renderable->getOffset(), lo, hi);
		if (open != kNone) {
			RenderCulling::Segment& segment = culling.segments[open];
			vector2 min = segment.min, max = segment.max;
			growBounds(min, max, lo, hi);
			if (segment.end - segment.begin < kMaxChunkSprites &&
				max.x - min.x <= kMaxChunkExtent && max.y - min.y <= kMaxChunkExtent) {
				segment.end = i + 1;
				segment.min = min;
				segment.max = max;
				renderable->_cullChunk.id = segment.chunk;
				continue;
			}
		}

		open = static_cast<std::uint32_t>(culling.segments.size());
		culling.segments.push_back(RenderCulling::Segment{ i, i + 1, culling.allocateChunk(open), lo, hi });
		renderable->_cullChunk.id = culling.segments.back().chunk;
	}
}

void IRenderer::_drawRenderable(Renderable* renderable, const RenderList& renderList, bool cull, const vector2& viewMin, const vector2& viewMax)
{
	if (!renderable || !renderable->isVisible()) {
		return;
	}

	switch (renderable->getRenderableType()) {
	case RENDERABLE_TYPE_SPRITE:
	{
		Sprite* sprite = (Sprite*)renderable;
		vector2 lo, hi;
		if (cull) {
			spriteQuadBounds(sprite, sprite->getOffset(), lo, hi);
		}
		if (!cull || overlapsView(lo, hi, viewMin, viewMax)) {
			_renderSprite(sprite, sprite->getTintColor(), sprite->getOffset(), renderList);
		}
	}
	break;
	case RENDERABLE_TYPE_ANIMATION:
	{
		Animation* animation = (Animation*)renderable;
		Frame* frame = animation->getFrameCount() ? animation->getCurrentFrame() : NULL;
		Sprite* sprite = frame ? frame->getSprite() : NULL;
		vector2 lo, hi;
		if (sprite && cull) {
			spriteQuadBounds(sprite, animation->getOffset(), lo, hi);
		}
		if (sprite && (!cull || overlapsView(lo, hi, viewMin, viewMax))) {
			_renderSprite(sprite, sprite->getTintColor(), animation->getOffset(), renderList);
		}
	}
	break;
	case RENDERABLE_TYPE_FONT:
	{
		Font* font = (Font*)renderable;
		_renderFont(font, font->getTintColor(), font->getOffset(), renderList);
	}
	break;
	default:
		break;
	}
}

void IRenderer::_drawRenderLists(bool screenSpace)
{
	RuntimeProfile::Scope profile(RuntimeProfile::Region::RenderLists);
	const bool cull = !screenSpace && m_pCamera;
	if (cull) {
		// Ungroup every changed list before regrouping any, so a sprite moved from
		// one list to another is not taken for one that sits in both.
		for (RenderList* renderList : _RenderLists) {
			if (renderList && !renderList->screenSpace && renderList->culling.built &&
				renderList->culling.revision != renderList->revision) {
				renderList->culling.release();
			}
		}
	}

	for (RenderList* renderList : _RenderLists) {
		if (!renderList || renderList->screenSpace != screenSpace) {
			continue;
		}

		_beginRenderList(*renderList);

		if (!cull) {
			for (Renderable* renderable : *renderList) {
				_drawRenderable(renderable, *renderList, false, vector2(0.0f, 0.0f), vector2(0.0f, 0.0f));
			}
		}
		else {
			vector2 viewMin(0.0f, 0.0f), viewMax(0.0f, 0.0f);
			_viewBounds(_parallaxCameraPosition(*renderList), viewMin, viewMax);

			RenderCulling::ListCache& culling = renderList->culling;
			if (!culling.built || culling.revision != renderList->revision) {
				_groupForCulling(*renderList);
			}
			for (RenderCulling::Segment& segment : culling.segments) {
				if (segment.chunk) {
					RenderCulling::Chunk& chunk = RenderCulling::chunkTable()[segment.chunk];
					if (chunk.dirty) {
						measureChunk(segment, culling.items);
						chunk.dirty = false;
					}
					if (!overlapsView(segment.min, segment.max, viewMin, viewMax)) {
						continue;
					}
				}
				for (std::uint32_t i = segment.begin; i < segment.end; ++i) {
					_drawRenderable(culling.items[i], *renderList, true, viewMin, viewMax);
				}
			}
		}

		_endRenderList(*renderList);
	}
}

vector2 IRenderer::_parallaxCameraPosition(const vector2& parallaxFactor, const vector2& parallaxOrigin) const
{
	const vector2 baseCameraPosition = m_pCamera->getRenderPosition();
	return parallaxOrigin + ((baseCameraPosition - parallaxOrigin) * parallaxFactor);
}

vector2 IRenderer::_parallaxCameraPosition(const RenderList& renderList) const
{
	return _parallaxCameraPosition(
		vector2(renderList.parallaxX, renderList.parallaxY),
		vector2(renderList.parallaxOriginX, renderList.parallaxOriginY));
}

void IRenderer::_viewBounds(const vector2& cameraPosition, vector2& viewMin, vector2& viewMax) const
{
	viewMin = vector2(INFINITY, INFINITY);
	viewMax = vector2(-INFINITY, -INFINITY);
	const float zoom = m_pCamera && m_pCamera->getZoom() > 0 ? m_pCamera->getZoom() : 1.0f;
	const float angle = m_pCamera ? -m_pCamera->getRotation() : 0.0f;
	const float c = std::cos(angle), sn = std::sin(angle);
	for (int i = 0; i < 4; ++i) {
		vector2 point((i & 1) ? m_nWidth : 0, (i & 2) ? m_nHeight : 0);
		if (m_pCamera && m_pCamera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter)
			point = point - m_pCamera->getCenter();
		point /= zoom;
		point = vector2(c * point.x - sn * point.y, sn * point.x + c * point.y);
		if (m_pCamera) {
			point = point + cameraPosition;
			if (m_pCamera->getZoomAnchorMode() != Camera::ZoomAnchorMode::TargetCenter)
				point = point - m_pCamera->getCenter();
		}
		viewMin.x = std::min(viewMin.x, point.x); viewMin.y = std::min(viewMin.y, point.y);
		viewMax.x = std::max(viewMax.x, point.x); viewMax.y = std::max(viewMax.y, point.y);
	}
}

vector2 IRenderer::_worldToScreen(const vector2& worldPosition, const vector2& cameraPosition) const
{
	if (!m_pCamera) {
		return worldPosition;
	}

	const vector2 center = m_pCamera->getCenter();
	const float zoom = (m_pCamera->getZoom() > 0.0f) ? m_pCamera->getZoom() : 1.0f;
	const float rotation = m_pCamera->getRotation();
	const float cosTheta = std::cos(rotation);
	const float sinTheta = std::sin(rotation);

	if (m_pCamera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter) {
		const vector2 translated = worldPosition - cameraPosition;
		const vector2 rotated(
			(translated.x * cosTheta) - (translated.y * sinTheta),
			(translated.x * sinTheta) + (translated.y * cosTheta));
		return vector2(
			center.x + (rotated.x * zoom),
			center.y + (rotated.y * zoom));
	}

	// Preserve legacy origin-oriented behavior.
	const vector2 legacyTranslated = worldPosition - (cameraPosition - center);
	const vector2 legacyRotated(
		(legacyTranslated.x * cosTheta) - (legacyTranslated.y * sinTheta),
		(legacyTranslated.x * sinTheta) + (legacyTranslated.y * cosTheta));
	return vector2(legacyRotated.x * zoom, legacyRotated.y * zoom);
}
