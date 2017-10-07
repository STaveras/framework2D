
#include "font.h"

#include "ITexture.h"

font::font(void):
	Renderable(RENDERABLE_TYPE_FONT),
	m_pTexture(NULL),
	m_fScale(1.0f)
{
	ZeroMemory(m_cWidths, 256);
}