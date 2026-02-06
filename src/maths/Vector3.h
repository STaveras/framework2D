
#pragma once

#include "../Maths.h"

typedef struct vector3 : public glm::vec3 
{
	vector3(void) {}
	vector3(float x, float y, float z) : glm::vec3(x, y, z) {}

#ifdef __D3DX9MATH_H__
	operator D3DXVECTOR3() const { return D3DXVECTOR3(this->x, this->y, this->z); }
#endif
} Vector3;