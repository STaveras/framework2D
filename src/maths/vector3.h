
#pragma once

#include "../Maths.h"

typedef struct vector3 : 
#ifdef GLM
	public glm::vec3 
#else
	public D3DXVECTOR3
#endif
{
	vector3(void) {}
	vector3(float x, float y, float z) : 
#ifdef GLM
		glm::vec3(x, y, z) 
#else
		D3DXVECTOR3(x,y,z)
#endif
	{	}
	vector3& operator=(const vector3& value) { *this = value; return *this; }

} Vector3;