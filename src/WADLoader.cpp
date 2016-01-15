/*
 * halfmapper, a renderer for GoldSrc maps and chapters.
 *
 * Copyright(C) 2014  Gonzalo �vila "gzalo" Alterach
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
#include <iostream>
#include "WADLoader.h"
#include "TextureLoader.h"

/**
 * Constructor.
 */
WADLoader::WADLoader()
{

}//end WADLoader::WADLoader()


/**
 * Destructor.
 */
WADLoader::~WADLoader()
{

}//end WADLoader::~WADLoader()


/**
 * Load textures from a WAD file.
 * \param szGamePaths A vector of gamepath strings.
 * \param szFilename  Filename to load.
 * \param videosystem Pointer to the videosystem.
 */
int WADLoader::LoadTexturesFromWAD(const std::vector<std::string> &szGamePaths, const std::string &szFilename, VideoSystem* videosystem)
{
	// Try to open the file from all known gamepaths.
	for (size_t i = 0; i < szGamePaths.size(); i++) {
		if (!this->m_sWadFile.is_open()) {
			this->m_sWadFile.open(szGamePaths[i] + szFilename.c_str(), std::ios::binary);
		}
	}

	// If the WAD wasn't found in any of the gamepaths...
	if (!this->m_sWadFile.is_open()) {
		std::cerr << "Can't load WAD " << szFilename << "." << std::endl;
		return -1;
	}

	// Read header.
	WADHeader sWadHeader; this->m_sWadFile.read((char*)&sWadHeader, sizeof(WADHeader));

	if (!this->IsValidWADHeader(sWadHeader)) {
		return -1;
	}

	// Read directory entries.
	WADEntry *sWadEntry = new WADEntry[sWadHeader.iLumpCount];

	this->m_sWadFile.seekg(sWadHeader.iLumpOffset, std::ios::beg);
	this->m_sWadFile.read((char*)sWadEntry, sizeof(WADEntry) * sWadHeader.iLumpCount);

	uint32_t* iOffsets = new uint32_t[sWadHeader.iLumpCount];

	for (uint32_t i = 0; i < sWadHeader.iLumpCount; i++) {
		iOffsets[i] = sWadEntry[i].iOffset;
	}

	TextureLoader::GetInstance()->LoadTextures(sWadHeader.iLumpCount, iOffsets, this->m_sWadFile, videosystem);

	this->m_sWadFile.close();

	delete[] sWadEntry;
	delete[] iOffsets;

	return 0;

}//end WADLoader::LoadTexturesFromWAD()


/**
 * Check if a loaded WAD header is valid.
 * \param sHeader Loaded WAD header structure.
 */
bool WADLoader::IsValidWADHeader(const WADHeader &sHeader)
{
	if (sHeader.szMagic[0] != 'W'
		|| sHeader.szMagic[1] != 'A'
		|| sHeader.szMagic[2] != 'D'
		|| sHeader.szMagic[3] != '3'
		) {
		return false;
	}

	return true;

}//end WADLoader::IsValidWADHeader()
