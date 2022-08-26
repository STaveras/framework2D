// File: ITexture.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/24/2010

#ifndef _ITEXTURE_H_
#define _ITEXTURE_H_

#include "Types.h"

#include <string>

typedef class ITexture
{
	Color m_ClrColor; // transparent color
	std::string m_strFilename;

public:
	ITexture(void): m_ClrColor(0) {}
	ITexture(const char* szFilename): m_strFilename(szFilename), m_ClrColor(0) {}
	virtual ~ITexture(void){ }

	const char* GetFilename(void) const { return m_strFilename.c_str(); }
	Color GetKeyColor(void) const { return m_ClrColor; }
	virtual unsigned int GetWidth(void) const = 0;
	virtual unsigned int GetHeight(void) const = 0;

	void SetFilename(const char* szFilename) { m_strFilename = szFilename; }
	void SetKeyColor(Color clrColor) { m_ClrColor = clrColor; }
	
}Texture;

#endif