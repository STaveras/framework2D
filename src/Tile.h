#pragma once

#include "GameObject.h"

#include "TileSet.h"
#include "TileLayerConfig.h"
#include "Square.h"
#include "CollidableGroup.h"

#include <algorithm>
#include <string>
#include <vector>

// Just to get something on the screen...
class Tile : public GameObject
{
   int _tileIndex = -1; // How far in the tileSheet this block is
   unsigned int _flipFlags = 0; // TileSet::kFlip* bits from the map's gid

   TileSet* _tileSet = NULL;
   TileCollisionMode _layerCollisionMode = TileCollisionMode::Solid;
   std::string _layerName;

   // TODO: Move this to TileSet
   Factory<Image> _tileImages;

public:
   Tile(void) : GameObject(GAME_OBJ_TILE) {
      this->setCollisionPredicate([](const GameObject& other) {
         return other.getType() != GAME_OBJ_TILE;
      });
      this->setStatic(true);
      this->addState("");
      this->start();
   }

   // tileIndex which tile to use, starting from 0, left-to-right, top-to-bottom
   explicit Tile(int tileIndex, TileSet* tileSet) :
      GameObject(GAME_OBJ_TILE),
      _tileSet(tileSet) {

      this->setStatic(true);

      this->setCollisionPredicate([](const GameObject& other) {
         return other.getType() != GAME_OBJ_TILE;
      });

      if (_tileSet) {
         this->addState(_tileSet->getTileInfo(tileIndex)._typeName.c_str()); // Look up the tileType in the tile set info
         this->start();
         this->setTileIndex(tileIndex);
      }
   }

   ~Tile(void) {
      _tileImages.clear();
   }

   TileSet* getTileSet(void) const {
      return _tileSet;
   }

   void setLayerCollisionMode(TileCollisionMode mode) {
      _layerCollisionMode = mode;
      applyLayerSurfaceTraitsToCurrentState();
      this->updateComponents();
      GameObject::invalidateSpatialIndex();
   }

   TileCollisionMode getLayerCollisionMode(void) const {
      return _layerCollisionMode;
   }

   void setLayerName(const std::string& layerName) {
      _layerName = layerName;
   }

   const std::string& getLayerName(void) const {
      return _layerName;
   }

   bool isOneWay(void) const {
      return _layerCollisionMode == TileCollisionMode::OneWay;
   }

   bool isNonCollidingLayer(void) const {
      return _layerCollisionMode == TileCollisionMode::None;
   }

   bool isCollidableLayer(void) const {
      return _layerCollisionMode != TileCollisionMode::None;
   }

   bool shouldCollideWith(const GameObject& other) const override
   {
      if (isNonCollidingLayer()) {
         return false;
      }

      if (other.getType() == GAME_OBJ_TILE) {
         return false;
      }

      return GameObject::shouldCollideWith(other);
   }

   int getTileIndex(void) const {
      return _tileIndex;
   }

   TileSet::TileInfo getTileInfo(void) const {
      if (_tileSet && _tileIndex >= 0) {
         return _tileSet->getTileInfo(_tileIndex);
      }
      return TileSet::TileInfo();
   }

   std::string getTileType(void) const {
      return _tileSet->getTileInfo(_tileIndex)._typeName;
   }

   unsigned int getFlipFlags(void) const {
      return _flipFlags;
   }

private:
   // Reproduces the flip flags with the renderer's sprite transform. The renderer draws
   // position + R(rotation) * S(scale) * (corner - center); the flips are an affine map
   // F(p) = A*p + t of the tile box onto itself, so we need R*S = A and center = -A^T*t.
   void applyFlipToImage(Image* image) const
   {
      if (!image || !_tileSet) {
         return;
      }

      if (_flipFlags == 0) {
         image->setCenter(vector2(0.0f, 0.0f));
         image->setScale(vector2(1.0f, 1.0f));
         image->setRotation(0.0f);
         return;
      }

      const float size = _tileSet->getTileSize();
      const vector2 t = TileSet::flipTilePoint(vector2(0.0f, 0.0f), size, _flipFlags);
      const vector2 ax = (TileSet::flipTilePoint(vector2(size, 0.0f), size, _flipFlags) - t) * (1.0f / size);
      const vector2 ay = (TileSet::flipTilePoint(vector2(0.0f, size), size, _flipFlags) - t) * (1.0f / size);

      if (_flipFlags & TileSet::kFlipDiagonal) {
         // A = [0 ay.x; ax.y 0] = R(90deg) * S(ax.y, -ay.x)
         image->setRotation(1.57079632679f);
         image->setScale(vector2(ax.y, -ay.x));
      }
      else {
         image->setRotation(0.0f);
         image->setScale(vector2(ax.x, ay.y));
      }
      image->setCenter(vector2(-(ax.x * t.x + ax.y * t.y), -(ay.x * t.x + ay.y * t.y)));
   }

   SurfaceTraits2D buildSurfaceTraitsForLayer(void) const
   {
      SurfaceTraits2D traits;
      traits.upNormal = vector2(0.0f, -1.0f);
      traits.oneWayEpsilon = 0.5f;
      traits.maxStepHeight = 8.0f;

      switch (_layerCollisionMode) {
      case TileCollisionMode::Solid:
         traits.flags = SurfaceFlags::Solid | SurfaceFlags::Walkable | SurfaceFlags::StepCandidate;
         break;
      case TileCollisionMode::OneWay:
         traits.flags = SurfaceFlags::Solid | SurfaceFlags::Walkable | SurfaceFlags::OneWay;
         break;
      case TileCollisionMode::None:
      default:
         traits.flags = SurfaceFlags::None;
         break;
      }

      return traits;
   }

   static void applySurfaceTraitsRecursive(Collidable* collidable, const SurfaceTraits2D& traits)
   {
      if (!collidable) {
         return;
      }

      collidable->setSurfaceTraits(traits);
      if (collidable->getType() != COL_OBJ_GROUP) {
         return;
      }

      CollidableGroup* group = (CollidableGroup*)collidable;
      for (Collidable* member : *group) {
         applySurfaceTraitsRecursive(member, traits);
      }
   }

   void applyLayerSurfaceTraitsToCurrentState(void)
   {
      GameObjectState* state = this->getState();
      if (!state) {
         return;
      }

      Collidable* collidable = state->getCollidable();
      if (!collidable) {
         return;
      }

      const SurfaceTraits2D traits = buildSurfaceTraitsForLayer();
      applySurfaceTraitsRecursive(collidable, traits);
   }

public:

   void setTileIndex(int tileIndex, unsigned int flipFlags)
   {
      _flipFlags = flipFlags & TileSet::kFlipMask;
      setTileIndex(tileIndex);
   }

   void setTileIndex(int tileIndex) 
   {
      _tileIndex = tileIndex;

      GameObjectState* state = this->getState();
      if (state && _tileIndex < 0) {
         state->setCollidable(NULL);
      }

      if (_tileSet && _tileIndex >= 0) {

         if (_tileIndex < _tileSet->getTileCounts().x * _tileSet->getTileCounts().y) {

            if (state) {

               int xPosition = (int)((_tileIndex % (int)_tileSet->getTileCounts().x) * _tileSet->getTileSize());
               int yPosition = (int)((_tileIndex / (int)_tileSet->getTileCounts().x) * _tileSet->getTileSize());

               int width = (int)(xPosition + _tileSet->getTileSize());
               int height = (int)(yPosition + _tileSet->getTileSize());

               RECT tileRect{
                  xPosition, yPosition, width, height
               };

               Image* tileImage = (Image*)state->getRenderable();

               if (tileImage) {
                  _tileImages.destroy(tileImage);
               }
               else {
                  tileImage = _tileImages.create();
                  tileImage->setSrcRect(tileRect);
                  tileImage->setTexture(_tileSet->getTileSheet());
               }

               applyFlipToImage(tileImage);
               state->setRenderable(tileImage);

               TileSet::TileInfo tileInfo = _tileSet->getTileInfo(tileIndex);

               if (tileInfo._typeName != "") {
                  state->setName(tileInfo._typeName.c_str());
               }

               if (tileInfo._collisionInfo != NULL) {
                  state->setCollidable(_tileSet->getCollision(tileIndex, _flipFlags));
                  applyLayerSurfaceTraitsToCurrentState();
               }
               else {
                  state->setCollidable(NULL);
               }
               // Originally we created new Collision objects here... But now, we entirely rely on the TileSet for these objects
            }
         }
      }

      this->updateComponents();
      GameObject::invalidateSpatialIndex();
   }

   void setTileSet(TileSet* tileSet) {

      _tileSet = tileSet;

      if (_tileSet) {
         this->setTileIndex(_tileIndex);
      }
      else {
         GameObject::invalidateSpatialIndex();
      }
   }

#ifdef _DEBUG
   void update(float time) override 
   {
      GameObject::update(time);

      if (Debug::dbgTiles) 
      {
         char buffer[128]{ 0 };
         sprintf_s(buffer, 128, "pos{%f, %f}\n", this->getPosition().x, this->getPosition().y);
         DEBUG_MSG(buffer);

         if (Collidable* collidable = this->getCollidable()) {
            sprintf_s(buffer, 128, "cpos{%f, %f}\n", this->getCollidable()->getPosition().x, this->getCollidable()->getPosition().y);
            DEBUG_MSG(buffer);
         }

         DEBUG_MSG("\n");
      }
   }
#endif
};
