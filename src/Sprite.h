#pragma once
#include "Image.h"
#include "Collidable.h"
class Sprite : public Image
{
    Collidable* _CollideInfo;

public:
	Sprite(void):Image(),_CollideInfo(NULL){}
	Sprite(const char* filename, color clearKeyColor = 0, rect* srcRect = NULL);

	using Image::Load;
	Sprite* Load(const char* filePath);
	void Save(const char* filePath);
};