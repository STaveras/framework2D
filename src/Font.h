
#pragma once

#include "Renderable.h"
#include "Types.h"

#include <unordered_map>
#include <vector>
#include <string>

class Font : public Renderable
{
	class ITexture* _texture; // font image
	
	int m_cWidths[256]; // widths for each character for kerning and spacing
	int m_nHeight;

	std::unordered_map<char, std::vector<int>> m_bitmapData; // Optional: raw bitmap rows per char

public:
	Font(void);
	Font(int defaultWidth, int defaultHeight): _texture(NULL), m_nHeight(defaultHeight) {
		for (int i = 0; i < 256; i++) {
			m_cWidths[i] = defaultWidth;
		}
	}

	int getWidth(char c)
	{
		// Make sure 'c' is within the ASCII range (no UNICODE support atm for these bitmapped fonts)
		if (c < 0 || c > 255) {
			return -1; // Invalid character
		}
		return m_cWidths[(unsigned char)c];
	}

	void setWidth(char c, int width)
	{
		// Make sure 'c' is within the ASCII range (no UNICODE support atm for these bitmapped fonts)
		if (c < 0 || c > 255) {
			return;
		}
		m_cWidths[(unsigned char)c] = width;
	}

	bool loadFromJSON(const std::string& filePath)
	{
		simdjson::ondemand::parser parser;
		simdjson::padded_string json = simdjson::padded_string::load(filePath);
		auto doc = parser.iterate(json);

		if (doc.type() != simdjson::ondemand::json_type::object)
			return false;

		for (auto field : doc.get_object())
		{
            auto key_view_result = field.unescaped_key();
            if (key_view_result.error()) continue; // skip if error
            std::string_view key_view = key_view_result.value();
            std::string key(key_view.data(), key_view.size());

			if (key.length() != 1)
				continue; // Skip multi-char keys or control codes

			char ch = key[0];
			std::vector<int> bitmap;
			int maxWidth = 0;

			for (auto val : field.value().get_array())
			{
				int row = int(val.get_int64());
				bitmap.push_back(row);
				int width = 0;
				int temp = row;
				while (temp)
				{
					width++;
					temp >>= 1;
				}
				if (width > maxWidth)
					maxWidth = width;
			}

			if (ch >= 0 && ch <= 255)
			{
				m_cWidths[(unsigned char)ch] = maxWidth;
				m_bitmapData[ch] = bitmap;
			}
		}

		return true;
	}

	const std::vector<int>& getBitmap(char c) const
	{
		static const std::vector<int> empty;
		auto it = m_bitmapData.find(c);
		return it != m_bitmapData.end() ? it->second : empty;
	}
};
