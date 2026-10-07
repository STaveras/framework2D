// CollisionDebugDraw.h
// The collision debug overlay as world-space line segments, so every backend
// draws the same colliders, sweeps, anchors and contact normals. A backend
// supplies line(start, end, r, g, b, a) and draws each segment as it comes.

#pragma once

#include "CollisionSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace CollisionDebugDraw
{
	template <typename LineFn>
	void cross(LineFn& line, const vector2& position, float radius, float r, float g, float b, float a)
	{
		line(vector2(position.x - radius, position.y), vector2(position.x + radius, position.y), r, g, b, a);
		line(vector2(position.x, position.y - radius), vector2(position.x, position.y + radius), r, g, b, a);
	}

	template <typename LineFn>
	void rect(LineFn& line, const vector2& min, const vector2& max, float r, float g, float b, float a)
	{
		line(vector2(min.x, min.y), vector2(max.x, min.y), r, g, b, a);
		line(vector2(max.x, min.y), vector2(max.x, max.y), r, g, b, a);
		line(vector2(max.x, max.y), vector2(min.x, max.y), r, g, b, a);
		line(vector2(min.x, max.y), vector2(min.x, min.y), r, g, b, a);
	}

	template <typename LineFn>
	void loop(LineFn& line, const std::vector<vector2>& vertices, float r, float g, float b, float a)
	{
		if (vertices.size() < 3) {
			return;
		}
		for (size_t i = 0; i < vertices.size(); ++i) {
			line(vertices[i], vertices[(i + 1) % vertices.size()], r, g, b, a);
		}
	}

	template <typename LineFn>
	void draw(const CollisionSystem& collisionSystem, LineFn&& line)
	{
		for (const CollisionDebugShape& shape : collisionSystem.getDebugShapes()) {
			if (!shape.collidable || (!shape.hasBounds && !shape.hasPolygon)) {
				continue;
			}

			float r = 0.2f, g = 0.95f, b = 0.25f, a = 0.9f;
			if (shape.hasPolygon) {
				r = 0.3f; g = 0.8f; b = 1.0f;
			}

			if (!shape.collidableActive) {
				r = 0.45f; g = 0.45f; b = 0.45f;
			}
			else if (shape.hasContact) {
				switch (shape.phase) {
				case CollisionPhase::Enter: r = 1.0f; g = 0.25f; b = 0.25f; break;
				case CollisionPhase::Stay:  r = 1.0f; g = 0.9f;  b = 0.15f; break;
				case CollisionPhase::Exit:  r = 1.0f; g = 0.35f; b = 1.0f;  break;
				}
			}

			// For polygon/group colliders, the SAT loop is the source of truth.
			// Drawing both polygon loops and AABB boxes makes it look like there are
			// extra square collision volumes around ramps.
			if (shape.hasBounds && !shape.hasPolygon) {
				rect(line, shape.min, shape.max, r, g, b, a);
			}
			for (const std::vector<vector2>& polygonLoop : shape.polygonLoops) {
				loop(line, polygonLoop, r, g, b, a);
			}
			if (shape.hasSweep) {
				line(shape.sweepStart, shape.sweepEnd, 0.15f, 0.75f, 1.0f, 0.8f);
			}
			cross(line, shape.objectPosition, 2.0f, 1.0f, 1.0f, 1.0f, 0.9f);
			cross(line, shape.collisionAnchor, 2.0f, 0.0f, 1.0f, 1.0f, 0.9f);

			if (shape.renderableOffset.x != 0.0f || shape.renderableOffset.y != 0.0f) {
				cross(line, shape.anchorWithRenderableOffset, 2.0f, 0.7f, 0.4f, 1.0f, 0.9f);
				line(shape.objectPosition, shape.anchorWithRenderableOffset, 0.35f, 0.35f, 1.0f, 0.75f);
			}
		}

		for (const CollisionDebugContact& contact : collisionSystem.getDebugContacts()) {
			if (!contact.overlapping || !contact.midpoint.has_value()) {
				continue;
			}

			const vector2& midpoint = contact.midpoint.value();
			cross(line, midpoint, 2.0f, 1.0f, 0.4f, 0.0f, 0.9f);

			if (contact.normal.has_value()) {
				vector2 normal = contact.normal.value();
				const float normalLength = std::sqrt((normal.x * normal.x) + (normal.y * normal.y));
				if (normalLength > 0.0f) {
					normal = vector2(normal.x / normalLength, normal.y / normalLength);
					float lineLength = 12.0f + (contact.penetrationDepth.value_or(0.0f) * 4.0f);
					if (contact.timeOfImpact.has_value()) {
						lineLength += std::max(0.0f, (1.0f - contact.timeOfImpact.value())) * 6.0f;
					}
					line(midpoint, midpoint + (normal * lineLength), 1.0f, 0.4f, 0.0f, 1.0f);
				}
			}

			if (contact.separation.has_value()) {
				line(midpoint, midpoint + contact.separation.value(), 0.15f, 0.9f, 0.2f, 0.9f);
			}
		}
	}
}
