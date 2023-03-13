// vector2.h

#pragma once

#include "../Maths.h"

typedef struct vector2 :
#ifdef GLM
	public glm::vec2
#elif defined(__D3DX9MATH_H__)
	public D3DXVECTOR2
#endif
{
	vector2(void) {}
#ifdef GLM
	vector2(glm::vec2 v) : glm::vec2(v) {}
#elif defined(__D3DX9MATH_H__)
	vector2(D3DXVECTOR2 v) : D3DXVECTOR2(v) {}
#endif
	vector2(float x, float y) :
#ifdef GLM
		glm::vec2(x, y)
#elif defined(__D3DX9MATH_H__)
		D3DXVECTOR2(x, y)
#endif
	{	}
	vector2& operator=(const vector2& v) { x = v.x; y = v.y; return *this; }

	float norm(void) const {
		return x * x + y * y;
	}

	void normalize(void) {
		x = x / norm();
		y = y / norm();
	}

#ifdef __D3DX9MATH_H__
	operator D3DXVECTOR2() const { return D3DXVECTOR2(this->x, this->y); }
#endif

} Vector2;