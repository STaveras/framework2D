/////////////////////////////////////////////////////////////////
//	File		:	"font.h"
//
//	Author		:	Stanley Taveras (ST)
//
//	Purpose		:	Contains the font class which is responsible
//					for displaying bitmapped fonts.
/////////////////////////////////////////////////////////////////
#pragma once
#include <windows.h>
//#include "CSGD_TextureManager.h"
class font
{
	unsigned char	m_ucStartChar;

	int		m_nImageID;
	int		m_nCharWidth;
	int		m_nCharHeight;
	int		m_nNumCol;

	RECT CalcRect(int);

public:
	font();
	font(int, int, int);

	/////////////////////////////////////////////////////////////////
	//	Function:	Accessors
	// 
	//	Purpose:	To get the specified type.
	/////////////////////////////////////////////////////////////////
	unsigned char GetStartChar(void) { return m_ucStartChar; }
	int GetImageID(void)			 { return m_nImageID; }
	int	GetCharWidth(void)			 { return m_nCharWidth; }
	int	GetCharHeight(void)			 { return m_nCharHeight; }
	int	GetColumns(void)			 { return m_nNumCol; }

	//////////////////////////////////////////////////////////
	//	Function:	Modifiers
	//
	//	Purpose:	To modify the specified type.
	//////////////////////////////////////////////////////////
	void SetStartChar(unsigned char ucStartChar) { m_ucStartChar = ucStartChar; }
	void SetImageID(int nImageID)				 { m_nImageID = nImageID; }
	void SetCharWidth(int nCharWidth)			 { m_nCharWidth = nCharWidth; }
	void SetCharHeight(int nCharHeight)			 { m_nCharHeight = nCharHeight; }
	void SetColumns(int nNumCol)				 { m_nNumCol = nNumCol; }

	void DrawString( const char* szText, int nPosX, int nPosY, int nSpacing, float fScale = 1.0f, float fScaleY = 1.0f, DWORD dwColor = D3DCOLOR_ARGB( 255, 255, 255, 255) );
};