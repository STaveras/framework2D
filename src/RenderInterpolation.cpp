// File: RenderInterpolation.cpp
#include "RenderInterpolation.h"
#include "Camera.h"
#include "IRenderer.h"
#include "Renderable.h"

namespace {
template <typename Fn>
void forEachWorldRenderable(IRenderer* renderer, Fn&& fn)
{
	const size_t count = renderer->getRenderListCount();
	for (size_t i = 0; i < count; ++i) {
		IRenderer::RenderList* list = renderer->getRenderList(i);
		if (!list || list->screenSpace) continue;
		for (Renderable* renderable : *list) {
			if (renderable) fn(renderable);
		}
	}
}

vector2 lerp(vector2 from, vector2 to, float alpha)
{
	return vector2(from.x + (to.x - from.x) * alpha, from.y + (to.y - from.y) * alpha);
}
}

bool RenderInterpolation::shouldBlend(vector2 from, vector2 to) const
{
	const float dx = to.x - from.x, dy = to.y - from.y;
	const float distanceSquared = dx * dx + dy * dy;
	// Unmoved: nothing to do. Beyond the snap distance: a teleport (respawn,
	// level change, a recycled pointer), which must not be smeared.
	return distanceSquared > 0.0f && distanceSquared <= _snapDistanceSquared;
}

RenderInterpolation::Previous* RenderInterpolation::find(Renderable* renderable, size_t& cursor)
{
	if (cursor < _previous.size() && _previous[cursor].renderable == renderable) {
		return &_previous[cursor++];
	}
	if (!_indexBuilt) {
		_index.clear();
		for (size_t i = 0; i < _previous.size(); ++i) _index.emplace(_previous[i].renderable, i);
		_indexBuilt = true;
	}
	auto found = _index.find(renderable);
	if (found == _index.end()) return nullptr;
	cursor = found->second + 1;
	return &_previous[found->second];
}

void RenderInterpolation::clear(void)
{
	_previous.clear();
	_index.clear();
	_indexBuilt = false;
	_restore.clear();
	_camera = nullptr;
	_cameraApplied = false;
}

void RenderInterpolation::snapshot(IRenderer* renderer)
{
	if (!renderer) return;
	_previous.clear();
	_indexBuilt = false;
	forEachWorldRenderable(renderer, [this](Renderable* renderable) {
		_previous.push_back(Previous{ renderable, renderable->getPosition(), 0 });
	});
	_camera = renderer->getCamera();
	if (_camera) _cameraPrevious = _camera->getPosition();
}

void RenderInterpolation::apply(IRenderer* renderer, float alpha)
{
	_restore.clear();
	_cameraApplied = false;
	if (!renderer || _previous.empty()) return;
	++_frame;
	size_t cursor = 0;
	forEachWorldRenderable(renderer, [this, alpha, &cursor](Renderable* renderable) {
		Previous* previous = find(renderable, cursor);
		// A renderable can sit in several lists; blend it only once.
		if (!previous || previous->appliedFrame == _frame) return;
		previous->appliedFrame = _frame;
		const vector2 current = renderable->getPosition();
		if (!shouldBlend(previous->position, current)) return;
		renderable->setPosition(lerp(previous->position, current, alpha));
		_restore.emplace_back(renderable, current);
	});

	Camera* camera = renderer->getCamera();
	if (camera && camera == _camera) {
		_cameraCurrent = camera->getPosition();
		if (shouldBlend(_cameraPrevious, _cameraCurrent)) {
			// Only the camera's own position: GameObject::setPosition would also
			// move whatever renderable the camera carries.
			camera->setPositionSilently(lerp(_cameraPrevious, _cameraCurrent, alpha));
			_cameraApplied = true;
		}
	}
}

void RenderInterpolation::restore(IRenderer* renderer)
{
	for (auto it = _restore.rbegin(); it != _restore.rend(); ++it) {
		it->first->setPosition(it->second);
	}
	_restore.clear();
	if (_cameraApplied && renderer && renderer->getCamera() == _camera) {
		_camera->setPositionSilently(_cameraCurrent);
	}
	_cameraApplied = false;
}
