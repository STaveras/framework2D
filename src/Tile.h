#pragma once

#include "GameObject.h"

#include "TileSet.h"
#include "TileLayerConfig.h"
#include "Square.h"
#include "CollidableGroup.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Just to get something on the screen...
class Tile : public GameObject
{
   int _tileIndex = -1; // How far in the tileSheet this block is
   unsigned int _flipFlags = 0; // TileSet::kFlip* bits from the map's gid

   TileSet* _tileSet = NULL;
   TileCollisionMode _layerCollisionMode = TileCollisionMode::Solid;
   bool _layerVisible = true;
   std::string _layerName;
	TileSet::UVLoop _uvLoop; // Copied from the tileset whenever the tile index is set
	TileUVMotion _uvMotion; // From the tile's layer
	vector2 _uvLoopPhase; // Where in its loop the tile's top-left sits before any motion
	double _uvLoopSeconds = 0.0; // Level clock the art was last moved to

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
      GameObject::invalidateSpatialIndex(this);
   }

   TileCollisionMode getLayerCollisionMode(void) const {
      return _layerCollisionMode;
   }

   void setLayerVisible(bool visible) {
      _layerVisible = visible;
      if (GameObjectState* state = this->getState()) {
         if (Renderable* renderable = state->getRenderable()) {
            renderable->setVisibility(visible);
         }
      }
   }

   bool isLayerVisible(void) const {
      return _layerVisible;
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
      const TileSet::TileInfo* info = _tileSet ? _tileSet->findTileInfo(_tileIndex) : NULL;
      return info ? info->_typeName : std::string();
   }

   unsigned int getFlipFlags(void) const {
      return _flipFlags;
   }

	// The layer's art motion. It moves the tile only along axes its tileset loops it on.
	void setUVMotion(const TileUVMotion& motion) {
		_uvMotion = motion;
		applyUVLoop();
	}

	// True when the layer moves this tile's art; the level advances it each frame.
	bool hasUVMotion(void) const {
		return movesUVLoopX() || movesUVLoopY();
	}

	// Continues the loop from the tiles to the left and above (same layer) when they move
	// through the same strip, so a run of moving tiles reads as one surface. The run's
	// first tile keeps the spot it was painted with. Call in map order: rows top to
	// bottom, each row left to right.
	void linkUVLoop(const Tile* left, const Tile* above)
	{
		_uvLoopPhase = paintedUVLoopPhase();
		const float size = _tileSet ? _tileSet->getTileSize() : 0.0f;
		if (movesUVLoopX() && left && left->movesUVLoopX() && left->_tileSet == _tileSet &&
			left->_uvLoop.left == _uvLoop.left && left->_uvLoop.width == _uvLoop.width) {
			_uvLoopPhase.x = left->_uvLoopPhase.x + size;
		}
		if (movesUVLoopY() && above && above->movesUVLoopY() && above->_tileSet == _tileSet &&
			above->_uvLoop.top == _uvLoop.top && above->_uvLoop.height == _uvLoop.height) {
			_uvLoopPhase.y = above->_uvLoopPhase.y + size;
		}
		applyUVLoop();
	}

	// Moves the art to where the layer's motion puts it at this time on the level clock.
	void updateUVLoop(double seconds)
	{
		_uvLoopSeconds = seconds;
		applyUVLoop();
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

	bool movesUVLoopX(void) const {
		return _uvLoop.width > 0 && _uvMotion.movesX();
	}

	bool movesUVLoopY(void) const {
		return _uvLoop.height > 0 && _uvMotion.movesY();
	}

	// Where in its loop the tile's painted art sits.
	vector2 paintedUVLoopPhase(void) const
	{
		if (!_tileSet || _tileIndex < 0) {
			return vector2(0.0f, 0.0f);
		}
		const int columns = (int)_tileSet->getTileCounts().x;
		const float size = _tileSet->getTileSize();
		return vector2(
			(float)(_tileIndex % columns) * size - (float)_uvLoop.left,
			(float)(_tileIndex / columns) * size - (float)_uvLoop.top);
	}

	// Only axes the layer moves loop; the rest draw the tile as painted.
	void applyUVLoop(void) const
	{
		GameObjectState* state = this->getState();
		Image* image = state ? (Image*)state->getRenderable() : nullptr;
		if (!image) {
			return;
		}

		RECT region = { 0, 0, 0, 0 };
		vector2 position;
		const vector2 moved = _uvMotion.displacementAt(_uvLoopSeconds, vector2((float)_uvLoop.width, (float)_uvLoop.height));
		if (movesUVLoopX()) {
			region.left = _uvLoop.left;
			region.right = _uvLoop.left + _uvLoop.width;
			position.x = _uvLoopPhase.x - moved.x;
		}
		if (movesUVLoopY()) {
			region.top = _uvLoop.top;
			region.bottom = _uvLoop.top + _uvLoop.height;
			position.y = _uvLoopPhase.y - moved.y;
		}
		image->setUVLoop(region, position);
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
         _uvLoop = TileSet::UVLoop();
         state->setCollidable(NULL);
         // A cleared tile (e.g. a collected pickup) must stop drawing too.
         if (Renderable* renderable = state->getRenderable()) {
            renderable->setVisibility(false);
         }
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

               // Reuse the tile's image when its index changes at runtime (e.g. a chest
               // opening); only the source rect and sheet need updating.
               Image* tileImage = (Image*)state->getRenderable();
               if (!tileImage) {
                  tileImage = _tileImages.create();
               }
               tileImage->setSrcRect(tileRect);
               tileImage->setTexture(_tileSet->getTileSheet());
               tileImage->setVisibility(_layerVisible);

               TileSet::TileInfo tileInfo = _tileSet->getTileInfo(tileIndex);
               _uvLoop = tileInfo._uvLoop;
               _uvLoopPhase = paintedUVLoopPhase();

               applyFlipToImage(tileImage);
               state->setRenderable(tileImage);
               applyUVLoop();

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
      GameObject::invalidateSpatialIndex(this);
   }

   void setTileSet(TileSet* tileSet) {

      _tileSet = tileSet;

      if (_tileSet) {
         this->setTileIndex(_tileIndex);
      }
      else {
         GameObject::invalidateSpatialIndex(this);
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
