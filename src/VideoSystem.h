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
#ifndef VIDEO_H
#define VIDEO_H

#include "util/MemoryDebugging.h"
#include "CommonTypes.h"

// Avoid including SDL just for these basic types.
typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;


/**
 * Rendering and Window management.
 */
class VideoSystem
{
public:
	/**
	 * Set the basic configuration of the window and renderer.
	 * \param iWidth         Window width.
	 * \param iHeight        Window height.
	 * \param fFox           Field of View angle.
	 * \param bFullscreen    Fullscreen or Windowed mode.
	 * \param bMultisampling Enable or disable multisampling.
	 * \param bVsync         Enable or disable Vsync.
	 */
	VideoSystem(const int &iWidth, const int &iHeight, const float &fFov, const bool &bFullscreen, const bool &bMultisampling, const bool &bVsync);

	/** Destructor */
	~VideoSystem();

	/** Create the window and GL context. */
	int  Init();

	/** Clear the buffer. */
	void ClearBuffer();

	/** Swap the buffers (double buffering). */
	void SwapBuffers();

	/**
	 * Setup the camera, in standard, or isometric mode.
	 * \param bIsometric Set isometric mode.
	 * \param fPosition  Position of the camera.
	 * \param fRotation  Angle of the camera.
	 * \param fIsoBounds Orthographic boundaries.
	 */
	void SetCamera(const bool &bIsometric, const Point3f &fPosition, const Point2f &fRotation, const float &fIsoBounds);

	/**
	 * Set the title of the program window.
	 * \param szTitle New window title.
	 */
	void SetWindowTitle(const char *szTitle);

	/**
	 * Create a new texture in video memory, and return it's ID.
	 * \param bIsLightmap Are we building a lightmap, or a normal mipmapped texture.
	 */
	unsigned int CreateTexture(const bool &bIsLightmap);

	/**
	 * Generate a texture in video memory.
	 * \param iMipLevel    Which scale of mipmap this texture is.
	 * \param iWidth       Texture width.
	 * \param iHeight      Texture height.
	 * \param pixels       Raw pixel data.
	 * \param bIsLightmap  Lightmaps are RGB, normal textures RGBA.
	 */
	void AddTexture(const unsigned int &iMipLevel, const unsigned int &iWidth, const unsigned int &iHeight, uint8_t* pixels, bool bIsLightmap);

	/**
	 * Create a new object buffer.
	 * \param iBufferSize Total size of the buffer.
	 * \param objects     Data.
	 */
	void CreateBuffer(const size_t &iBufferSize, unsigned int* objects);

	/**
	 * Add data to the created buffer.
	 * \param objects   Buffer objects.
	 * \param iDataSize Total size of the data to add.
	 * \param data      Raw data.
	 */
	void AddDataToBuffer(const unsigned int &objects, const size_t &iDataSize, const void* data);

	/**
	 * Begin a frame, by translating to the correct poisiton and setting the global lightmap.
	 * \param x          X translation.
	 * \param y          Y translation.
	 * \param z          Z translation.
	 * \param iTextureId Lightmap ID.
	 */
	void BeginFrame(const float &x, const float &y, const float &z, const unsigned int &iTextureId);

	/**
	* Push data to be rendered.
	* \param objects        Data points.
	* \param iTextureId     Texture ID to use.
	* \param iDataSize      Size of each data point.
	* \param iDataArraySize Total size of the data array.
	*/
	void PushData(const unsigned int &objects, const unsigned int &iTextureId, const unsigned int &iDataSize, const size_t &iDataArraySize);

	/** Finish a frame. */
	void EndFrame();

	/** Calculate and return the current frame rate. */
	float GetFPS();

private:
	/** Set the perspective of the viewport, and set some GL hints. */
	void SetupViewport();

	/**
	 * Enable or disable multisampling for the GL context.
	 * \param bEnable Enable or Disable.
	 */
	void SetMultisampling(const bool &bEnable);

	/**
	* Enable or disable Vsync for the GL context.
	* \param bEnable Enable or Disable.
	*/
	void SetVsync(const bool &bEnable);

	unsigned int  m_iWidth;         /** Window width. */
	unsigned int  m_iHeight;        /** Window height. */
	float         m_fFov;           /** Field of View angle. */
	bool          m_bFullscreen;    /** Fullscreen or Windowed mode. */
	bool          m_bMultisampling; /** Multisampling enable or disable. */
	bool          m_bVsync;         /** Vsync enable or disable. */
	SDL_Window*   sdlWindow;        /** Pointer to the SDL Window. */
	SDL_GLContext sdlGLContext;     /** Hold the SDL OpenGL Context. */
	unsigned int  m_iFrameStartMs;  /** Frame start time, to work out the frame time delta. */

};//end VideoSystem

#endif //VIDEO_H
