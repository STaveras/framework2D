
#include "DIKeyboard.h"

DIKeyboard::DIKeyboard(void):
m_bDeviceLost(false),
m_lpDevice(NULL)
{
	memset(m_cKeyBuffer, 0, _countof(m_cKeyBuffer));
}

DIKeyboard::~DIKeyboard(void)
{
	Release();
}

void DIKeyboard::Release(void)
{
	if(m_lpDevice)
	{
		m_lpDevice->Unacquire();
		m_lpDevice->Release();
		m_lpDevice = NULL;
	}
}

void DIKeyboard::Update(void)
{
	if(m_lpDevice)
	{
		if(m_bDeviceLost)
		{
			m_lpDevice->Unacquire();

			if(!FAILED(m_lpDevice->Acquire()))
				m_bDeviceLost = false;
		}
		else
		{
			memcpy_s(m_cBackBuffer, 256, m_cKeyBuffer, 256);

			if(m_lpDevice->GetDeviceState(sizeof(m_cKeyBuffer), (LPVOID)&m_cKeyBuffer) == DIERR_INPUTLOST)
			{
				m_lpDevice->Unacquire();

				if(FAILED(m_lpDevice->Acquire()))
				{
					m_bDeviceLost = true;
				}
				else
				{
					// Attempt to read the device state again
					m_lpDevice->GetDeviceState(sizeof(m_cKeyBuffer), (LPVOID)&m_cKeyBuffer); 
				}
			}
		} // !m_bDeviceLost
	}
}