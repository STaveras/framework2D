#include "ImageLoaders.h"

#include "Types.h"

// int32_t flipEndianness(int32_t value) {

// 	// // byte flippedValue[4] = (byte*)(&value);
// 	// byte *rawValue = (byte*)(&value);

// 	// for (int i = 0; i < 4; i++) {

// 	// 	// flippedValue[i] = (byte)((void*)(&value))[3-i];

// 	// }
// 	// return flippedValue;
// }

int32_t flipEndianness(int32_t value)
 {
	int32_t flippedValue = value;

	byte* pV1 = (byte*)(&flippedValue), *pV2 = (byte*)(&value);

	for (int i = 0; i < sizeof(int32_t); i++) {
		pV1[i] = pV2[(sizeof(int32_t) - 1) - i];
	}

	return flippedValue;
}

char* Renderer::Image::loadBMP(const char* szFilepath, long& lWidth, long& lHeight, int& nBytesPerPixel)
{	
	ifpstream ifl;
	ifl.open(szFilepath, std::ios_base::in | std::ios_base::binary);

	char* chImageData = NULL;

	if(ifl.is_open())
	{
		short sType = -1;
		int   nSize = -1;
		int   nOffset = -1;

		// This codeblock reads in the header information for the bitmap
		ifl.read((char*)&sType, sizeof(short));
		ifl.read((char*)&nSize, sizeof(int));
		ifl.seekg(4, std::ios_base::cur); // Move foward 4 bytes, skipping the reserved information
		ifl.read((char*)&nOffset, sizeof(int));

		if (sType == 0x4D42)
		{
			short sPixelDepth;
			long  lHeaderSize;
			long  lImageSize;

			// Loads the bitmap DIB information, based on the common Windows V3 format
			ifl.read((char*)&lHeaderSize, sizeof(long));
			ifl.read((char*)&lWidth, sizeof(long));
			ifl.read((char*)&lHeight, sizeof(long));
			ifl.seekg(2, std::ios_base::cur); // Skip color plane information
			ifl.read((char*)&sPixelDepth, sizeof(short));
			ifl.seekg(4, std::ios_base::cur); // Skip compression method
			ifl.read((char*)&lImageSize, sizeof(long));
			ifl.seekg(nOffset);	// Seek directly to the image data

			// Just in case lImageSize is read in as zero...
			// Calculate the image size (assuming nSize and nOffset read in fine)
			if(!lImageSize)
				lImageSize = nSize - nOffset;

			nBytesPerPixel = (sPixelDepth / 8);

			// Create the image buffer with the given image size times 3 (3 bytes per pixel = 24 bits)
			chImageData = new char[lImageSize * nBytesPerPixel];
			ifl.read(chImageData, (lImageSize * nBytesPerPixel));
		}
	}
	ifl.close();

	return chImageData;
}

// char* Renderer::Image::loadPNG(const char* szFilepath, long& lWidth, long& lHeight, int& nBytesPerPixel)
char* Renderer::Image::loadPNG(const char* szFilepath)
{
	struct {
		uint32_t width;
		uint32_t height;
		uint8_t  bitDepth;
		uint8_t  colorType;
		uint8_t  compressionType;
		uint8_t  filterMethod;
		uint8_t  interlaceMethod;
	}imageInfo;

	ifpstream ifl;
	ifl.open(szFilepath, std::ios_base::in | std::ios_base::binary);

	char* chImageData = NULL;
	if(ifl.is_open())
	{
		char szFormatID[4] = {0};

		ifl.seekg(1, std::ios_base::cur);
		ifl.read(szFormatID, 3); // The only important information in a PNG's header... (For our use)

		if(!strcmp(szFormatID, "PNG"))
		{
			char szChunkInfo[5] = {0};

			ifl.seekg(4, std::ios_base::cur); // Pass over the rest of the PNG header info

			while(!ifl.eof())
			{
				int32_t length{};

				ifl.read(szChunkInfo, 4);
				
				memcpy(&length, szChunkInfo, sizeof(int32_t));

				length = flipEndianness(length); 

				ifl.read(szChunkInfo, 4);

				// Critical chunk
				if (szChunkInfo[0] < 91) {
					
					// Image header information
					if (!strcmp(szChunkInfo, "IHDR")) {
						
						ifl.read((char*)&imageInfo, length);

						imageInfo.width = flipEndianness(imageInfo.width);
						imageInfo.height = flipEndianness(imageInfo.height);

					} // Pallette info
					else if (!strcmp(szChunkInfo, "PLTE")) {
						ifl.seekg(length, std::ios_base::cur);
					} // Image data
					else if (!strcmp(szChunkInfo, "IDAT")) {
						chImageData = new char[length]; 
						ifl.read(chImageData, length);
					} 
					else if (!strcmp(szChunkInfo, "IEND")) {
						break;
					}
				}
				else { // Anciallary 
					ifl.seekg(length, std::ios_base::cur);
				}

				// Chunk CRC; we should actually use this to verify image integrity... 
				// But it probably means a slowdown so.... ehhh
				ifl.read(szChunkInfo, 4);
			}
		}
	}
	ifl.close();

	return chImageData;
}

char* Renderer::Image::loadTGA(const char* szFilepath, short& sImageWidth, short& sImageHeight, int& nBytesPerPixel)
{
	// TODO: Support different pixel-order TGAs in the future...
	ifpstream ifl;
	ifl.open(szFilepath, std::ios_base::in | std::ios_base::binary);

	char* chImageData = NULL;
	if(ifl.is_open())
	{
		char chIDLength = 0;
		char chColorMapType = 0;
		char chImageType = 0;

		ifl.read(&chIDLength, sizeof(char));
		ifl.read(&chColorMapType, sizeof(char));
		ifl.read(&chImageType, sizeof(char));

		// If this is a TrueColor image (or has any image data at all...)
		if (chImageType == 0x02)
		{						
			short sFirstEntryIndex = 0;
			short sColorMapLength = 0;

			char  chPixelDepth = 0;
			char  chColorMapEntrySize = 0;
			char  chImageDescriptor = 0;

			// If a color map isn't present, skip the color map specification
			if(!chColorMapType)
				ifl.seekg(5, std::ios_base::cur);
			else // Else, actually read in this information
			{
				ifl.read((char*)&sFirstEntryIndex, sizeof(short));
				ifl.read((char*)&sColorMapLength, sizeof(short));
				ifl.read(&chColorMapEntrySize, sizeof(char));
			}

			// Read the rest of the image information
			ifl.seekg(4, std::ios_base::cur); // Omit the X-Origin, Y-Origin stuff... (I see no use for it right now)
			ifl.read((char*)&sImageWidth, sizeof(short));
			ifl.read((char*)&sImageHeight, sizeof(short));
			ifl.read(&chPixelDepth, sizeof(char));
			ifl.read(&chImageDescriptor, sizeof(char));

			// I'm ignoring the image ID section of the image, if it has one
			if(chIDLength)
				ifl.seekg(chIDLength, std::ios_base::cur);

			// Same with the color map... I'm not supporting it right now (so that means only TrueColor Images!)
			if(chColorMapEntrySize)
				ifl.seekg(chColorMapEntrySize, std::ios_base::cur);

			nBytesPerPixel = (chPixelDepth / 8);

			// Calculate the actual image size (in bytes)
			int nImageSize = sImageWidth * sImageHeight * nBytesPerPixel;

			char* chImageData = new char[nImageSize];
			ifl.read(chImageData, nImageSize);
		}
	}
	ifl.close();

	return chImageData;
}