// File: XAudioFile.h
// Author: Stanley Taveras
// Created: 3/14/2010
// Modified: 3/14/2010

#pragma once

#ifdef _WIN32

#include <xaudio2.h>

class XAudioFile
{
	HANDLE m_hFile;
	WAVEFORMATEX m_WaveFormat;
	XAUDIO2_BUFFER m_Buffer;

public:
	XAudioFile(void);
	~XAudioFile(void);

	bool Open(const char* szFilename);
};

#endif