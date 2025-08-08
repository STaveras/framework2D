
#pragma once

#ifndef _RENDERCOLLISIONVOLUMEBOUNDSOPERATOR_H_
#define _RENDERCOLLISIONVOLUMEBOUNDSOPERATOR_H_

#include "ObjectOperator.h"

#include "GameObject.h"

class RenderCollisionVolumeBoundsOperator : public ObjectOperator
{
	RenderList _renderList;

public:
	bool operator()(GameObject* object)
	{
		if (object && object->getCollidable()) {
			object->getCollidable()->renderBounds();
		}
		return true;
	}
};

#endif // _RENDERCOLLISIONVOLUMEBOUNDSOPERATOR_H_
