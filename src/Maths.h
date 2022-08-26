#ifndef _MATHS_H_
#define _MATHS_H_

#pragma once // kinda redundant but ay

#include "Types.h"

#define vector2 glm::vec2
#include "maths/vector3.h"
#define vector4 glm::vec4
#define matrix4x4 glm::mat4

// Write a wrapper/your own math libraries for vectors and matrices and allow them to dump their data to DirectX/OpenGL

// bool isOdd( int val ) { return (val & 0x01) != 0; }
#define IS_ODD(val) (val & 0x01) != 0

#endif