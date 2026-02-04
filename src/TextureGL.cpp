// TextureGL.cpp

#include "TextureGL.h"

#include "stb/stb_image.h"

#include <stdexcept>

TextureGL::TextureGL(const char* path) : ITexture(path)
{
	int texWidth = 0;
	int texHeight = 0;
	int texChannels = 0;
	stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	_width = static_cast<unsigned int>(texWidth);
	_height = static_cast<unsigned int>(texHeight);

	glGenTextures(1, &_textureId);
	glBindTexture(GL_TEXTURE_2D, _textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

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
