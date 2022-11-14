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
	Color _clearKeyColor; // transparent color
	std::string _fileName;

public:
	ITexture(void): _clearKeyColor(0) {}
	ITexture(const char* szFilename): _fileName(szFilename), _clearKeyColor(0) {}
	virtual ~ITexture(void){ }

	const char* getFilename(void) const { return _fileName.c_str(); }
	Color getKeyColor(void) const { return _clearKeyColor; }
	virtual unsigned int getWidth(void) const = 0;
	virtual unsigned int getHeight(void) const = 0;

	void SetFilename(const char* szFilename) { _fileName = szFilename; }
	void SetKeyColor(Color clrColor) { _clearKeyColor = clrColor; }
	
}Texture;

#endif