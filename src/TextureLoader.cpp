/*
 * halfmapper, a renderer for GoldSrc maps and chapters.
 *
 * Copyright(C) 2014  Gonzalo Ávila "gzalo" Alterach
 * Copyright(C) 2015  Anthony "birkett" Birkett
 *
 * This file is part of halfmapper.
 *
 * This program is free software; you can redistribute it and / or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 */
#include "util/MemoryDebugging.h"
#include "logger/Logger.h"
#include "TextureLoader.h"
#include "VideoSystem.h"


/**
 * Get the singleton instance.
 */
TextureLoader* TextureLoader::GetInstance()
{
	static std::auto_ptr<TextureLoader> instance(new TextureLoader());
	return instance.get();

}//end TextureLoader::GetInstance()


/**
 * Destructor.
 */
TextureLoader::~TextureLoader()
{

}//end TextureLoader::~TextureLoader()


/**
 * Load textures from a file.
 * \param szGamePaths A vector of gamepath strings.
 * \param iOffsets    Array of texture offsets.
 * \param inFile      File stream (already opened).
 * \param videosystem Pointer to the videosystem object.
 */
int TextureLoader::LoadTextures(const unsigned int &iNumberOfTextures, const uint32_t* iOffsets, std::ifstream &inFile, VideoSystem* videosystem)
{
	uint8_t *dataDr  = new uint8_t[512*512];   // Raw texture data.
	uint8_t *dataUp  = new uint8_t[512*512*4]; // 32 bit texture.
	uint8_t *dataPal = new uint8_t[256*3];     // 256 color pallete.

	for(unsigned int i = 0; i < iNumberOfTextures; i++) {
		inFile.seekg(iOffsets[i], std::ios::beg);

		TextureInfo sTextureInfo;
		inFile.read((char*)&sTextureInfo, sizeof(TextureInfo));

		// Only load if it's the first appearance of the texture.
		if(this->m_vLoadedTextures.count(sTextureInfo.szName) == 0) {
			Texture sTexture;
			sTexture.iWidth  = sTextureInfo.iWidth;
			sTexture.iHeight = sTextureInfo.iHeight;

			sTexture.iTextureId = videosystem->CreateTexture(false);

			// Sizes of each mipmap.
			const int dimensionsSquared[4] = {1,4,16,64};
			const int dimensions[4] = {1,2,4,8};

			// Read each mipmap.
			for(int mip = 3; mip >= 0; mip--) {
				if (sTextureInfo.iOffsets[0] == 0 || sTextureInfo.iOffsets[1] == 0 || sTextureInfo.iOffsets[2] == 0 || sTextureInfo.iOffsets[3] == 0) {
					Logger::GetInstance()->AddMessage(E_WARNING, "Found WAD texture reference in BSP, but doesn't appear to be loaded:", sTextureInfo.szName);
					break;
				}

				inFile.seekg(iOffsets[i] + sTextureInfo.iOffsets[mip], std::ios::beg);
				inFile.read((char*)dataDr, sTextureInfo.iWidth * sTextureInfo.iHeight / dimensionsSquared[mip]);

				if(mip == 3) {
					// Read the pallete (comes after last mipmap).
					uint16_t dummy;
					inFile.read((char*)&dummy, 2);
					inFile.read((char*)dataPal, 256 * 3);
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

				videosystem->AddTexture(mip, sTextureInfo.iWidth / dimensions[mip], sTextureInfo.iHeight / dimensions[mip], dataUp, false);
			}

			this->m_vLoadedTextures[sTextureInfo.szName] = sTexture;

			/* If mipmap offsets = 0
			Create new texture in its place with dummy values.
			Texture n;
			n.iTextureId = 0;
			n.iWidth = 1;
			n.iHeight = 1;
			textures[sTextureInfo.szName]=n;
			*/
		}
	}

	delete[] dataDr;
	delete[] dataUp;
	delete[] dataPal;

	return 0;
}//end TextureLoader::LoadTextures()


/**
 * Constructor. Private for singleton.
 */
TextureLoader::TextureLoader()
{
}//end TextureLoader::TextureLoader()
