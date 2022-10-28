#include "Sprite.h"
#include "ITexture.h"
#include "FileSystem.h"
#include <fstream>

// TODO: Replace saving and loading of "sprite data" using JSON
// TODO: Support pallette swapping

void Sprite::Save(const char* filePath)
{
	std::ofstream ofl(filePath, std::ios_base::binary);

	if(ofl.good())
	{
		size_t uiStrLength = strlen(m_pTexture->GetFilename()) + 1;
		color clrColor = m_pTexture->GetKeyColor();

		ofl.write((char*)&uiStrLength, sizeof(size_t));
		ofl.write(m_pTexture->GetFilename(), uiStrLength);
		ofl.write((char*)&m_SrcRect, sizeof(RECT));
		ofl.write((char*)&clrColor, sizeof(color));
		ofl.write((char*)&m_Center, sizeof(vector2));
	}

	ofl.close();
	ofl.clear();
}

Sprite* Sprite::Load(const char* filePath)
{
	Sprite* sprite = NULL;
	if (!strcmp(FileSystem::File::GetFileExtension(filePath), "spr"))
	{
		std::ifstream ifl(filePath);

		if(ifl.good())
		{
			size_t uiStrLength;
			char szPath[MAX_PATH];
			RECT srcRect;
			color clrColor;
			vector2 center;

			ifl.read((char*)&uiStrLength, sizeof(size_t));
			ifl.read(szPath, uiStrLength);
			ifl.read((char*)&srcRect, sizeof(srcRect));
			ifl.read((char*)&clrColor, sizeof(color));
			ifl.read((char*)&center, sizeof(vector2));

			Image::Load(szPath, clrColor, &srcRect);
			this->SetCenter(center);
			sprite = this;
		}

		ifl.close();
		ifl.clear();
	}
	else
        Image::Load(filePath);

	return sprite;
}