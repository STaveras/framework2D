// File: VirtualGamePad.h
#pragma once

#include "IInput.h"
#include "VirtualButton.h"
#include <list>

class VirtualGamePad
{
    bool m_bConnected;
	int m_nPadNumber;
	IInput* m_pInput;
	std::list<VirtualButton> m_lsButtons;

public:
	VirtualGamePad(void):m_bConnected(false), m_nPadNumber(-1), m_pInput(NULL){}
	~VirtualGamePad(void){}

	bool IsConnected(void) const { return m_bConnected; }
	int GetPadNumber(void) const { return m_nPadNumber; }
	IInput* GetInputInterface(void) { return m_pInput; }
	std::list<VirtualButton>& GetButtons(void) { return m_lsButtons; }
	VirtualButton* GetButtons(ButtonID eKey);

	void SetIsConnected(bool bConnected) { m_bConnected = bConnected; }
	void SetPadNumber(int nPadNumber) { m_nPadNumber = nPadNumber; }
	void SetInputInterface(IInput* pInput) { m_pInput = pInput; }

	void addButton(VirtualButton btn) { m_lsButtons.push_back(btn); }
	void RemoveButton(VirtualButton btn);

	bool ButtonPressed(ButtonID btnID);
	bool ButtonReleased(ButtonID btnID);
};

typedef VirtualGamePad controller;
// Author: Stanley Taveras