
#include "Font.h"

#include "Types.h"

Font::Font(void): Renderable(RENDERABLE_TYPE_FONT), _texture(NULL) {
	ZeroMemory(m_cWidths, 256);
}