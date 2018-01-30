
#include "DirectInput.h"

// TODO: Add support for mouse and gamepads

DirectInput::DirectInput(void):
IInput(),
m_hWnd(NULL),
m_lpDirectInput(NULL)
{}

DirectInput::DirectInput(HINSTANCE hInstance, HWND hWnd):
IInput(),
m_hWnd(hWnd),
m_lpDirectInput(NULL)
{
	DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_lpDirectInput, NULL);
}

void DirectInput::Initialize(void)
{
	if(m_lpDirectInput)
	{
		m_pKeyboard = new DIKeyboard();

		m_lpDirectInput->CreateDevice(GUID_SysKeyboard, &((DIKeyboard*)(m_pKeyboard))->m_lpDevice, NULL);

		((DIKeyboard*)(m_pKeyboard))->m_lpDevice->SetDataFormat(&c_dfDIKeyboard);
		((DIKeyboard*)(m_pKeyboard))->m_lpDevice->SetCooperativeLevel(m_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		
		if(FAILED(((DIKeyboard*)(m_pKeyboard))->m_lpDevice->Acquire()))
			((DIKeyboard*)(m_pKeyboard))->m_bDeviceLost = true;
	}
}

void DirectInput::Update(void)
{
	if(m_pKeyboard)
		m_pKeyboard->Update();
}

void DirectInput::Shutdown(void)
{
	if(m_lpDirectInput)
	{
		m_pKeyboard->Release();

		m_lpDirectInput->Release();
		m_lpDirectInput = NULL;
	}
}