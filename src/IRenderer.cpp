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

	for (; itr != m_Textures.end(); itr++)
	{
		if(!strcmp((*itr)->getFileName(), szFilename))
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
// Whether the quad every backend draws for sprite (its source rect around its
// center, scaled, rotated, at position + offset) overlaps the view. This is the
// same bounds test the backends cull with, done before they build the quad, so it
// skips exactly the sprites they would skip: the many off-screen tiles of a large map.
bool spriteMayBeVisible(const Sprite* sprite, const vector2& offset, const vector2& viewMin, const vector2& viewMax)
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

	vector2 lo(position.x + std::min(x0, x1), position.y + std::min(y0, y1));
	vector2 hi(position.x + std::max(x0, x1), position.y + std::max(y0, y1));
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
	return !(hi.x < viewMin.x || lo.x > viewMax.x || hi.y < viewMin.y || lo.y > viewMax.y);
}
}

void IRenderer::_drawRenderLists(bool screenSpace)
{
	for (RenderList* renderList : _RenderLists) {
		if (!renderList || renderList->screenSpace != screenSpace) {
			continue;
		}

		_beginRenderList(*renderList);

		const bool cull = !renderList->screenSpace && m_pCamera;
		vector2 viewMin(0.0f, 0.0f), viewMax(0.0f, 0.0f);
		if (cull) {
			_viewBounds(_parallaxCameraPosition(*renderList), viewMin, viewMax);
		}

		for (Renderable* renderable : *renderList) {
			if (!renderable || !renderable->isVisible()) {
				continue;
			}

			switch (renderable->getRenderableType()) {
			case RENDERABLE_TYPE_SPRITE:
			{
				Sprite* sprite = (Sprite*)renderable;
				if (!cull || spriteMayBeVisible(sprite, sprite->getOffset(), viewMin, viewMax)) {
					_renderSprite(sprite, sprite->getTintColor(), sprite->getOffset(), *renderList);
				}
			}
			break;
			case RENDERABLE_TYPE_ANIMATION:
			{
				Animation* animation = (Animation*)renderable;
				Frame* frame = animation->getFrameCount() ? animation->getCurrentFrame() : NULL;
				Sprite* sprite = frame ? frame->getSprite() : NULL;
				if (sprite && (!cull || spriteMayBeVisible(sprite, animation->getOffset(), viewMin, viewMax))) {
					_renderSprite(sprite, sprite->getTintColor(), animation->getOffset(), *renderList);
				}
			}
			break;
			case RENDERABLE_TYPE_FONT:
			{
				Font* font = (Font*)renderable;
				_renderFont(font, font->getTintColor(), font->getOffset(), *renderList);
			}
			break;
			default:
				break;
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
