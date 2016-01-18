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
VideoSystem::VideoSystem(const int iWidth, const int iHeight, const float fFov, const bool bFullscreen, const bool bMultisampling, const bool bVsync)
{
	m_iWidth         = iWidth;
	m_iHeight        = iHeight;
	m_fFov           = fFov;
	m_bFullscreen    = bFullscreen;
	m_bMultisampling = bMultisampling;
	m_bVsync         = bVsync;
	m_iFrameStartMs  = SDL_GetTicks();

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
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't initialize SDL.");
		return -1;
	}

	unsigned int windowflags = SDL_WINDOW_OPENGL;

	if (this->m_bFullscreen) {
		windowflags |= SDL_WINDOW_FULLSCREEN;
	}

	this->SetMultisampling(this->m_bMultisampling); // Setup multisampling before creating the window.

	this->sdlWindow = SDL_CreateWindow("HalfMapper (loading maps, please wait)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->m_iWidth, this->m_iHeight, windowflags);

	if (this->sdlWindow == NULL) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't create SDL window.");
		return -1;
	}

	this->sdlGLContext = SDL_GL_CreateContext(this->sdlWindow);

	if (this->sdlGLContext == NULL) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't create OpenGL context.");
		return -1;
	}

	SDL_GL_MakeCurrent(this->sdlWindow, this->sdlGLContext);

	if (glewInit() != GLEW_OK) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't initialize GLEW.");
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
void VideoSystem::SetCamera(const bool bIsometric, const Point3f fPosition, const Point2f fRotation, const float fIsoBounds)
{
	if (bIsometric) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-fIsoBounds, fIsoBounds, -fIsoBounds, fIsoBounds, -100000.0, 100000.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1.0f, 2.0f, 1.0f);
		glRotatef(fRotation.m_fY, 1.0f, 0.0f, 0.0f);
		glRotatef(fRotation.m_fX, 0.0f, 1.0f, 0.0f);
		glTranslatef(-fPosition.m_fX, -fPosition.m_fY, -fPosition.m_fZ);
	}
	else {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotated(fRotation.m_fY, 1.0f, 0.0f, 0.0f);
		glRotated(fRotation.m_fX, 0.0f, 1.0f, 0.0f);
		glTranslatef(-fPosition.m_fX, -fPosition.m_fY, -fPosition.m_fZ);
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
 * Create a new texture in video memory, and return it's ID.
 * \param bIsLightmap Are we building a lightmap, or a normal mipmapped texture.
 */
unsigned int VideoSystem::CreateTexture(const bool &bIsLightmap)
{
	unsigned int iTextureID = 0;

	glGenTextures(1, &iTextureID);
	glBindTexture(GL_TEXTURE_2D, iTextureID);

	// Slightly different create params when building a lightmap vs building a mipmapped texture.
	if (!bIsLightmap) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return iTextureID;

}//end VideoSystem::CreateTexture()


 /**
  * Generate a texture in video memory.
  * \param iMipLevel    Which scale of mipmap this texture is.
  * \param iWidth       Texture width.
  * \param iHeight      Texture height.
  * \param pixels       Raw pixel data.
  * \param bIsLightmap  Lightmaps are RGB, normal textures RGBA.
  */
void VideoSystem::AddTexture(const unsigned int &iMipLevel, const unsigned int &iWidth, const unsigned int &iHeight, uint8_t* pixels, bool bIsLightmap)
{
	unsigned int iFormatFlag = GL_RGBA;

	if (bIsLightmap) {
		iFormatFlag = GL_RGB;
	}

	glTexImage2D(GL_TEXTURE_2D, iMipLevel, iFormatFlag, iWidth, iHeight, 0, iFormatFlag, GL_UNSIGNED_BYTE, pixels);

	if (glGetError() != GL_NO_ERROR) {
		Logger::GetInstance()->AddMessage(E_ERROR, "GL texture loading error, code:", glGetError());
	}

}//end VideoSystem::AddTexture()


/**
 * Create a new object buffer.
 * \param iBufferSize Total size of the buffer.
 * \param objects     Data.
 */
void VideoSystem::CreateBuffer(const size_t &iBufferSize, unsigned int* objects)
{
	glGenBuffers((int)iBufferSize, objects);

}//end VideoSyetem::CreateBuffer()


/**
 * Add data to the created buffer.
 * \param objects   Buffer objects.
 * \param iDataSize Total size of the data to add.
 * \param data      Raw data.
 */
void VideoSystem::AddDataToBuffer(const unsigned int objects, const size_t &iDataSize, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, objects);
	glBufferData(GL_ARRAY_BUFFER, iDataSize, data, GL_STATIC_DRAW);

}//end VideoSystem::AddDataToBuffer()


/**
 * Begin a frame, by translating to the correct poisiton and setting the global lightmap.
 * \param x          X translation.
 * \param y          Y translation.
 * \param z          Z translation.
 * \param iTextureId Lightmap ID.
 */
void VideoSystem::BeginFrame(const float &x, const float &y, const float &z, const unsigned int iTextureId)
{
	glPushMatrix();
	glTranslatef(x, y, z);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, iTextureId);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

}//end VideoSystem::BeginFrame()


/**
 * Push data to be rendered.
 * \param objects        Data points.
 * \param iTextureId     Texture ID to use.
 * \param iDataSize      Size of each data point.
 * \param iDataArraySize Total size of the data array.
 */
void VideoSystem::PushData(const unsigned int objects, const unsigned int iTextureId, const unsigned int iDataSize, const size_t iDataArraySize)
{
	glBindBuffer(GL_ARRAY_BUFFER, objects);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, iTextureId);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, iDataSize, (char*)NULL + 4 * 3);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2, GL_FLOAT, iDataSize, (char*)NULL + 4 * 5);

	glVertexPointer(3, GL_FLOAT, iDataSize, (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, (int)iDataArraySize);

}//end VideoSystem::PushData()


/**
 * Finish a frame.
 */
void VideoSystem::EndFrame()
{
	glPopMatrix();

}//end VideoSystem::EndFrame()


/**
 * Calculate and return the current frame rate.
 */
float VideoSystem::GetFPS()
{
	int iFrameDelta = SDL_GetTicks() - this->m_iFrameStartMs;
	this->m_iFrameStartMs = SDL_GetTicks();
	float fFPS = 1000 / (float)iFrameDelta;

	return fFPS;

}//end VideoSystem::GetFPS()


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
void VideoSystem::SetMultisampling(const bool bEnable)
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
void VideoSystem::SetVsync(const bool bEnable)
{
	bEnable ? SDL_GL_SetSwapInterval(1) : SDL_GL_SetSwapInterval(0);
	this->m_bVsync = bEnable;

}//end VideoSystem::SetVsync()
