#pragma once

#include <string>

// TODO: Make these state keys that correspond to states that are built-in to tiles
enum class TileCollisionMode
{
	None,
	Solid,
	OneWay
};

struct TileLayerConfig
{
	int id = -1;
	std::string name;
	int startX = 0;
	int startY = 0;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	TileCollisionMode collisionMode = TileCollisionMode::Solid;
	int drawOrder = 0;
};
