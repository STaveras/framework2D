// TextureGL.cpp

#include "TextureGL.h"

#include "stb/stb_image.h"

#include <stdexcept>

TextureGL::TextureGL(const char* path, Color colorKey) : ITexture(path)
{
	int texWidth = 0;
	int texHeight = 0;
	int texChannels = 0;
	stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}
	if (colorKey._color != 0) {
		for (size_t i = 0, count = static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight); i < count; ++i) {
			stbi_uc* pixel = pixels + i * 4;
			if (pixel[0] == colorKey.r && pixel[1] == colorKey.g && pixel[2] == colorKey.b) {
				pixel[3] = 0;
			}
		}
	}

	_width = static_cast<unsigned int>(texWidth);
	_height = static_cast<unsigned int>(texHeight);

	glGenTextures(1, &_textureId);
	glBindTexture(GL_TEXTURE_2D, _textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);
}

TextureGL::~TextureGL()
{
	if (_textureId != 0) {
		glDeleteTextures(1, &_textureId);
		_textureId = 0;
	}
}
