// File: Sound.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SOUND_H)
#define _SOUND_H

#include "SOUND_PLAYBACK_MODE.h"

class Sound
{
	SOUND_PLAYBACK_MODE m_eMode;
	const char* m_szName;
	const char* m_szFilename;

public:
	void Play(void);
	void Stop(void);
	void Reset(void);
};

typedef Sound Music;

#endif  //_SOUND_H
