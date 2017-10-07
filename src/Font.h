
#pragma once

#include "Renderable.h"
#include "Types.h"

class font : public Renderable
{
	class ITexture* m_pTexture; // font image
	int m_cWidths[256]; // widths for each character
	int m_nHeight;
	float m_fScale; // for sizing the font
	vector2 m_vPosition; // screen coords 

public:
	font(void);
};
