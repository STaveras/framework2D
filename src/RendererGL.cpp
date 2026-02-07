// RendererGL.cpp

#include "RendererGL.h"

#include "Animation.h"
#include "Camera.h"
#include "CollisionSystem.h"
#include "Debug.h"
#include "Engine2D.h"
#include "Frame.h"
#include "Game.h"
#include "GameState.h"
#include "Sprite.h"
#include "TextureGL.h"

#include <cmath>

namespace {
constexpr float kRadiansToDegrees = 180.0f / 3.14159265358979323846f;

const CollisionSystem* getActiveCollisionSystem()
{
	Game* game = Engine2D::getGame();
	if (!game || game->empty()) {
		return nullptr;
	}

	ProgramState* topState = game->top();
	GameState* gameState = dynamic_cast<GameState*>(topState);
	if (!gameState) {
		return nullptr;
	}

	return gameState->getCollisionSystem();
}

void drawLine(const vector2& start, const vector2& end, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_LINES);
	glVertex2f(start.x, start.y);
	glVertex2f(end.x, end.y);
	glEnd();
}

void drawCross(const vector2& position, float radius, float r, float g, float b, float a)
{
	drawLine(
		vector2(position.x - radius, position.y),
		vector2(position.x + radius, position.y),
		r, g, b, a);
	drawLine(
		vector2(position.x, position.y - radius),
		vector2(position.x, position.y + radius),
		r, g, b, a);
}

void drawRect(const vector2& min, const vector2& max, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_LINE_LOOP);
	glVertex2f(min.x, min.y);
	glVertex2f(max.x, min.y);
	glVertex2f(max.x, max.y);
	glVertex2f(min.x, max.y);
	glEnd();
}

void drawCollisionDebugOverlay(const CollisionSystem& collisionSystem)
{
	const auto& shapes = collisionSystem.getDebugShapes();
	const auto& contacts = collisionSystem.getDebugContacts();
	if (shapes.empty() && contacts.empty()) {
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.0f);

	for (const CollisionDebugShape& shape : shapes) {
		if (!shape.collidable || !shape.hasBounds) {
			continue;
		}

		float r = 0.2f;
		float g = 0.95f;
		float b = 0.25f;
		float a = 0.9f;

		if (!shape.collidableActive) {
			r = 0.45f;
			g = 0.45f;
			b = 0.45f;
		}
		else if (shape.hasContact) {
			switch (shape.phase) {
			case CollisionPhase::Enter:
				r = 1.0f;
				g = 0.25f;
				b = 0.25f;
				break;
			case CollisionPhase::Stay:
				r = 1.0f;
				g = 0.9f;
				b = 0.15f;
				break;
			case CollisionPhase::Exit:
				r = 1.0f;
				g = 0.35f;
				b = 1.0f;
				break;
			}
		}

		drawRect(shape.min, shape.max, r, g, b, a);
		drawCross(shape.objectPosition, 2.0f, 1.0f, 1.0f, 1.0f, 0.9f);
		drawCross(shape.collisionAnchor, 2.0f, 0.0f, 1.0f, 1.0f, 0.9f);

		if (shape.renderableOffset.x != 0.0f || shape.renderableOffset.y != 0.0f) {
			drawCross(shape.anchorWithRenderableOffset, 2.0f, 0.7f, 0.4f, 1.0f, 0.9f);
			drawLine(shape.objectPosition, shape.anchorWithRenderableOffset, 0.35f, 0.35f, 1.0f, 0.75f);
		}
	}

	for (const CollisionDebugContact& contact : contacts) {
		if (!contact.overlapping || !contact.midpoint.has_value()) {
			continue;
		}

		const vector2& midpoint = contact.midpoint.value();
		drawCross(midpoint, 2.0f, 1.0f, 0.4f, 0.0f, 0.9f);

		if (contact.normal.has_value()) {
			vector2 normal = contact.normal.value();
			float normalLength = std::sqrt((normal.x * normal.x) + (normal.y * normal.y));
			if (normalLength > 0.0f) {
				normal = vector2(normal.x / normalLength, normal.y / normalLength);
				float lineLength = 12.0f + (contact.penetrationDepth.value_or(0.0f) * 4.0f);
				drawLine(midpoint, midpoint + (normal * lineLength), 1.0f, 0.4f, 0.0f, 1.0f);
			}
		}
	}

	glEnable(GL_TEXTURE_2D);
}
}

RendererGL::RendererGL() : IRenderer(RENDERER_TYPE_GL) {}

RendererGL::RendererGL(Window* window) :
	IRenderer(RENDERER_TYPE_GL, window ? window->getWidth() : 0, window ? window->getHeight() : 0),
	_window(window ? window->getUnderlyingWindow() : nullptr)
{
}

RendererGL::~RendererGL()
{
	shutdown();
}

void RendererGL::setVerticalSync(bool vsyncEnabled)
{
	IRenderer::setVerticalSync(vsyncEnabled);
	if (_window) {
		glfwSwapInterval(vsyncEnabled ? 1 : 0);
	}
}

void RendererGL::_drawImage(Sprite* sprite, Color tint, vector2 offset)
{
	if (!sprite) {
		return;
	}

	TextureGL* texture = static_cast<TextureGL*>(const_cast<ITexture*>(sprite->getTexture()));
	if (!texture) {
		return;
	}

	const RECT& srcRect = sprite->getSrcRect();
	const float srcWidth = static_cast<float>(srcRect.right - srcRect.left);
	const float srcHeight = static_cast<float>(srcRect.bottom - srcRect.top);

	if (srcWidth <= 0.0f || srcHeight <= 0.0f) {
		return;
	}

	const float texWidth = static_cast<float>(texture->getWidth());
	const float texHeight = static_cast<float>(texture->getHeight());

	const float u0 = srcRect.left / texWidth;
	const float v0 = srcRect.top / texHeight;
	const float u1 = srcRect.right / texWidth;
	const float v1 = srcRect.bottom / texHeight;

	const vector2 position = sprite->getPosition() + offset;
	const vector2 center = sprite->getCenter();
	const vector2 scale = sprite->getScale();
	const float rotationRadians = sprite->getRotation();

	glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

	glColor4f(tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f);

	glPushMatrix();
	// Match DirectX sprite semantics: position refers to the sprite's center.
	glTranslatef(position.x, position.y, 0.0f);
	glRotatef(rotationRadians * kRadiansToDegrees, 0.0f, 0.0f, 1.0f);
	glScalef(scale.x, scale.y, 1.0f);

	glBegin(GL_QUADS);
	glTexCoord2f(u0, v0);
	glVertex2f(-center.x, -center.y);

	glTexCoord2f(u1, v0);
	glVertex2f(srcWidth - center.x, -center.y);

	glTexCoord2f(u1, v1);
	glVertex2f(srcWidth - center.x, srcHeight - center.y);

	glTexCoord2f(u0, v1);
	glVertex2f(-center.x, srcHeight - center.y);
	glEnd();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);
}

ITexture* RendererGL::createTexture(const char* szFilename, Color colorKey)
{
	ITexture* pTexture = _textureExists(szFilename);

	if (!pTexture) {
		pTexture = (ITexture*)new TextureGL(szFilename);
		pTexture->SetKeyColor(colorKey);
		m_Textures.store(pTexture);
	}

	return pTexture;
}

bool RendererGL::destroyTexture(const ITexture* texture)
{
	if (!texture) {
		return false;
	}

	return IRenderer::destroyTexture(texture);
}

void RendererGL::initialize(void)
{
	if (!_window) {
		return;
	}

	glfwMakeContextCurrent(_window);

	setVerticalSync(m_bVerticalSync);

	int framebufferWidth = 0;
	int framebufferHeight = 0;
	glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
	if (framebufferWidth <= 0 || framebufferHeight <= 0) {
		framebufferWidth = m_nWidth;
		framebufferHeight = m_nHeight;
	}
	glViewport(0, 0, framebufferWidth, framebufferHeight);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, static_cast<double>(m_nWidth), static_cast<double>(m_nHeight), 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RendererGL::shutdown(void)
{
	if (_window) {
		glfwMakeContextCurrent(_window);
	}
}

void RendererGL::render(void)
{
	if (!_window) {
		return;
	}

	IRenderer::render();

	int framebufferWidth = 0;
	int framebufferHeight = 0;
	glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
	if (framebufferWidth <= 0 || framebufferHeight <= 0) {
		framebufferWidth = m_nWidth;
		framebufferHeight = m_nHeight;
	}
	glViewport(0, 0, framebufferWidth, framebufferHeight);

	glClearColor(m_ClearColor.r / 255.0f, m_ClearColor.g / 255.0f, m_ClearColor.b / 255.0f, m_ClearColor.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, static_cast<double>(m_nWidth), static_cast<double>(m_nHeight), 0.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (m_pCamera) {
		vector2 cameraPosition = m_pCamera->getPosition() - m_pCamera->getCenter();

		glScalef(m_pCamera->getZoom(), m_pCamera->getZoom(), 1.0f);
		glRotatef(m_pCamera->getRotation() * kRadiansToDegrees, 0.0f, 0.0f, 1.0f);
		glTranslatef(-cameraPosition.x, -cameraPosition.y, 0.0f);
	}

	if (!_RenderLists.empty()) {
		for (unsigned int i = 0; i < _RenderLists.size(); i++) {
			for (RenderList::iterator o = _RenderLists.at(i)->begin(); o != _RenderLists.at(i)->end(); o++) {
				if ((*o) && (*o)->isVisible()) {
					switch ((*o)->getRenderableType()) {
					case RENDERABLE_TYPE_NULL:
					case RENDERABLE_TYPE_WIDGET:
					case RENDERABLE_TYPE_FONT:
						break;
					case RENDERABLE_TYPE_SPRITE:
					{
						Image* image = (Image*)(*o);
						_drawImage(image, image->getTintColor(), image->getOffset());
					}
					break;
					case RENDERABLE_TYPE_ANIMATION:
					{
						Animation* animation = (Animation*)(*o);
						if (animation->getFrameCount()) {
							_drawImage(animation->getCurrentFrame()->getSprite(),
								animation->getCurrentFrame()->getSprite()->getTintColor(),
								animation->getOffset());
						}
					}
					break;
					}
				}
			}
		}
	}

	if (DEBUGGING/* && Debug::dbgCollision*/) {
		if (const CollisionSystem* collisionSystem = getActiveCollisionSystem()) {
			drawCollisionDebugOverlay(*collisionSystem);
		}
	}

	glfwSwapBuffers(_window);
}
