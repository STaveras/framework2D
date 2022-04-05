
#pragma once

class IKeyboard
{
public:
	virtual ~IKeyboard(void) = 0;

	virtual void Release(void) = 0;

	virtual bool KeyDown(int nKey) = 0;
	virtual bool KeyUp(int nKey) = 0;
	virtual bool KeyPressed(int nKey) = 0;
	virtual bool KeyReleased(int nKey) = 0;

	virtual void Update(void) = 0;
};