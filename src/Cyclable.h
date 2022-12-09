#pragma once

// This is a weird one
class Cyclable
{
public:
	virtual void start(void) = 0;
	virtual void update(float time) = 0;
	virtual void finish(void) = 0;
};