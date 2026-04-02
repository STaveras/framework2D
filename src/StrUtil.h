// File: StrUtil.h
// Author: Stan Taveras
// Legacy compatibility shim. Prefer StrUtils.h in new code.

#pragma once

#include "StrUtils.h"

#include <cctype>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#define STR_EQUALS(x, y) (std::strcmp((x), (y)) == 0)

inline char* strtrlws(char* szString)
{
	if (!szString) {
		return nullptr;
	}

	while (*szString != '\0' && std::isspace((unsigned char)*szString)) {
		++szString;
	}
	return szString;
}

inline char* strtrtws(char* szString)
{
	if (!szString) {
		return nullptr;
	}

	size_t len = std::strlen(szString);
	while (len > 0 && std::isspace((unsigned char)szString[len - 1])) {
		szString[len - 1] = '\0';
		--len;
	}
	return szString;
}

inline char* strtrws(char* szString)
{
	return strtrtws(strtrlws(szString));
}

inline bool streqls(const char* szLeft, const char* szRight, bool bCaseSensitive = false)
{
	if (!szLeft || !szRight) {
		return false;
	}
	return bCaseSensitive ? (std::strcmp(szLeft, szRight) == 0) : StrUtils::IEquals(szLeft, szRight);
}

inline void strsubst(char* szString, char szReplacement, const char* szDelims)
{
	if (!szString || !szDelims) {
		return;
	}

	for (char* pItr = szString; *pItr != '\0'; ++pItr) {
		if (std::strchr(szDelims, *pItr)) {
			*pItr = szReplacement;
		}
	}
}

inline void strdiv(std::list<std::string>& output, const char* szTarget, const char* szDelims)
{
	output.clear();
	for (const std::string& token : StrUtils::SplitAny(szTarget ? szTarget : "", szDelims ? szDelims : "", true, true)) {
		output.push_back(token);
	}
}

inline std::vector<std::string> split(const std::string& s, char delimiter)
{
	return StrUtils::Split(s, delimiter, false);
}
