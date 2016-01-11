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
#include "halfmapper.h"
#include "VideoSystem.h"
#include "wad.h"
#include "bsp.h"
#include "ConfigXML.h"


/**
 * Constructor.
 */
HalfMapper::HalfMapper()
{

}//end HalfMapper::HalfMapper()


/**
 * Destructor. Clean up obects and clear vectors.
 */
HalfMapper::~HalfMapper()
{
	m_LoadedMaps.clear();
	delete this->m_VideoSystem;
	delete this->m_XMLConfiguration;

}//end HalfMapper::~HalfMapper()


/**
 * Run the application.
 * \param iArgc  Number of program arguments.
 * \param szArgv Array of arguments.
 */
int HalfMapper::Run(int iArgc, char* szArgv[])
{
	this->m_XMLConfiguration = new ConfigXML();

	this->m_XMLConfiguration->LoadProgramConfig();

	(iArgc >= 2) ? this->m_XMLConfiguration->LoadMapConfig(szArgv[1]) : this->m_XMLConfiguration->LoadMapConfig("halflife.xml");

	this->m_VideoSystem = new VideoSystem(
		this->m_XMLConfiguration->m_iWidth,
		this->m_XMLConfiguration->m_iHeight,
		this->m_XMLConfiguration->m_fFov,
		this->m_XMLConfiguration->m_bFullscreen,
		this->m_XMLConfiguration->m_bMultisampling,
		this->m_XMLConfiguration->m_bVsync
	);

	if (this->m_VideoSystem->Init() != 0) {
		return -1;
	}

	if (this->LoadTextures() != 0) {
		return -1;
	}

	this->LoadMaps();

	this->MainLoop();

	return 0;

}//end HalfMapper::Run()


/*
 * Load WAD's from the configured gamepaths.
 */
int HalfMapper::LoadTextures()
{
	for (size_t i = 0; i < this->m_XMLConfiguration->m_vWads.size(); i++) {
		if (wadLoad(this->m_XMLConfiguration->m_szGamePaths, this->m_XMLConfiguration->m_vWads[i] + ".wad") != 0) {
			return -1;
		}
	}

	return 0;

}//end HalfMapper::LoadTextures()


/*
 * Load BSP's from the configured gamepaths.
 */
void HalfMapper::LoadMaps()
{
	int ticks          = SDL_GetTicks();
	int mapCount       = 0;
	int mapRenderCount = 0;
	int totalTris      = 0;

	for (size_t i = 0; i < this->m_XMLConfiguration->m_vChapterEntries.size(); i++) {
		for (size_t j = 0; j < this->m_XMLConfiguration->m_vChapterEntries[i].m_vMapEntries.size(); j++) {
			ChapterEntry sChapterEntry = this->m_XMLConfiguration->m_vChapterEntries[i];
			MapEntry sMapEntry = this->m_XMLConfiguration->m_vChapterEntries[i].m_vMapEntries[j];

			if (sChapterEntry.m_bRender && sMapEntry.m_bRender) {
				BSP *level = new BSP(this->m_XMLConfiguration->m_szGamePaths, "maps/" + sMapEntry.m_szName + ".bsp", sMapEntry);
				level->SetChapterOffset(sChapterEntry.m_fOffsetX, sChapterEntry.m_fOffsetY, sChapterEntry.m_fOffsetZ);
				totalTris += level->totalTris;
				this->m_LoadedMaps.push_back(level);
				mapRenderCount++;
			}

			mapCount++;
		}
	}

	cout << mapCount << " maps found in config file." << endl;
	cout << mapRenderCount << " maps to render - loaded in " << SDL_GetTicks() - ticks << " ms." << endl;
	cout << "Total triangles: " << totalTris << endl;

}//end HalfMapper::LoadMaps()


/**
 * Main program loop.
 */
void HalfMapper::MainLoop()
{
	bool quit = false;
	int kw = 0, ks = 0, ka = 0, kd = 0, kq = 0, ke = 0, kr = 0, kc = 0;
	float position[3] = { 0.0f, 0.0f, 0.0f };
	float rotation[2] = { 0.0f, 0.0f };
	float isoBounds = 1000.0;
	int oldMs = SDL_GetTicks(), frame = 0;

	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) quit = true;
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
				if (event.key.keysym.sym == SDLK_w) kw = 1;
				if (event.key.keysym.sym == SDLK_s) ks = 1;
				if (event.key.keysym.sym == SDLK_a) ka = 1;
				if (event.key.keysym.sym == SDLK_d) kd = 1;
				if (event.key.keysym.sym == SDLK_q) kq = 1;
				if (event.key.keysym.sym == SDLK_e) ke = 1;

				if (event.key.keysym.sym == SDLK_LSHIFT) kr = 1;
				if (event.key.keysym.sym == SDLK_LCTRL) kc = 1;
			}
			if (event.type == SDL_KEYUP) {
				if (event.key.keysym.sym == SDLK_w) kw = 0;
				if (event.key.keysym.sym == SDLK_s) ks = 0;
				if (event.key.keysym.sym == SDLK_a) ka = 0;
				if (event.key.keysym.sym == SDLK_d) kd = 0;
				if (event.key.keysym.sym == SDLK_q) kq = 0;
				if (event.key.keysym.sym == SDLK_e) ke = 0;

				if (event.key.keysym.sym == SDLK_LSHIFT) kr = 0;
				if (event.key.keysym.sym == SDLK_LCTRL) kc = 0;
			}
			if (event.type == SDL_MOUSEMOTION) {
				rotation[0] += event.motion.xrel / 15.0f;
				rotation[1] += event.motion.yrel / 15.0f;
			}
		}

		//Clamp mouse aim
		if (rotation[0] > 360) rotation[0] -= 360.0;
		if (rotation[0] < 0) rotation[0] += 360.0;
		if (rotation[1] < -89) rotation[1] = -89;
		if (rotation[1] > 89) rotation[1] = 89;

		//Velocities
		int vsp = kr ? 32 : (kc ? 2 : 8), hsp = kr ? 32 : (kc ? 2 : 8);

		float m_left = 0.0f, m_frontal = 0.0f;

		if (kw) m_frontal++;
		if (ks) m_frontal--;
		if (ka) m_left++;
		if (kd) m_left--;

		if (this->m_XMLConfiguration->m_bIsometric) {
			position[2] += m_left * hsp;
			position[1] += m_frontal * hsp;

			if (ke) isoBounds += vsp * 10.0f;
			if (kq) isoBounds -= vsp * 10.0f;
			isoBounds = max(10.0f, isoBounds);
		}
		else {
			if (m_frontal || m_left) {
				float rotationF = rotation[0] * (float)M_PI / 180.0f + atan2(m_frontal, m_left);
				position[0] -= hsp * cos(rotationF);
				position[2] -= hsp * sin(rotationF);
			}
			if (ke) position[1] += vsp;
			if (kq) position[1] -= vsp;
		}

		this->m_VideoSystem->ClearBuffer();

		this->m_VideoSystem->SetCamera(this->m_XMLConfiguration->m_bIsometric, position, rotation, isoBounds);

		//Map render
		for (size_t i = 0; i < this->m_LoadedMaps.size(); i++) {
			this->m_LoadedMaps[i]->render();
		}

		this->m_VideoSystem->SwapBuffers();

		frame++;
		if (frame == 30) {
			frame = 0;

			//FPS calculation
			int dt = SDL_GetTicks() - oldMs;
			oldMs = SDL_GetTicks();
			char bf[64];
			sprintf(bf, "%.2f FPS - %.2f %.2f %.2f", 30000.0f / (float)dt, position[0], position[1], position[2]);
			this->m_VideoSystem->SetWindowTitle(bf);
		}
	}

	SDL_Quit();

}//end HalfMapper::MainLoop()


/**
 * Program entry point.
 * \param argc Number of program arguments.
 * \param argv Array of arguments.
 */
int main(int argc, char** argv)
{
	HalfMapper application;

	return application.Run(argc, argv);

}//end main()
