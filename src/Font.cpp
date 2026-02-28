
#include "Font.h"

#include "Types.h"

#include <algorithm>

namespace {
int getRowWidth(int row)
{
	int width = 0;
	while (row > 0) {
		++width;
		row >>= 1;
	}
	return width;
}
}

Font::Font(void)
	: Renderable(RENDERABLE_TYPE_FONT)
	, _texture(NULL)
	, m_nHeight(0)
	, m_nBitmapWidth(0)
{
	ZeroMemory(m_cWidths, sizeof(m_cWidths));
}

Font::Font(int defaultWidth, int defaultHeight)
	: Renderable(RENDERABLE_TYPE_FONT)
	, _texture(NULL)
	, m_nHeight(defaultHeight)
	, m_nBitmapWidth(defaultWidth)
{
	for (int i = 0; i < 256; ++i) {
		m_cWidths[i] = defaultWidth;
	}
}

int Font::getWidth(char c) const
{
	return m_cWidths[static_cast<unsigned char>(c)];
}

void Font::setWidth(char c, int width)
{
	m_cWidths[static_cast<unsigned char>(c)] = width;
}

int Font::getHeight() const
{
	return m_nHeight;
}

void Font::setHeight(int height)
{
	m_nHeight = height;
}

int Font::getBitmapWidth() const
{
	return m_nBitmapWidth;
}

void Font::setText(const std::string& text)
{
	m_text = text;
}

const std::string& Font::getText() const
{
	return m_text;
}

bool Font::loadFromJSON(const std::string& filePath)
{
	m_bitmapData.clear();
	m_nBitmapWidth = 0;

	simdjson::ondemand::parser parser;
	auto jsonResult = simdjson::padded_string::load(filePath);
	if (jsonResult.error()) {
		return false;
	}

	auto docResult = parser.iterate(jsonResult.value());
	if (docResult.error()) {
		return false;
	}

	auto doc = std::move(docResult.value());
	auto typeResult = doc.type();
	if (typeResult.error() || typeResult.value() != simdjson::ondemand::json_type::object) {
		return false;
	}

	int maxHeight = 0;
	int maxBitmapWidth = 0;
	auto objectResult = doc.get_object();
	if (objectResult.error()) {
		return false;
	}

	for (auto field : objectResult.value())
	{
		auto keyResult = field.unescaped_key();
		if (keyResult.error()) {
			continue;
		}

		std::string_view key = keyResult.value();
		if (key.size() != 1) {
			continue;
		}

		const char ch = key[0];
		std::vector<int> bitmap;
		int maxWidth = 0;

		auto arrayResult = field.value().get_array();
		if (arrayResult.error()) {
			continue;
		}

		for (auto value : arrayResult.value())
		{
			auto rowResult = value.get_int64();
			if (rowResult.error()) {
				continue;
			}

			const int row = static_cast<int>(rowResult.value());
			bitmap.push_back(row);
			const int rowWidth = getRowWidth(row);
			maxWidth = std::max(maxWidth, rowWidth);
			maxBitmapWidth = std::max(maxBitmapWidth, rowWidth);
		}

		m_cWidths[static_cast<unsigned char>(ch)] = maxWidth;
		m_bitmapData[ch] = bitmap;
		maxHeight = std::max(maxHeight, static_cast<int>(bitmap.size()));
	}

	m_nHeight = maxHeight;
	m_nBitmapWidth = maxBitmapWidth;
	return !m_bitmapData.empty();
}

const std::vector<int>& Font::getBitmap(char c) const
{
	static const std::vector<int> empty;
	const auto it = m_bitmapData.find(c);
	return it != m_bitmapData.end() ? it->second : empty;
}
