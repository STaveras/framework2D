
#ifdef _WIN32

#include "XAudioFile.h"

XAudioFile::XAudioFile(void)
{
	ZeroMemory(this, sizeof(XAudioFile));
}

XAudioFile::~XAudioFile(void)
{

}

bool XAudioFile::Open(const char* szFilename)
{
	return false;
}

#endif