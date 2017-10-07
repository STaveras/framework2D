// Tile.h
#pragma once
#include "GameObject.h"
class Tile : public GameObject
{
    friend class TileSet;

public:
    void SetSprite(Sprite* sprite);
    void SetAnimation(Animation* animation);
};

class TileSet
{
    std::vector<Tile*> _Tiles;

public:
    void DefineTile(Renderable* renderable, Collidable* collidable = NULL);
    Tile* operator=(unsigned int i) { return _Tiles[i]; }
};

class TileGrid
{
    TileSet* _Tiles;
    Tile*** _Grid;

public:
    explicit TileGrid(unsigned horizontal, unsigned vertical)
    {
        _Grid = new Tile**[horizontal];
        
        for (unsigned x = 0; x < horizontal; x++)
            _Grid[x] = new Tile*[vertical];
    }

    void Map(Tile* t, unsigned x, unsigned y)
    {

    }

    void Load(const char* path);
};