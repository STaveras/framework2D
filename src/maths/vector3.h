
#pragma once

#include "../Maths.h"

typedef struct vector3 : public glm::vec3 {

        vector3(void) {}
        vector3(float x, float y, float z):glm::vec3(x, y, z) {}
        vector3& operator=(const vector3& value) { *this = value; return *this; }

        // TODO: Write D3DX overrides to directly use these instead of the built-in D3DX

} Vector3;