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
#ifndef BSP_H
#define BSP_H

#include "util/MemoryDebugging.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>

// Formward declarations.
struct MapEntry;
struct Texture;
class  VideoSystem;

// References:
// http://hlbsp.sourceforge.net/index.php?content=bspdef
// http://nemesis.thewavelength.net/index.php?p=35

/**
 * List of lumps found in a valid v30 BSP.
 */
enum BSPLumpNames
{
	LUMP_ENTITIES = 0,
	LUMP_PLANES,
	LUMP_TEXTURES,
	LUMP_VERTICES,
	LUMP_VISIBILITY,
	LUMP_NODES,
	LUMP_TEXINFO,
	LUMP_FACES,
	LUMP_LIGHTING,
	LUMP_CLIPNODES,
	LUMP_LEAVES,
	LUMP_MARKSURFACES,
	LUMP_EDGES,
	LUMP_SURFEDGES,
	LUMP_MODELS,

	LUMP_COUNT, // 15 total lumps.

};//end BSPLumpNames


/**
 * Represent a 3D point in space.
 */
struct Vertex3f
{
	/** Constructor. */
	Vertex3f()
	{
		x = y = z = 0.0f;
	}

	/** Constructor. */
	Vertex3f(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	/** Convert between coordinate systems. */
	void FixHand()
	{
		float swapY = y;
		y = z;
		z = swapY;
		x = -x;
	}

	float x; /** X value. */
	float y; /** Y value. */
	float z; /** Z value. */

};//end Vertex3f


/**
 * Each lump in the BSP has an entry in the BSP header.
 */
struct BSPLump
{
	int32_t iOffset; /** Byte offsets to the lump. */
	int32_t iLength; /** Byte length of the lump. */

};//end BSPLump


/**
 * The BSP header. Contains the BSP version, and the lump entries.
 */
struct BSPHeader
{
	int32_t iVersion;         /** BSP version. 30 for Half-Life. */
	BSPLump lump[LUMP_COUNT]; /** Lump entries. */

};//end BSPHeader


/** 
 * BSP Face entry.
 */
struct BSPFace
{
	uint16_t iPlane;          /** Plane the face is parallel to. */
	uint16_t iPlaneSide;      /** Set if different normals orientation. */
	uint32_t iFirstEdge;      /** Index of the first surfedge. */
	uint16_t iEdges;          /** Number of consecutive surfedges. */
	uint16_t iTextureInfo;    /** Index of the texture info structure. */
	uint8_t  iStyles[4];      /** Specify lighting styles. */
	int32_t  iLightmapOffset; /** Offsets into the raw lightmap data. */

};//end BSPFace


/**
 * BSP Edge entry.
 */
struct BSPEdge
{
	uint16_t iVertex3f[2]; /** Indices into Vertex3f array. */

};//end BSPEdge


/**
 * BSP texture information entry.
 */
struct BSPTextureInfo
{
	Vertex3f vS;      /** S shift vector. */
	float    fSShift; /** Texture shift in S direction. */
	Vertex3f vT;      /** T shift vector. */
	float    fTShift; /** Texture shift in T direction. */
	uint32_t iMiptex; /** Index into textures array. */
	uint32_t nFlags;  /** Texture flags, seem to always be 0. */

};//end BSPTextureInfo


/**
 * BSP texture header.
 */
struct BSPTextureHeader
{
	uint32_t nMipTextures; /** Number of BSPMIPTEX structures. */

};//end BSPTextureHeader


/**
 * BSP model entry.
 */
struct BSPModel
{
	#define MAX_MAP_HULLS 4

	float    fMins[3];                  /** Bounding box minumum size. */
	float    fMaxs[3];                  /** Bounding box maximum size. */
    Vertex3f vOrigin;                   /** Model origin. */
    int32_t  iHeadnodes[MAX_MAP_HULLS]; /** Index into nodes array. */
    int32_t  nVisLeafs;                 /** ??? */
	int32_t  iFirstFace;                /** Face index in the faces lump. */
	int32_t  nFaces;                    /** Number of faces. */

};//end BSPModel


/**
 * UV coordinates.
 */
struct UVCoordinates
{
	float u; /** U (X) position. */
	float v; /** V (Y) position. */

};//end UVCoordinates


/**
 * Final vector, with UV coordinates.
 */
struct VectorFinal
{
	float x;
	float y;
	float z;
	float u;
	float v;
	float ul;
	float vl;

	/** Constructor. */
	VectorFinal(float _x, float _y, float _z, float _u, float _v)
	{
		x = _x;
		y = _y;
		z = _z;
		u = _u;
		v = _v;
	}

	/** Constructor. */
	VectorFinal(Vertex3f vt, UVCoordinates c, UVCoordinates c2)
	{
		x = vt.x, y = vt.y, z = vt.z;
		u = c.u, v = c.v;
		ul = c2.u;
		vl = c2.v;
	}

};//end VectorFinal


/**
 * BSP Lightmap.
 */
struct Lightmap
{
	unsigned char *offset;   /** Lightmap offset. */
	int           m_iWidth;  /** Lightmap width. */
	int           m_iHeight; /** Lightmap height. */
	int           m_iFinalX; /** Lightmap X position. */
	int           m_iFinalY; /** Lightmap Y position. */

};//end Lightmap


/**
 * List of points, stored with their associated texture ID.
 */
struct TrianglePointMap
{
	std::vector<VectorFinal> triangles; /** List of points. */
	int                      texId;     /** Texture ID. */

};//end TrianglePointMap


/**
 * Load a BSP file.
 */
class BSP
{
public:
	/**
	 * Contructor.
	 * \param szGamePaths List of game paths to attempt loading from.
	 * \param sMapEntry   MapEntry structure, containing config data for the current map.
	 * \param videosystem Pointer to the video system.
	 */
	BSP(const std::vector<std::string> &szGamePaths, const MapEntry &sMapEntry, VideoSystem* &videosystem);

	/** Destructor. */
	~BSP();

	/** Render this map. */
	void Render();

	/**
	 * Set the offset, common to all maps in the same chapter.
	 * \param x X offset.
	 * \param y Y offset.
	 * \param z Z offset.
	 */
	void SetChapterOffset(const float x, const float y, const float z);


	size_t totalTris; /** Total number of loaded triangles. */

private:
	/**
	 * Calculate correct coordinates.
	 * \param v      Base position vector.
	 * \param vs     S position vector.
	 * \param vt     T position vector.
	 * \param sShift Offset in the S direction.
	 * \param tShift Offset in the T direction.
	 */
	inline UVCoordinates CalculateCoordinates(Vertex3f v, Vertex3f vs, Vertex3f vt, float sShift, float tShift);

	/** Calculate the map offset, using landmarks + chater and single map offsets. */
	void CalculateOffset();

	/**
	 * Check if this is a valid v30 BSP.
	 * \param sHeader BSP Header.
	 */
	bool IsValidBSPHeader(const BSPHeader &sHeader);

	/**
	 * Parse the entitities in the map.
	 * \param sHeader   BSP Header.
	 * \param sMapEntry MapEntry structure.
	 */
	void ParseEntities(const BSPHeader &sHeader, const MapEntry &sMapEntry);

	/**
	 * Load the faces and lightmaps.
	 * \param sHeader BSP Header.
	 */
	void LoadFacesAndLightMaps(const BSPHeader &sHeader);

	/** Generate the lightmap. */
	void GenerateLightMaps();

	/**
	 * Load models from the map.
	 * \param sHeader BSP Header.
	 */
	void LoadModels(const BSPHeader &sHeader);

	/**
	 * Load surfaces and edges.
	 * \param sHeader BSP Header.
	 */
	void LoadSurfEdges(const BSPHeader &sHeader);

	/**
	 * Load textures stored in the map.
	 * \param sHeader BSP Header.
	 */
	void LoadTextures(const BSPHeader &sHeader);

	/**
	 * Load the actual map geometry.
	 * \param sHeader BSP Header.
	 */
	void LoadTris(const BSPHeader &sHeader);

	
	VideoSystem*                                     m_VideoSystem;       /** Pointer to the video system. */
	std::ifstream                                    m_sBSPFile;          /** File stream. */
	std::string                                      m_szMapID;           /** Map name. */
	unsigned char*                                   lmapAtlas;           /** Lightmap atlas. */
	unsigned int                                     lmapTexId;           /** Lightmap texture ID. */
	std::vector<Lightmap>                            lmaps;               /** Loaded lightmaps. */
	std::map<std::string, TrianglePointMap>          texturedTris;        /** Loaded map geometry. */
	unsigned int*                                    bufObjects;          /** Geometry to be pushed to the vertex buffer. */
	BSPTextureInfo*                                  m_btfs;              /** Texture info header. */
	float*                                           minUV;               /** UV calculation. */
	float*                                           maxUV;               /** UV calculation. */
	uint8_t*                                         lmap;                /** Calculated lightmap for this map. */
	std::vector<std::string>                         m_vTexNames;         /** Vector of loaded texture names. */
	std::map<std::string, std::vector<std::string> > m_vDontRenderModel;  /** Vector of models to ignore rendering. */
	std::map<int, bool>                              m_vDontRenderFace;   /** Vector of faces to ignore rendering. */
	std::vector<Vertex3f>                            m_vVerticesPrime;    /** Loaded surfedges. */
	Vertex3f                                         ConfigOffsetChapter; /** Offset for all maps in the same chapter. */

};//end BSP

#endif //BSP_H
