// File: TextureMTL.cpp
#ifdef __APPLE__
#include "TextureMTL.h"

// Constructor
TextureMTL::TextureMTL(const char* szFilename)
    : ITexture(szFilename), _texture(nil), _width(0), _height(0)
{
    // Any specific initialization logic can be added here if needed
}

// Destructor
TextureMTL::~TextureMTL(void)
{
    // Release the Metal texture or any other resources if needed
}
#endif //__APPLE__
