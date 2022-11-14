
#pragma once

#include "Renderable.h"
#include "Types.h"

class Font : public Renderable
{
	class ITexture* _texture; // font image
	
	int m_cWidths[256]; // widths for each character
	int m_nHeight;

public:
	Font(void);
};
