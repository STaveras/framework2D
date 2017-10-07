/////////////////////////////////////////////////////////////////
//	File		:	"font.cpp"
//
//	Author		:	Stanley Taveras (ST)
//
//	Purpose		:	Contains the font class which is responsible
//					for displaying bitmapped fonts.
/////////////////////////////////////////////////////////////////

#include "font.h"

//font::font()
//{
//	//SetStartChar( 0 );
//	//SetImageID( -1 );
//	//SetCharWidth( 0 );
//	//SetCharHeight( 0 );
//	//SetColumns( 1 );
//}

//font::font(int nImageID, int nCharWidth, int nCharHeight)
//{
//	//SetStartChar( 0 );
//	//SetImageID( nImageID );
//	//SetCharWidth( nCharWidth );
//	//SetCharHeight( nCharHeight );
//	//SetColumns( 0 );
//}

//RECT font::CalcRect(int index)
//{
//	RECT rChar;
//	rChar.left = m_nCharWidth * (index % m_nNumCol);
//	rChar.top  = m_nCharHeight * (index / m_nNumCol);
//	rChar.right = rChar.left + m_nCharWidth;
//	rChar.bottom = rChar.top + m_nCharHeight;
//
//	return rChar;
//}

//void font::DrawString( const char* szText, int nPosX, int nPosY, int nSpacing, float fScaleX, float fScaleY, DWORD dwColor )
//{
//	//if ( m_nImageID == -1)
//		return;
//
//	int nOffsetX = nPosX;
//	int nOffsetY = nPosY;
//
//	for (unsigned int i = 0; i < strlen(szText); i++)
//	{		
//		//int index = (int)(szText[i] - m_ucStartChar);
//
//		//if ( szText[i] == ' ' )
//		//{
//		//	nOffsetX += (int)(m_nCharWidth * fScaleX) + nSpacing;
//		//	continue;
//		//}
//		//else if ( szText[i] == '\n' )
//		//{
//		//	nOffsetX = nPosX;
//		//	nOffsetY += (int)(m_nCharHeight * fScaleY);
//		//	continue;
//		//}
//
//		//RECT r = CalcRect(index);
//
//		//CSGD_TextureManager::GetInstance()->Draw( m_nImageID, nOffsetX, nOffsetY, fScaleX, fScaleY, &r , 0, 0, 0.0f, dwColor);
//		//nOffsetX += (int)(m_nCharWidth * fScaleX) + nSpacing;
//	}
//}