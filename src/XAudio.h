// File: XAudio.h
// Author: Stanley Taveras
// Created: 3/13/2010
// Modified: 3/13/2010

#pragma once

#include "IAudioSystem.h"

#include <xaudio2.h>
#pragma comment(lib, "Xaudio2.lib")

class XAudio : public IAudioSystem
{
	IXAudio2* m_pXAudio2;
	IXAudio2MasteringVoice* m_pMasteringVoice;

	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset);

public:
	XAudio(void);
	~XAudio(void);

	void Initialize(void);
	void Shutdown(void);
};