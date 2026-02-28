
#pragma once

#include "Renderable.h"
#include "Types.h"

#include <simdjson.h>

#include <unordered_map>
#include <string>
#include <vector>

class Font : public Renderable
{
	class ITexture* _texture; // font image
	
	int m_cWidths[256]; // widths for each character for kerning and spacing
	int m_nHeight;
	int m_nBitmapWidth;
	std::string m_text;

	std::unordered_map<char, std::vector<int>> m_bitmapData; // Optional: raw bitmap rows per char

public:
	Font(void);
	Font(int defaultWidth, int defaultHeight);

	int getWidth(char c) const;
	void setWidth(char c, int width);

	int getHeight() const;
	void setHeight(int height);
	int getBitmapWidth() const;

	void setText(const std::string& text);
	const std::string& getText() const;

	bool loadFromJSON(const std::string& filePath);
	const std::vector<int>& getBitmap(char c) const;
};
