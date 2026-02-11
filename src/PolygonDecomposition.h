#pragma once

#include "Types.h"

#include <vector>

namespace PolygonDecomposition {

enum class ResultCode
{
	Success,
	InvalidInput,
	SelfIntersecting,
	TriangulationFailed
};

struct Result
{
	ResultCode code = ResultCode::InvalidInput;
	bool wasConcave = false;
	std::vector<std::vector<vector2>> pieces;
};

Result decomposeToConvex(const std::vector<vector2>& inputVertices);
const char* resultCodeToString(ResultCode code);

} // namespace PolygonDecomposition
