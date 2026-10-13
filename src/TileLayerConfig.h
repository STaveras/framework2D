#pragma once

#include "Maths.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

struct TileMapPropertyDescriptor
{
	std::string name;
	std::string type;
	std::string value;
};

// TODO: Make these state keys that correspond to states that are built-in to tiles
enum class TileCollisionMode
{
	None,
	Solid,
	OneWay
};

// How a layer moves the art of its uv-looping tiles (TileSet uv_loop_x/uv_loop_y), from the
// uv_scroll_x/y, uv_sway_x/y and uv_sway_period properties of the layer or the groups it is
// in. Distances are source pixels; positive values move the art right or down on screen.
struct TileUVMotion
{
	vector2 scroll; // pixels per second
	vector2 sway; // oscillation amplitude in pixels
	float swayPeriod = 0.0f; // seconds per back-and-forth cycle

	bool movesX(void) const { return scroll.x != 0.0f || (sway.x != 0.0f && swayPeriod > 0.0f); }
	bool movesY(void) const { return scroll.y != 0.0f || (sway.y != 0.0f && swayPeriod > 0.0f); }

	// How far the art has moved after the given time. The scroll part is wrapped by
	// loopSize (when non-zero) so long play times keep their precision.
	vector2 displacementAt(double seconds, vector2 loopSize) const
	{
		const double swayAmount = (swayPeriod > 0.0f) ? std::sin(seconds * 6.283185307179586 / swayPeriod) : 0.0;
		auto axis = [&](float speed, float amplitude, float size) {
			double travelled = (double)speed * seconds;
			if (size > 0.0f) {
				travelled = std::fmod(travelled, (double)size);
			}
			return (float)(travelled + amplitude * swayAmount);
		};
		return vector2(axis(scroll.x, sway.x, loopSize.x), axis(scroll.y, sway.y, loopSize.y));
	}
};

struct TileLayerConfig
{
	int id = -1;
	std::string name;
	int startX = 0;
	int startY = 0;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float parallaxX = 1.0f;
	float parallaxY = 1.0f;
	bool visible = true;
	TileCollisionMode collisionMode = TileCollisionMode::Solid;
	int drawOrder = 0;
	// Tiled's layer tint (ARGB, multiplied into each tile) and opacity.
	uint32_t tintColor = 0xFFFFFFFFu;
	float opacity = 1.0f;
	TileUVMotion uvMotion;
	// Every custom property on the layer, for gameplay that reads them (e.g. open_layer).
	std::vector<TileMapPropertyDescriptor> properties;

	const TileMapPropertyDescriptor* findProperty(const std::string& propertyName) const
	{
		for (const TileMapPropertyDescriptor& property : properties) {
			if (property.name == propertyName) {
				return &property;
			}
		}
		return nullptr;
	}
};
