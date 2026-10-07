#pragma once

#include "Types.h"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

class Camera;
class IRenderer;
class Renderable;

// Fixed-step render interpolation. The simulation advances in whole ticks, so
// at an arbitrary present time the latest tick is up to one tick old (or new).
// Before each tick the world-space renderables' positions are recorded; at
// render time each one is drawn at lerp(previous, current, alpha), where alpha
// is the leftover accumulator fraction, and restored straight after so game
// code only ever sees simulation positions. Screen-space lists are left alone.
class RenderInterpolation
{
	struct Previous { Renderable* renderable; vector2 position; std::uint32_t appliedFrame; };

	// Recorded in render-list order, so apply() can match by walking the lists in
	// the same order; the index is only built if the lists changed in between.
	std::vector<Previous> _previous;
	std::unordered_map<Renderable*, size_t> _index;
	bool _indexBuilt = false;
	std::vector<std::pair<Renderable*, vector2>> _restore;
	Camera* _camera = nullptr;
	vector2 _cameraPrevious{ 0.0f, 0.0f };
	vector2 _cameraCurrent{ 0.0f, 0.0f };
	bool _cameraApplied = false;
	std::uint32_t _frame = 0;
	float _snapDistanceSquared = 128.0f * 128.0f;

	bool shouldBlend(vector2 from, vector2 to) const;
	Previous* find(Renderable* renderable, size_t& cursor);

public:
	// Moves longer than this per tick are teleports and are drawn without blending.
	void setSnapDistance(float distance) { _snapDistanceSquared = distance * distance; }
	void clear(void);

	// Call before each simulation tick.
	void snapshot(IRenderer* renderer);
	// Call just before rendering; alpha in [0, 1].
	void apply(IRenderer* renderer, float alpha);
	// Call right after rendering.
	void restore(IRenderer* renderer);
};
