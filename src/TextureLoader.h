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
#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include "util/MemoryDebugging.h"
#include <vector>
#include <string>
#include <fstream>
#include <map>

class VideoSystem;

/**
 * Holds basic data on each texture object.
 */
#define MAXTEXTURENAME 16
#define MIPLEVELS      4
struct TextureInfo
{
	char     szName[MAXTEXTURENAME]; /** Name of the texture. */
	uint32_t iWidth;                 /** Texture width. */
	uint32_t iHeight;                /** Texture height. */
	uint32_t iOffsets[MIPLEVELS];    /** Offsets to texture mipmaps. */
};//end WadTextureInfo


/**
 * Hold basic data on each loaded texture.
 */
struct Texture
{
	unsigned int iTextureId; /** ID used internally. */
	unsigned int iWidth;     /** Texture width. */
	unsigned int iHeight;    /** Texture height. */
};//end WadTexture


class TextureLoader
{
public:
	/** Get the singleton instance. */
	static TextureLoader* GetInstance();

	/** Destructor. */
	~TextureLoader();

	/**
	 * Load textures from a WAD file.
	 * \param szGamePaths A vector of gamepath strings.
	 * \param iOffsets    Array of texture offsets.
	 * \param inFile      File stream (already opened).
	 * \param videosystem Pointer to the videosystem object.
	 */
	int LoadTextures(const unsigned int &iNumberOfTextures, const uint32_t* iOffsets, std::ifstream &inFilfe, VideoSystem* videosystem);

	std::map<std::string, Texture> m_vLoadedTextures; /** Map of loaded textures, accessible by name. */

private:
	/* Constructor. Private for singleton. */
	TextureLoader();

};//end TextureLoader

#endif //TEXTURELOADER_H
