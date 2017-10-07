
#pragma once

#ifdef WIN32
#include <Windows.h>
#endif

namespace File
{
	const char* getFileExtension(const char* szFilepath)
	{
		size_t uiFilepathLength = strlen(szFilepath);

		const char* szExtension = szFilepath + uiFilepathLength;

		while (*(--szExtension) != '.' && --uiFilepathLength)
			continue;

		return szExtension;
	}
}