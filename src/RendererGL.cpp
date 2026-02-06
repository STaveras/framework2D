// RendererGL.cpp

#include "RendererGL.h"

#include "Animation.h"
#include "Camera.h"
#include "Frame.h"
#include "Sprite.h"
#include "TextureGL.h"

#include <cmath>

namespace {
constexpr float kRadiansToDegrees = 180.0f / 3.14159265358979323846f;
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

	glfwSwapBuffers(_window);
}
