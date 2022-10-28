
#include "font.h"

#include "ITexture.h"

Font::Font(void):
	Renderable(RENDERABLE_TYPE_FONT),
	m_pTexture(NULL) {
	ZeroMemory(m_cWidths, 256);
}