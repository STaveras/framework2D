#pragma once

#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "Collidable.h"

#include <vector>

class PolygonCollider : public Collidable
{
	std::vector<vector2> _localVertices;
	mutable std::vector<vector2> _worldVertices;
	mutable vector2 _cachedMin = vector2(0.0f, 0.0f);
	mutable vector2 _cachedMax = vector2(0.0f, 0.0f);
	mutable bool _cacheDirty = true;
	bool _isConvex = false;
	bool _isValid = false;

	void markDirty(void) const;
	void rebuildCache(void) const;
	void rebuildValidation(void);

public:
	PolygonCollider(void) : Collidable(COL_OBJ_POLYGON) {}
	PolygonCollider(const PolygonCollider& rhs);

	const std::vector<vector2>& getLocalVertices(void) const { return _localVertices; }
	const std::vector<vector2>& getWorldVertices(void) const;

	vector2 getMin(void) const;
	vector2 getMax(void) const;

	bool isConvex(void) const { return _isConvex; }
	bool isValid(void) const { return _isValid; }

	void setLocalVertices(const std::vector<vector2>& vertices);

	virtual void setPosition(vector2 position) override;
	virtual void setPosition(float x, float y) override;

	virtual bool collidesWith(vector2 point) override;
	virtual bool collidesWith(const Collidable* collidable) override;

	bool findTopSurfaceYAtX(float x, float& outY) const;
};

#endif // _POLYGON_H_
