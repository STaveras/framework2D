// vector2.h

#pragma once

#include "../Maths.h"

typedef struct vector2 : public glm::vec2
{
    vector2() : glm::vec2(0.0f, 0.0f) {}
    vector2(const glm::vec2& v) : glm::vec2(v) {}
    vector2(const vector2& v) : glm::vec2(v.x, v.y) {}
    vector2(float x, float y) : glm::vec2(x, y) {}

    vector2& operator=(const vector2& v) { x = v.x; y = v.y; return *this; }

    float norm() const { return x * x + y * y; }

    vector2& normalize() {
        float normVal = length();
        if (normVal != 0.0f) {
            x /= normVal;
            y /= normVal;
        }
        return *this;
    }

    float length() const { return sqrtf(norm()); }

#ifdef GLM_VERSION
    operator glm::vec2() const { return glm::vec2(x, y); }
#endif

#ifdef __D3DX9MATH_H__
    operator D3DXVECTOR2() const { return D3DXVECTOR2(x, y); }
#endif
}Vector2;

//// GLM interoperability
//template <typename T>
//inline T operator*(const vector2& v, float scalar) {
//    return T(v.x * scalar, v.y * scalar);
//}
//
//template <typename T>
//inline T operator*(float scalar, const vector2& v) {
//    return T(v.x * scalar, v.y * scalar);
//}
//
//template <typename T>
//inline T operator+(const vector2& a, const T& b) {
//    return T(a.x + b.x, a.y + b.y);
//}
//
//template <typename T>
//inline T operator-(const vector2& a, const T& b) {
//    return T(a.x - b.x, a.y - b.y);
//}