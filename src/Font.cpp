
#include "font.h"

#include "ITexture.h"

Font::Font(void):
	Renderable(RENDERABLE_TYPE_FONT),
	_texture(NULL) {
	ZeroMemory(m_cWidths, 256);
}