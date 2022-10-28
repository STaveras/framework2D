
#pragma once

#include "Types.h"
// #include "Material.h"

typedef struct vertex 
{
    vector3 position;
    vector3 color;
    
    vertex(float x, float y, float z, float r, float g, float b):position(x,y,z), color(r,g,b) { }
    vertex(float x, float y, float z, Color col):position(x,y,z), color(col) { }
}Vertex;