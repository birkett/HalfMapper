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
#ifndef HALFMAPPER_H
#define HALFMAPPER_H

#include <vector>

// Forward declarations.
class ConfigXML;
class VideoSystem;
class BSP;


/**
 * Main application object.
 */
class HalfMapper
{
public:
	/** Constructor. */
	HalfMapper();

	/** Destructor. **/
	~HalfMapper();

	/**
	 * Run the application.
	 * \param iArgc  Number of program arguments.
	 * \param szArgv Array of arguments.
	 */
	int Run(int iArgc, char* szArgv[]);

private:
	/** Load WAD's from the configured gamepaths. */
	int  LoadTextures();

	/** Load BSP's from the configured gamepaths. */
	void LoadMaps();

	/** Program main loop. */
	void MainLoop();

	ConfigXML*        m_XMLConfiguration; /** Loaded configuration data. */
	VideoSystem*      m_VideoSystem;      /** Pointer to the video system */
	std::vector<BSP*> m_LoadedMaps;       /** Vector of loaded maps. */

};//end HalfMapper

#endif //HALFMAPPER_H
