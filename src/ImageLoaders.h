#ifndef _IMAGE_LOADERS_H_
#define _IMAGE_LOADERS_H_

namespace framework
{
	namespace FileFormats
	{
		// How are we supposed to know the image dimensions prior to opening the file?
		// Or was the intent here to be able to define the image dimensions?
		// I guess actually, it was to output the image dimensions, as they're being passed in by reference...
		//////////
		char* loadBMP(const char* szFilepath, long& lWidth, long& lHeight, int& nBytesPerPixel);
		char* loadDXT(const char* szFilepath, long& lWidth, long& lHeight, short& sPixelDepth); // TODO: Implement DXT format texture loading
		char* loadJPG(const char* szFilepath, long& lWidth, long& lHeight, short& sPixelDepth); // TODO: Implement JPEG format texture loading
		// char* loadPNG(const char* szFilepath, long& lWidth = 0, long& lHeight = 0, int& nBytesPerPixel = 32); // UNDONE: Finish PNG image loading
		char* loadPNG(const char* szFilepath);
		char* loadTGA(const char* szFilepath, short& sImageWidth, short& sImageHeight, int& nBytesPerPixel);
	}
}

#endif