// File: ITexture.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/24/2010

#if !defined(_ITEXTURE_H_)
#define _ITEXTURE_H_
#include "color.h"
#include <string>

class ITexture
{
	color m_ClrColor; // transparent color
	std::string m_strFilename;

public:
	ITexture(void): m_ClrColor(0) {}
	ITexture(const char* szFilename): m_strFilename(szFilename), m_ClrColor(0) {}
	virtual ~ITexture(void){ }

	const char* GetFilename(void) const { return m_strFilename.c_str(); }
	color GetKeyColor(void) const { return m_ClrColor; }
	virtual unsigned int GetWidth(void) const = 0;
	virtual unsigned int GetHeight(void) const = 0;

	void SetFilename(const char* szFilename) { m_strFilename = szFilename; }
	void SetKeyColor(color clrColor) { m_ClrColor = clrColor; }
};

#endif