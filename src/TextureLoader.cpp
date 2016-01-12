#include <iostream>
#include "GL/glew.h"
#include "bsp.h"
#include "TextureLoader.h"

/**
 * Constructor.
 */
TextureLoader::TextureLoader()
{

}//end TextureLoader::TextureLoader()


/**
 * Destructor.
 */
TextureLoader::~TextureLoader()
{
	this->m_sWadFile.close();

}//end TextureLoader::~TextureLoader()


/**
 * Load textures from a WAD file.
 * \param szGamePaths A vector of gamepath strings.
 * \param szFilename  Filename to load.
 */
int TextureLoader::LoadTexturesFromWAD(const std::vector<std::string> &szGamePaths, const string &szFilename) {

	// Try to open the file from all known gamepaths.
	for (size_t i = 0; i < szGamePaths.size(); i++) {
		if (!this->m_sWadFile.is_open()) {
			this->m_sWadFile.open(szGamePaths[i] + szFilename.c_str(), std::ios::binary);
		}
	}

	// If the WAD wasn't found in any of the gamepaths...
	if(!this->m_sWadFile.is_open()) {
		std::cerr << "Can't load WAD " << szFilename << "." << std::endl;
		return -1;
	}
	
	// Read header.
	WadHeader sWadHeader; this->m_sWadFile.read((char*)&sWadHeader, sizeof(WadHeader));

	if (!this->IsValidWADHeader(sWadHeader)) {
		return -1;
	}
	
	// Read directory entries.
	WadEntry *sWadEntry = new WadEntry[sWadHeader.iLumpCount];

	this->m_sWadFile.seekg(sWadHeader.iLumpOffset, ios::beg);		
	this->m_sWadFile.read((char*)sWadEntry, sizeof(WadEntry) * sWadHeader.iLumpCount);
	
	uint8_t *dataDr  = new uint8_t[512*512];   // Raw texture data.
	uint8_t *dataUp  = new uint8_t[512*512*4]; // 32 bit texture.
	uint8_t *dataPal = new uint8_t[256*3];     // 256 color pallete.
	
	for(unsigned int i = 0; i < sWadHeader.iLumpCount; i++) {
		this->m_sWadFile.seekg(sWadEntry[i].iOffset, ios::beg);

		TextureInfo sTextureInfo;
		this->m_sWadFile.read((char*)&sTextureInfo, sizeof(TextureInfo));

		// Only load if it's the first appearance of the texture.
		if(textures.count(sTextureInfo.szName) == 0) {
			Texture sTexture;
			sTexture.iWidth = sTextureInfo.iWidth;
			sTexture.iWidth = sTextureInfo.iHeight;

			glGenTextures(1, &sTexture.iTextureId);		
			glBindTexture(GL_TEXTURE_2D, sTexture.iTextureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
			
			// Sizes of each mipmap.
			const int dimensionsSquared[4] = {1,4,16,64};
			const int dimensions[4] = {1,2,4,8};
			
			// Read each mipmap.
			for(int mip = 3; mip >= 0; mip--) {
				if (sTextureInfo.iOffsets[0] == 0 || sTextureInfo.iOffsets[1] == 0 || sTextureInfo.iOffsets[2] == 0 || sTextureInfo.iOffsets[3] == 0) {
					std::cout << "Texture found, but no mipmaps. " << sTextureInfo.szName << std::endl;
					break;
				}

				this->m_sWadFile.seekg(sWadEntry[i].iOffset + sTextureInfo.iOffsets[mip], ios::beg);
				this->m_sWadFile.read((char*)dataDr, sTextureInfo.iWidth * sTextureInfo.iHeight / dimensionsSquared[mip]);
			
				if(mip == 3) {
					// Read the pallete (comes after last mipmap).
					uint16_t dummy;
					this->m_sWadFile.read((char*)&dummy, 2);
					this->m_sWadFile.read((char*)dataPal, 256 * 3);
				}
				
				for (uint32_t y = 0; y < sTextureInfo.iHeight / dimensions[mip]; y++) {
					for (uint32_t x = 0; x < sTextureInfo.iWidth / dimensions[mip]; x++) {
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4]     = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3];
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 1] = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3 + 1];
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 2] = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3 + 2];

						// Do full transparency on blue pixels.
						if (   dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4]     == 0
							&& dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 1] == 0
							&& dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 2] == 255
						) {
							dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 3] = 0;
						}
						else {
							dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 3] = 255;
						}
					}
				}
				
				glTexImage2D(GL_TEXTURE_2D, mip, GL_RGBA, sTextureInfo.iWidth / dimensions[mip], sTextureInfo.iHeight / dimensions[mip], 0, GL_RGBA, GL_UNSIGNED_BYTE, dataUp);
			}
			
			this->m_vLoadedTextures[sTextureInfo.szName] = sTexture;
		}
	}
	
	this->m_sWadFile.close();

	delete []dataDr; delete []dataUp; delete []dataPal; delete []sWadEntry;
	return 0;
}


/**
 * Check if a loaded WAD header is valid.
 * \param sHeader Loaded WAD header structure.
 */
bool TextureLoader::IsValidWADHeader(const WadHeader &sHeader)
{
	if (   sHeader.szMagic[0] != 'W'
		|| sHeader.szMagic[1] != 'A'
		|| sHeader.szMagic[2] != 'D'
		|| sHeader.szMagic[3] != '3'
	) {
		return false;
	}

	return true;

}//end TextureLoader::IsValidWADHeader()