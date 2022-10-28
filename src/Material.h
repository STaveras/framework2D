
#pragma once

#include "color.h"
#include "Factory.h"

typedef struct material 
{
    color _color;
    // texture
}Material;

typedef Material* materalId;
typedef class Factory<Material> MaterialManager;