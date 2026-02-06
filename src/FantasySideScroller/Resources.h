#pragma once

#include "../System.h"

#include <string>

inline const char* BaseDir()
{
	return System::GlobalDataPath();
}

inline std::string BasePath(const char* relativePath)
{
	std::string base = System::GlobalDataPath();
	if (!base.empty() && base.back() != '/' && base.back() != '\\') {
		base.push_back('/');
	}
	return base + relativePath;
}
