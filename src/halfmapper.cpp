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
#include <iostream>
#include <algorithm>
#include <SDL.h>
#include "halfmapper.h"
#include "VideoSystem.h"
#include "TextureLoader.h"
#include "bsp.h"
#include "ConfigXML.h"


/**
 * Constructor.
 */
HalfMapper::HalfMapper()
{
	this->m_bShouldQuit  = false;
	this->m_fIsoBounds   = 1000.0;

	this->m_fPosition = new Point3f();
	this->m_fRotation = new Point2f();

}//end HalfMapper::HalfMapper()


/**
 * Destructor. Clean up obects and clear vectors.
 */
HalfMapper::~HalfMapper()
{
	m_LoadedMaps.clear();
	delete this->m_TextureLoader;
	delete this->m_VideoSystem;
	delete this->m_XMLConfiguration;
	delete this->m_fPosition;
	delete this->m_fRotation;

}//end HalfMapper::~HalfMapper()


/**
 * Run the application.
 * \param iArgc  Number of program arguments.
 * \param szArgv Array of arguments.
 */
int HalfMapper::Run(const int iArgc, char* szArgv[])
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
	this->m_TextureLoader = new TextureLoader();

	for (size_t i = 0; i < this->m_XMLConfiguration->m_vWads.size(); i++) {
		if (this->m_TextureLoader->LoadTexturesFromWAD(this->m_XMLConfiguration->m_szGamePaths, this->m_XMLConfiguration->m_vWads[i] + ".wad") != 0) {
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
				BSP *level = new BSP(this->m_XMLConfiguration->m_szGamePaths, sMapEntry);
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
 * Handle keyboard, mouse and event input.
 */
void HalfMapper::InputLoop()
{
	static const Uint8* keystate = nullptr;
	static float        fLeft    = 0.0f;
	static float        fFrontal = 0.0f;
	static int          iSpeed   = 8;

	// Reset these on every loop back to defaults.
	fLeft    = 0.0f;
	fFrontal = 0.0f;
	iSpeed   = 8;

	// Handle movement with the keyboard state. Cleaner than checking SDL_KEYUP and SDL_KEYDOWN.
	keystate = SDL_GetKeyboardState(NULL);

	if (keystate[SDL_SCANCODE_W]) { fFrontal++; }
	if (keystate[SDL_SCANCODE_S]) { fFrontal--; }
	if (keystate[SDL_SCANCODE_A]) { fLeft++;    }
	if (keystate[SDL_SCANCODE_D]) { fLeft--;    }

	if (keystate[SDL_SCANCODE_LCTRL])  { iSpeed = 2; }
	if (keystate[SDL_SCANCODE_LSHIFT]) { iSpeed = 32; }

	if (keystate[SDL_SCANCODE_Q]) {
		this->m_XMLConfiguration->m_bIsometric ? (this->m_fIsoBounds -= iSpeed * 10.0f) : (this->m_fPosition->m_fY -= iSpeed);
	}
	if (keystate[SDL_SCANCODE_E]) {
		this->m_XMLConfiguration->m_bIsometric ? (this->m_fIsoBounds += iSpeed * 10.0f) : (this->m_fPosition->m_fY += iSpeed);
	}

	// Handle mouse movement and events.
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				this ->m_bShouldQuit = true; break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					this->m_bShouldQuit = true;
				}
				break;

			case SDL_MOUSEMOTION:
				m_fRotation->m_fX += event.motion.xrel / 15.0f;
				m_fRotation->m_fY += event.motion.yrel / 15.0f;
				break;
		}//end switch(event.type)
	}//end while

	// Clamp mouse aim.
	if (m_fRotation->m_fX > 360) { m_fRotation->m_fX -= 360.0; }
	if (m_fRotation->m_fX < 0  ) { m_fRotation->m_fX += 360.0; }
	if (m_fRotation->m_fY < -89) { m_fRotation->m_fY  = -89;   }
	if (m_fRotation->m_fY > 89 ) { m_fRotation->m_fY  = 89;    }

	// Calculate camera position.
	if (this->m_XMLConfiguration->m_bIsometric) {
		m_fPosition->m_fZ += fLeft    * iSpeed;
		m_fPosition->m_fY += fFrontal * iSpeed;
		m_fIsoBounds = max(10.0f, m_fIsoBounds);
	}
	else {
		if (fFrontal || fLeft) {
			float rotationF = this->m_fRotation->m_fX * (float)M_PI / 180.0f + atan2(fFrontal, fLeft);
			m_fPosition->m_fX -= iSpeed * cos(rotationF);
			m_fPosition->m_fZ -= iSpeed * sin(rotationF);
		}
	}

}//end HalfMapper::InputLoop()


/**
 * Main program loop.
 */
void HalfMapper::MainLoop()
{
	int iFrameStartTime = SDL_GetTicks();
	int iFrame          = 0;

	while (!this->m_bShouldQuit) {
		this->InputLoop();
		this->m_VideoSystem->ClearBuffer();

		this->m_VideoSystem->SetCamera(this->m_XMLConfiguration->m_bIsometric, this->m_fPosition, this->m_fRotation, this->m_fIsoBounds);

		// Render each loaded map.
		for (size_t i = 0; i < this->m_LoadedMaps.size(); i++) {
			this->m_LoadedMaps[i]->render();
		}

		this->m_VideoSystem->SwapBuffers();

		iFrame++;
		if (iFrame == 30) {
			iFrame = 0;

			// Calculate framerate.
			int iFrameDelta = SDL_GetTicks() - iFrameStartTime;
			iFrameStartTime = SDL_GetTicks();
			char bf[64];
			sprintf(bf, "%.2f FPS - %.2f %.2f %.2f", 30000.0f / (float)iFrameDelta, this->m_fPosition->m_fX, this->m_fPosition->m_fY, this->m_fPosition->m_fZ);
			this->m_VideoSystem->SetWindowTitle(bf);
		}
	}//end while

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
