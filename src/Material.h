
#pragma once

#include "Types.h"
#include "Factory.h"

typedef struct material 
{
    framework::color _color;
    // texture

    // std::list<shader*> _shaders;
}Material;

typedef Material* materalId;
typedef class Factory<Material> MaterialManager;