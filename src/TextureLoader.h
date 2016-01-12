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

#include <vector>
#include <string>
#include <fstream>
#include <map>

// References:
// http://hlbsp.sourceforge.net/index.php?content=waddef
// http://nemesis.thewavelength.net/index.php?p=35

/**
 * First 12 bytes of a WAD are the header.
 */
struct WADHeader
{
	char     szMagic[4];  /** Magic signature (WAD2 or WAD3). */
	uint32_t iLumpCount;  /** Number of lump entries. */
	uint32_t iLumpOffset; /** Offset to the lump. */
};//end WadHeader


/**
 * Each object in the WAD has a 32 byte entry after the WAD header.
 */
struct WADEntry
{
	uint32_t iOffset;      /** Object position in the WAD. */
	uint32_t iDiskSize;    /** Object size in WAD. */
	uint32_t iSize;        /** Uncompressed object size. */
	uint8_t  iType;        /** Type of object. */
	bool     bCompression; /** Compression. */
	uint8_t  iPadding0;    /** Unused. */
	uint8_t  iPadding1;    /** Unused. */
	char     szName[16];   /** Object name, null terminated. */
};//end WadEntry


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
	TextureLoader();
	~TextureLoader();

	int LoadTexturesFromWAD(const std::vector<std::string> &szGamePaths, const std::string &szFilename);

private:
	/**
	 * Check if a loaded WAD header is valid.
	 * \param sHeader Loaded WAD header structure.
	 */
	bool IsValidWADHeader(const WADHeader &sHeader);

	std::ifstream                  m_sWadFile;        /** File stream to load the WAD. */
	std::map<std::string, Texture> m_vLoadedTextures; /** Map of loaded textures, accessible by name. */

};//end TextureLoader

#endif //TEXTURELOADER_H
