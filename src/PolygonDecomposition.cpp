#include "PolygonDecomposition.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace PolygonDecomposition {
namespace {
constexpr float kEpsilon = 0.0001f;

float crossZ(const vector2& a, const vector2& b, const vector2& c)
{
	const vector2 ab = b - a;
	const vector2 ac = c - a;
	return (ab.x * ac.y) - (ab.y * ac.x);
}

bool nearlyEqual(float a, float b, float epsilon = kEpsilon)
{
	return std::fabs(a - b) <= epsilon;
}

bool pointsEqual(const vector2& a, const vector2& b, float epsilon = kEpsilon)
{
	return nearlyEqual(a.x, b.x, epsilon) && nearlyEqual(a.y, b.y, epsilon);
}

float signedArea(const std::vector<vector2>& vertices)
{
	if (vertices.size() < 3) {
		return 0.0f;
	}

	double area = 0.0;
	for (size_t i = 0; i < vertices.size(); ++i) {
		const vector2& a = vertices[i];
		const vector2& b = vertices[(i + 1) % vertices.size()];
		area += ((double)a.x * (double)b.y) - ((double)b.x * (double)a.y);
	}
	return (float)(area * 0.5);
}

bool isConvexPolygon(const std::vector<vector2>& vertices)
{
	if (vertices.size() < 3) {
		return false;
	}

	float sign = 0.0f;
	for (size_t i = 0; i < vertices.size(); ++i) {
		const vector2& a = vertices[i];
		const vector2& b = vertices[(i + 1) % vertices.size()];
		const vector2& c = vertices[(i + 2) % vertices.size()];
		const float value = crossZ(a, b, c);
		if (std::fabs(value) <= kEpsilon) {
			continue;
		}
		if (sign == 0.0f) {
			sign = value;
			continue;
		}
		if ((sign > 0.0f && value < 0.0f) || (sign < 0.0f && value > 0.0f)) {
			return false;
		}
	}

	return sign != 0.0f;
}

std::vector<vector2> sanitizeVertices(const std::vector<vector2>& input)
{
	std::vector<vector2> sanitized;
	sanitized.reserve(input.size());

	for (const vector2& vertex : input) {
		if (sanitized.empty() || !pointsEqual(sanitized.back(), vertex)) {
			sanitized.push_back(vertex);
		}
	}

	if (sanitized.size() > 1 && pointsEqual(sanitized.front(), sanitized.back())) {
		sanitized.pop_back();
	}

	if (sanitized.size() < 3) {
		return sanitized;
	}

	bool changed = true;
	int guard = 0;
	while (changed && sanitized.size() >= 3 && guard < 32) {
		changed = false;
		++guard;

		for (size_t i = 0; i < sanitized.size(); ++i) {
			const size_t prev = (i + sanitized.size() - 1) % sanitized.size();
			const size_t next = (i + 1) % sanitized.size();
			if (std::fabs(crossZ(sanitized[prev], sanitized[i], sanitized[next])) <= kEpsilon) {
				sanitized.erase(sanitized.begin() + (long long)i);
				changed = true;
				break;
			}
		}
	}

	return sanitized;
}

int orientation(const vector2& a, const vector2& b, const vector2& c)
{
	const float value = crossZ(a, b, c);
	if (std::fabs(value) <= kEpsilon) {
		return 0;
	}
	return (value > 0.0f) ? 1 : -1;
}

bool onSegment(const vector2& a, const vector2& b, const vector2& point)
{
	return
		point.x <= std::max(a.x, b.x) + kEpsilon &&
		point.x >= std::min(a.x, b.x) - kEpsilon &&
		point.y <= std::max(a.y, b.y) + kEpsilon &&
		point.y >= std::min(a.y, b.y) - kEpsilon;
}

bool segmentsIntersect(const vector2& p1, const vector2& q1, const vector2& p2, const vector2& q2)
{
	const int o1 = orientation(p1, q1, p2);
	const int o2 = orientation(p1, q1, q2);
	const int o3 = orientation(p2, q2, p1);
	const int o4 = orientation(p2, q2, q1);

	if (o1 != o2 && o3 != o4) {
		return true;
	}

	if (o1 == 0 && onSegment(p1, q1, p2)) {
		return true;
	}
	if (o2 == 0 && onSegment(p1, q1, q2)) {
		return true;
	}
	if (o3 == 0 && onSegment(p2, q2, p1)) {
		return true;
	}
	if (o4 == 0 && onSegment(p2, q2, q1)) {
		return true;
	}

	return false;
}

bool isSimplePolygon(const std::vector<vector2>& vertices)
{
	if (vertices.size() < 3) {
		return false;
	}

	for (size_t i = 0; i < vertices.size(); ++i) {
		const size_t iNext = (i + 1) % vertices.size();
		for (size_t j = i + 1; j < vertices.size(); ++j) {
			const size_t jNext = (j + 1) % vertices.size();

			if (i == j || iNext == j || jNext == i) {
				continue;
			}
			if (i == 0 && jNext == 0) {
				continue;
			}

			if (segmentsIntersect(vertices[i], vertices[iNext], vertices[j], vertices[jNext])) {
				return false;
			}
		}
	}

	return true;
}

bool pointInTriangle(const vector2& point, const vector2& a, const vector2& b, const vector2& c)
{
	const float c1 = crossZ(a, b, point);
	const float c2 = crossZ(b, c, point);
	const float c3 = crossZ(c, a, point);

	const bool hasNegative = (c1 < -kEpsilon) || (c2 < -kEpsilon) || (c3 < -kEpsilon);
	const bool hasPositive = (c1 > kEpsilon) || (c2 > kEpsilon) || (c3 > kEpsilon);
	return !(hasNegative && hasPositive);
}

bool triangulateEarClipping(const std::vector<vector2>& vertices, std::vector<std::vector<vector2>>& outTriangles)
{
	outTriangles.clear();
	if (vertices.size() < 3) {
		return false;
	}

	std::vector<size_t> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	int guard = 0;
	const int guardLimit = (int)vertices.size() * (int)vertices.size();

	while (indices.size() > 3 && guard < guardLimit) {
		++guard;
		bool earFound = false;

		for (size_t i = 0; i < indices.size(); ++i) {
			const size_t prevIndex = indices[(i + indices.size() - 1) % indices.size()];
			const size_t currIndex = indices[i];
			const size_t nextIndex = indices[(i + 1) % indices.size()];

			const vector2& prev = vertices[prevIndex];
			const vector2& curr = vertices[currIndex];
			const vector2& next = vertices[nextIndex];

			if (crossZ(prev, curr, next) <= kEpsilon) {
				continue;
			}

			bool hasPointInside = false;
			for (size_t j = 0; j < indices.size(); ++j) {
				const size_t testIndex = indices[j];
				if (testIndex == prevIndex || testIndex == currIndex || testIndex == nextIndex) {
					continue;
				}
				if (pointInTriangle(vertices[testIndex], prev, curr, next)) {
					hasPointInside = true;
					break;
				}
			}

			if (hasPointInside) {
				continue;
			}

			outTriangles.push_back({ prev, curr, next });
			indices.erase(indices.begin() + (long long)i);
			earFound = true;
			break;
		}

		if (!earFound) {
			return false;
		}
	}

	if (indices.size() != 3) {
		return false;
	}

	outTriangles.push_back({
		vertices[indices[0]],
		vertices[indices[1]],
		vertices[indices[2]]
	});

	return true;
}

} // namespace

const char* resultCodeToString(ResultCode code)
{
	switch (code) {
	case ResultCode::Success:
		return "success";
	case ResultCode::InvalidInput:
		return "invalid_input";
	case ResultCode::SelfIntersecting:
		return "self_intersecting";
	case ResultCode::TriangulationFailed:
		return "triangulation_failed";
	default:
		return "unknown";
	}
}

Result decomposeToConvex(const std::vector<vector2>& inputVertices)
{
	Result result;

	std::vector<vector2> vertices = sanitizeVertices(inputVertices);
	if (vertices.size() < 3) {
		result.code = ResultCode::InvalidInput;
		return result;
	}

	if (!isSimplePolygon(vertices)) {
		result.code = ResultCode::SelfIntersecting;
		return result;
	}

	const float area = signedArea(vertices);
	if (std::fabs(area) <= kEpsilon) {
		result.code = ResultCode::InvalidInput;
		return result;
	}

	if (area < 0.0f) {
		std::reverse(vertices.begin(), vertices.end());
	}

	if (isConvexPolygon(vertices)) {
		result.code = ResultCode::Success;
		result.wasConcave = false;
		result.pieces.push_back(vertices);
		return result;
	}

	result.wasConcave = true;
	if (!triangulateEarClipping(vertices, result.pieces)) {
		result.code = ResultCode::TriangulationFailed;
		result.pieces.clear();
		return result;
	}

	result.code = ResultCode::Success;
	return result;
}

} // namespace PolygonDecomposition
