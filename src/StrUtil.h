// File: StrUtil.h
// Author: Stan Taveras
// Created: 6/29/2010
// Modified: 6/29/2010

#pragma once

#include <list>
#include <string>

#define STR_EQUALS(x, y) !strcmp(x, y)

// Trims leading whitespace
static char* strtrlws(char* szString)
{
	char* pItr = szString;

	while(*pItr == ' ' || *pItr == '\t' || *pItr == '\n')
		pItr++;

	return pItr;
}

// Trims trailing whitespace
static char* strtrtws(char* szString)
{
	char* pItr = szString + strlen(szString);

	while(*pItr == ' ' || *pItr == '\t' || *pItr == '\n')
	{
		*pItr = '\0';
		pItr--;
	}

	return szString;
}

// Trims leading and trailing whitespace
static char* strtrws(char* szString)
{
	return strtrtws(strtrlws(szString));
}

// Determines whether two strings are the same
static bool streqls(const char* szLeft, const char* szRight, bool bCaseSensitive = false)
{
	if(bCaseSensitive)
	{
		return !strcmp(szLeft, szRight);
	}
	else
	{
		size_t uiLength = strlen(szLeft);

		if(uiLength != strlen(szRight))
			return false;

		const char* pItr1 = szLeft + uiLength;
		const char* pItr2 = szRight + uiLength;

		for(; uiLength > 0; uiLength--, pItr1--, pItr2--)
		{
			if(tolower(*pItr1) != tolower(*pItr2))
				return false;
		}
	}

	return true;
}

// Replaces occurances of characters specified by szDelims with szReplacement
static void strsubst(char* szString, char szReplacement, const char* szDelims)
{
	char* pItr = szString;

	while(*pItr != '\0')
	{
		for(int i = 0; i < (int)strlen(szDelims); i++)
		{
			if(*pItr == szDelims[i])
				*pItr = szReplacement;
		}

		pItr++;
	}
}

// Outputs a list of strings from the szTarget string, separated by characters specified in szDelims
static void strdiv(std::list<std::string>& output, const char* szTarget, const char* szDelims)
{
	std::string szTemp;
	const char* pItr = szTarget;

	while(*pItr != '\0')
	{
		for(int i = 0; i < (int)strlen(szDelims); i++)
		{
			if(*pItr == szDelims[i] && szTemp != "")
			{
				output.push_back(szTemp);
				szTemp.clear();
				pItr++;
				break;
			}
		}

		szTemp += *pItr;
		pItr++;
	}

	if(szTemp != "")
		output.push_back(szTemp);
}