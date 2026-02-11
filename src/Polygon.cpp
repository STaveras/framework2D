#include "Polygon.h"

#include "CollidableGroup.h"
#include "Square.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace {
constexpr float kEpsilon = 0.0001f;

float crossZ(const vector2& a, const vector2& b, const vector2& c)
{
	const vector2 ab = b - a;
	const vector2 ac = c - a;
	return (ab.x * ac.y) - (ab.y * ac.x);
}

void buildSquareVertices(const Square* square, std::vector<vector2>& outVertices)
{
	outVertices.clear();
	if (!square) {
		return;
	}

	const vector2 min = square->getMin();
	const vector2 max = square->getMax();
	outVertices.push_back(vector2(min.x, min.y));
	outVertices.push_back(vector2(max.x, min.y));
	outVertices.push_back(vector2(max.x, max.y));
	outVertices.push_back(vector2(min.x, max.y));
}

vector2 computeCentroid(const std::vector<vector2>& vertices)
{
	if (vertices.empty()) {
		return vector2(0.0f, 0.0f);
	}

	vector2 centroid(0.0f, 0.0f);
	for (const vector2& vertex : vertices) {
		centroid.x += vertex.x;
		centroid.y += vertex.y;
	}
	return vector2(
		centroid.x / (float)vertices.size(),
		centroid.y / (float)vertices.size());
}

bool projectOnAxis(const std::vector<vector2>& vertices, const vector2& axis, float& outMin, float& outMax)
{
	if (vertices.empty()) {
		return false;
	}

	outMin = dot(vertices[0], axis);
	outMax = outMin;
	for (size_t i = 1; i < vertices.size(); ++i) {
		const float value = dot(vertices[i], axis);
		if (value < outMin) {
			outMin = value;
		}
		if (value > outMax) {
			outMax = value;
		}
	}
	return true;
}

bool satOverlap(
	const std::vector<vector2>& a,
	const std::vector<vector2>& b,
	std::optional<vector2>& outNormal,
	std::optional<float>& outPenetrationDepth)
{
	if (a.size() < 3 || b.size() < 3) {
		return false;
	}

	float minOverlap = std::numeric_limits<float>::max();
	vector2 bestAxis(0.0f, 0.0f);
	bool hasAxis = false;

	auto testAxes = [&](const std::vector<vector2>& vertices) -> bool {
		for (size_t i = 0; i < vertices.size(); ++i) {
			const vector2& p0 = vertices[i];
			const vector2& p1 = vertices[(i + 1) % vertices.size()];
			vector2 edge = p1 - p0;
			if (edge.norm() <= kEpsilon) {
				continue;
			}

			vector2 axis(-edge.y, edge.x);
			axis.normalize();

			float aMin = 0.0f;
			float aMax = 0.0f;
			float bMin = 0.0f;
			float bMax = 0.0f;
			if (!projectOnAxis(a, axis, aMin, aMax) || !projectOnAxis(b, axis, bMin, bMax)) {
				return false;
			}

			const float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
			if (overlap <= 0.0f) {
				return false;
			}

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
				hasAxis = true;
			}
		}
		return true;
	};

	if (!testAxes(a) || !testAxes(b) || !hasAxis) {
		return false;
	}

	vector2 centerA = computeCentroid(a);
	vector2 centerB = computeCentroid(b);
	vector2 toB = centerB - centerA;
	if (dot(toB, bestAxis) < 0.0f) {
		bestAxis = vector2(-bestAxis.x, -bestAxis.y);
	}

	outNormal = bestAxis;
	outPenetrationDepth = minOverlap;
	return true;
}

bool pointInConvexPolygon(const std::vector<vector2>& vertices, const vector2& point)
{
	if (vertices.size() < 3) {
		return false;
	}

	float sign = 0.0f;
	for (size_t i = 0; i < vertices.size(); ++i) {
		const vector2& p0 = vertices[i];
		const vector2& p1 = vertices[(i + 1) % vertices.size()];
		const float c = crossZ(p0, p1, point);
		if (std::abs(c) <= kEpsilon) {
			continue;
		}

		if (sign == 0.0f) {
			sign = c;
		}
		else if ((sign > 0.0f && c < 0.0f) || (sign < 0.0f && c > 0.0f)) {
			return false;
		}
	}

	return true;
}
} // namespace

PolygonCollider::PolygonCollider(const PolygonCollider& rhs)
	: Collidable(rhs),
	  _localVertices(rhs._localVertices),
	  _worldVertices(rhs._worldVertices),
	  _cachedMin(rhs._cachedMin),
	  _cachedMax(rhs._cachedMax),
	  _cacheDirty(true),
	  _isConvex(rhs._isConvex),
	  _isValid(rhs._isValid)
{
}

void PolygonCollider::markDirty(void) const
{
	_cacheDirty = true;
}

void PolygonCollider::rebuildValidation(void)
{
	_isValid = false;
	_isConvex = false;

	if (_localVertices.size() < 3) {
		return;
	}

	// Check for non-zero cross product (non-degenerate polygon)
	float sign = 0.0f;
	bool hasNonZeroCross = false;
	for (size_t i = 0; i < _localVertices.size(); ++i) {
		const vector2& a = _localVertices[i];
		const vector2& b = _localVertices[(i + 1) % _localVertices.size()];
		const vector2& c = _localVertices[(i + 2) % _localVertices.size()];
		const float cZ = crossZ(a, b, c);
		if (std::abs(cZ) <= kEpsilon) {
			continue;
		}

		hasNonZeroCross = true;
		if (sign == 0.0f) {
			sign = cZ;
			continue;
		}

		// If cross products have different signs, polygon is concave, but still valid for walking
		// We just can't use SAT collision detection on it
		if ((sign > 0.0f && cZ < 0.0f) || (sign < 0.0f && cZ > 0.0f)) {
			// Mark as valid for surface sampling, but not convex for SAT collision
			_isValid = true;
			_isConvex = false;
			return;
		}
	}

	if (!hasNonZeroCross) {
		return;
	}

	// All cross products had the same sign - polygon is convex
	_isConvex = true;
	_isValid = true;
}

void PolygonCollider::rebuildCache(void) const
{
	if (!_cacheDirty) {
		return;
	}

	_worldVertices.clear();
	if (_localVertices.empty()) {
		_cachedMin = vector2(0.0f, 0.0f);
		_cachedMax = vector2(0.0f, 0.0f);
		_cacheDirty = false;
		return;
	}

	_worldVertices.reserve(_localVertices.size());
	for (const vector2& localVertex : _localVertices) {
		_worldVertices.push_back(this->getPosition() + localVertex);
	}

	_cachedMin = _worldVertices[0];
	_cachedMax = _worldVertices[0];
	for (size_t i = 1; i < _worldVertices.size(); ++i) {
		const vector2& worldVertex = _worldVertices[i];
		_cachedMin.x = std::min(_cachedMin.x, worldVertex.x);
		_cachedMin.y = std::min(_cachedMin.y, worldVertex.y);
		_cachedMax.x = std::max(_cachedMax.x, worldVertex.x);
		_cachedMax.y = std::max(_cachedMax.y, worldVertex.y);
	}

	_cacheDirty = false;
}

const std::vector<vector2>& PolygonCollider::getWorldVertices(void) const
{
	rebuildCache();
	return _worldVertices;
}

vector2 PolygonCollider::getMin(void) const
{
	rebuildCache();
	return _cachedMin;
}

vector2 PolygonCollider::getMax(void) const
{
	rebuildCache();
	return _cachedMax;
}

void PolygonCollider::setLocalVertices(const std::vector<vector2>& vertices)
{
	_localVertices = vertices;
	rebuildValidation();
	markDirty();
}

void PolygonCollider::setPosition(vector2 position)
{
	Positionable::setPosition(position);
	markDirty();
}

void PolygonCollider::setPosition(float x, float y)
{
	Positionable::setPosition(x, y);
	markDirty();
}

bool PolygonCollider::collidesWith(vector2 point)
{
	if (!_isValid || !_isConvex) {
		return false;
	}

	const vector2 min = getMin();
	const vector2 max = getMax();
	if (point.x < min.x || point.x > max.x || point.y < min.y || point.y > max.y) {
		return false;
	}

	return pointInConvexPolygon(getWorldVertices(), point);
}

bool PolygonCollider::collidesWith(const Collidable* collidable)
{
	if (!_isValid || !_isConvex || !collidable || !collidable->isActive()) {
		return false;
	}

	const vector2 min = getMin();
	const vector2 max = getMax();

	switch (collidable->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)collidable;
		if (!square) {
			return false;
		}

		vector2 squareMin = square->getMin();
		vector2 squareMax = square->getMax();
		if (max.x < squareMin.x || min.x > squareMax.x || max.y < squareMin.y || min.y > squareMax.y) {
			return false;
		}

		std::vector<vector2> squareVertices;
		buildSquareVertices(square, squareVertices);
		std::optional<vector2> normal;
		std::optional<float> penetrationDepth;
		return satOverlap(getWorldVertices(), squareVertices, normal, penetrationDepth);
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon || !polygon->isValid() || !polygon->isConvex()) {
			return false;
		}

		vector2 otherMin = polygon->getMin();
		vector2 otherMax = polygon->getMax();
		if (max.x < otherMin.x || min.x > otherMax.x || max.y < otherMin.y || min.y > otherMax.y) {
			return false;
		}

		std::optional<vector2> normal;
		std::optional<float> penetrationDepth;
		return satOverlap(getWorldVertices(), polygon->getWorldVertices(), normal, penetrationDepth);
	}
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group) {
			return false;
		}

		for (const Collidable* member : *group) {
			if (member && this->collidesWith(member)) {
				return true;
			}
		}
		return false;
	}
	default:
		return this->collidesWith(collidable->getPosition());
	}
}

bool PolygonCollider::findTopSurfaceYAtX(float x, float& outY) const
{
	// Allow sampling even for non-convex but valid polygons. Non-convex
	// polygons can still provide a meaningful top surface at a given X,
	// so only reject entirely invalid/degenerate polygons here.
	if (!_isValid) {
		return false;
	}

	const std::vector<vector2>& vertices = getWorldVertices();
	if (vertices.size() < 3) {
		return false;
	}

	std::vector<float> intersections;
	intersections.reserve(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		const vector2& p0 = vertices[i];
		const vector2& p1 = vertices[(i + 1) % vertices.size()];

		if (std::abs(p1.x - p0.x) <= kEpsilon) {
			if (std::abs(x - p0.x) <= kEpsilon) {
				intersections.push_back(p0.y);
				intersections.push_back(p1.y);
			}
			continue;
		}

		const float minX = std::min(p0.x, p1.x) - kEpsilon;
		const float maxX = std::max(p0.x, p1.x) + kEpsilon;
		if (x < minX || x > maxX) {
			continue;
		}

		const float t = (x - p0.x) / (p1.x - p0.x);
		if (t < -kEpsilon || t > 1.0f + kEpsilon) {
			continue;
		}

		const float y = p0.y + ((p1.y - p0.y) * t);
		intersections.push_back(y);
	}

	if (intersections.empty()) {
		return false;
	}

	std::sort(intersections.begin(), intersections.end());
	outY = intersections.front();
	return true;
}
