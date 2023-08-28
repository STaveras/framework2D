// vector2.h

#pragma once

#include "../Maths.h"

typedef struct vector2 : public glm::vec2
{
	vector2(void) {}
	vector2(const glm::vec2& v) : glm::vec2(v) {}
	vector2(const vector2& v) : glm::vec2(v.x, v.y) {}
	vector2(float x, float y) : glm::vec2(x, y) {}

	vector2& operator=(const vector2& v) { x = v.x; y = v.y; return *this; }
	//float operator*(const vector2& v) { return x * v.x + y * v.y; }

	float norm(void) const {
		return x * x + y * y;
	}

	vector2& normalize(void)
	{
		float normVal = norm();
		x /= normVal;
		y /= normVal;
		return *this;
	}

	float length(void) const {
		return sqrtf(this->norm());
	}

	//operator glm::vec2() const { return glm::vec2(this->x, this->y); }

#ifdef __D3DX9MATH_H__
	operator D3DXVECTOR2() const { return D3DXVECTOR2(this->x, this->y); }
#endif

} Vector2;