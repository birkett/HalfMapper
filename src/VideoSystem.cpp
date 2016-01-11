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
#include <iostream>
#include <SDL.h>
#include <GL/glew.h>
#include "VideoSystem.h"

/**
 * Set the basic configuration of the window and renderer.
 * \param iWidth         Window width.
 * \param iHeight        Window height.
 * \param fFox           Field of View angle.
 * \param bFullscreen    Fullscreen or Windowed mode.
 * \param bMultisampling Enable or disable multisampling.
 * \param bVsync         Enable or disable Vsync.
 */
VideoSystem::VideoSystem(int iWidth, int iHeight, float fFov, bool bFullscreen, bool bMultisampling, bool bVsync)
{
	m_iWidth         = iWidth;
	m_iHeight        = iHeight;
	m_fFov           = fFov;
	m_bFullscreen    = bFullscreen;
	m_bMultisampling = bMultisampling;
	m_bVsync         = bVsync;

}//end VideoSystem::VideoSystem()


/**
 * Cleanup the renderer.
 */
VideoSystem::~VideoSystem()
{
	SDL_GL_DeleteContext(this->sdlGLContext);
	SDL_DestroyWindow(this->sdlWindow);

}//end VideoSystem::~VideoSystem()


/**
 * Create the window and GL context.
 */
int VideoSystem::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "Can't initialize SDL." << std::endl;
		return -1;
	}

	unsigned int windowflags = SDL_WINDOW_OPENGL;

	if (this->m_bFullscreen) {
		windowflags |= SDL_WINDOW_FULLSCREEN;
	}

	this->SetMultisampling(this->m_bMultisampling); // Setup multisampling before creating the window.

	this->sdlWindow = SDL_CreateWindow("HalfMapper (loading maps, please wait)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->m_iWidth, this->m_iHeight, windowflags);

	if (this->sdlWindow == NULL) {
		std::cerr << "Can't create SDL window." << std::endl;
		return -1;
	}

	this->sdlGLContext = SDL_GL_CreateContext(this->sdlWindow);

	if (this->sdlGLContext == NULL) {
		std::cerr << "Can't create OpenGL Context." << std::endl;
		return -1;
	}

	SDL_GL_MakeCurrent(this->sdlWindow, this->sdlGLContext);

	if (glewInit() != GLEW_OK) {
		std::cerr << "Can't initialize Glew." << std::endl;
		return -1;
	}

	SDL_SetRelativeMouseMode(SDL_TRUE);

	this->SetVsync(this->m_bVsync); // Set Vsync after creating the GL context.

	this->SetupViewport();

	return 0;

}//end VideoSystem::Init()


/**
 * Clear the buffer.
 */
void VideoSystem::ClearBuffer()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}//end VideoSystem::ClearBuffer()


/**
 * Swap the buffers (double buffering).
 */
void VideoSystem::SwapBuffers()
{
	SDL_GL_SwapWindow(this->sdlWindow);

}//end VideoSystem::SwapBuffers()


/**
 * Setup the camera, in standard, or isometric mode.
 * \param bIsometric Set isometric mode.
 * \param fPosition  Position of the camera.
 * \param fRotation  Angle of the camera.
 * \param fIsoBounds Orthographic boundaries.
 */
void VideoSystem::SetCamera(bool bIsometric, float fPosition[3], float fRotation[2], float fIsoBounds)
{
	if (bIsometric) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-fIsoBounds, fIsoBounds, -fIsoBounds, fIsoBounds, -100000.0, 100000.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1.0f, 2.0f, 1.0f);
		glRotatef(fRotation[1], 1.0f, 0.0f, 0.0f);
		glRotatef(fRotation[0], 0.0f, 1.0f, 0.0f);
		glTranslatef(-fPosition[0], -fPosition[1], -fPosition[2]);
	}
	else {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotated(fRotation[1], 1.0f, 0.0f, 0.0f);
		glRotated(fRotation[0], 0.0f, 1.0f, 0.0f);
		glTranslatef(-fPosition[0], -fPosition[1], -fPosition[2]);
	}

}//end VideoSystem::SetCamera()


/**
 * Set the title of the program window.
 * \param szTitle New window title.
 */
void VideoSystem::SetWindowTitle(const char* szTitle)
{
	SDL_SetWindowTitle(this->sdlWindow, szTitle);

}//end VideoSystem::SetWindowTitle()


/**
 * Set the perspective of the viewport, and set some GL hints.
 */
void VideoSystem::SetupViewport()
{
	glViewport(0, 0, this->m_iWidth, this->m_iHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
		gluPerspective(this->m_fFov, (float)this->m_iWidth / (float)this->m_iHeight, 20.0f, 50000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.8f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glEnable(GL_TEXTURE_2D);

}//end VideoSystem::SetupViewport()


/**
 * Enable or disable multisampling for the GL context.
 * \param bEnable Enable or Disable.
 */
void VideoSystem::SetMultisampling(bool bEnable)
{
	if (bEnable) {
		glEnable(GL_MULTISAMPLE);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	else {
		glDisable(GL_MULTISAMPLE);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	this->m_bMultisampling = bEnable;

}//end VideoSystem::SetMultiSampling()


/**
 * Enable or disable Vsync for the GL context.
 * \param bEnable Enable or Disable.
 */
void VideoSystem::SetVsync(bool bEnable)
{
	bEnable ? SDL_GL_SetSwapInterval(1) : SDL_GL_SetSwapInterval(0);
	this->m_bVsync = bEnable;

}//end VideoSystem::SetVsync()
