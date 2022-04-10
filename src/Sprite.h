#pragma once
#include "Image.h"
#include "Collidable.h"
class Sprite : public Image // Should probably just contain an image
{
  Collidable* _CollideInfo;

public:
  Sprite(void) :Image(), _CollideInfo(NULL) {}
  Sprite(const char* filename, color clearKeyColor = 0, RECT* srcRect = NULL) :Image(filename, clearKeyColor, srcRect) {}

  Collidable* getCollisionInfo(void) const { return _CollideInfo; }
  void setCollisionInfo(Collidable* collisionObject) { _CollideInfo = collisionObject; }

  //using Image::Load;

  Sprite* Load(const char* filePath);
  void Save(const char* filePath);
};