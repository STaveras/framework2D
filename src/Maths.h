// Maths.h
#pragma once

#ifndef _MATHS_H_
#define _MATHS_H_

#ifdef __D3DX9MATH_H__
#include <d3dx9math.h>
#endif
#include <glm/glm.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "maths/Vector2.h"
#include "maths/Vector3.h"

#if !defined(_WIN32) || defined (GLM)// We're going to have to check this at run time if we want to swap between DX and VK on the fly

#define vector4 glm::vec4
#define matrix4x4 glm::mat4

#endif

// Write a wrapper/your own math libraries for vectors and matrices and allow them to dump their data to DirectX/OpenGL

static float dot(vector2 a, vector2 b) {
	return a.x * b.x + a.y * b.y;
}

// bool isOdd( int val ) { return (val & 0x01) != 0; }
#define IS_ODD(val) (val & 0x01) != 0

#endif // _MATHS_H_