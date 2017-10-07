// File: SoundManager.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SOUNDMANAGER_H)
#define _SOUNDMANAGER_H

#include "Factory.h"
#include "Sound.h"

class SoundManager
{
	Factory<Sound> m_Sounds;
};

#endif  //_SOUNDMANAGER_H
