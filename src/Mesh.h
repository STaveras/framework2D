#pragma once

#include "Types.h"
#include "Vertex.h"

#include <vector>

class Mesh //: public entity
{
    std::vector<Vertex> vertices;

public:
    Mesh(const char* fromFile);
};