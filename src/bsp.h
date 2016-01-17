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

#define LUMP_ENTITIES      0
#define LUMP_PLANES        1
#define LUMP_TEXTURES      2
#define LUMP_VERTICES      3
#define LUMP_VISIBILITY    4
#define LUMP_NODES         5
#define LUMP_TEXINFO       6
#define LUMP_FACES         7
#define LUMP_LIGHTING      8
#define LUMP_CLIPNODES     9
#define LUMP_LEAVES       10
#define LUMP_MARKSURFACES 11
#define LUMP_EDGES        12
#define LUMP_SURFEDGES    13
#define LUMP_MODELS       14
#define HEADER_LUMPS      15


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
	int32_t iVersion;           /** BSP version. 30 for Half-Life. */
	BSPLump lump[HEADER_LUMPS]; /** Lump entries. */
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
	uint32_t iLightmapOffset; /** Offsets into the raw lightmap data. */
};//end BSPFace


struct BSPEDGE
{
	uint16_t iVertex3f[2]; // Indices into Vertex3f array
};

struct BSPTEXTUREINFO
{
	Vertex3f vS;
	float fSShift;    // Texture shift in s direction
	Vertex3f vT;
	float fTShift;    // Texture shift in t direction
	uint32_t iMiptex; // Index into textures array
	uint32_t nFlags;  // Texture flags, seem to always be 0
};

struct BSPTEXTUREHEADER
{
	uint32_t nMipTextures; // Number of BSPMIPTEX structures
};

#define MAX_MAP_HULLS 4
struct BSPMODEL
{
    float nMins[3], nMaxs[3];          // Defines bounding box
    Vertex3f vOrigin;                  // Coordinates to move the // coordinate system
    int32_t iHeadnodes[MAX_MAP_HULLS]; // Index into nodes array
    int32_t nVisLeafs;                 // ???
    int32_t iFirstFace, nFaces;        // Index and count into faces
};

struct COORDS
{
	float u, v;
};

struct VECFINAL
{
	float x,y,z,u,v,ul,vl;
	VECFINAL(float _x, float _y, float _z, float _u, float _v){
		x = _x;
		y = _y;
		z = _z;
		u = _u;
		v = _v;
	}
	VECFINAL(Vertex3f vt, COORDS c, COORDS c2){
		x = vt.x, y = vt.y, z = vt.z;
		u = c.u, v = c.v;
		ul = c2.u;
		vl = c2.v;
	}
};

struct LMAP
{
	unsigned char *offset; int w,h;
	int finalX, finalY;
};

struct TEXSTUFF
{
	std::vector<VECFINAL> triangles;
	int texId;
};

class BSP
{
public:
	BSP(const std::vector<std::string> &szGamePaths, const MapEntry &sMapEntry, VideoSystem* &videosystem);
	~BSP();
	void Render();
	void SetChapterOffset(const float x, const float y, const float z);
	size_t totalTris;

private:
	void CalculateOffset();
	void ParseEntities(const BSPHeader &sHeader, const MapEntry &sMapEntry);
	bool IsValidBSPHeader(const BSPHeader &sHeader);
	void LoadFacesAndLightMaps(const BSPHeader &sHeader);
	void GenerateLightMaps();
	void LoadModels(const BSPHeader &sHeader);
	void LoadSurfEdges(const BSPHeader &sHeader);
	void LoadTextures(const BSPHeader &sHeader);
	void LoadTris(const BSPHeader &sHeader);

	VideoSystem* m_VideoSystem;

	std::ifstream m_sBSPFile;

	std::string m_szMapID;

	unsigned char *lmapAtlas;
	unsigned int lmapTexId;

	std::vector<LMAP> lmaps;
	std::map<std::string, TEXSTUFF> texturedTris;
	unsigned int *bufObjects;
	BSPTEXTUREINFO *m_btfs;
	float *minUV;
	float *maxUV;
	uint8_t *lmap;

	std::vector<std::string>  m_vTexNames; /** Vector of loaded texture names. */
	std::map<std::string, std::vector<std::string> > m_vDontRenderModel;
	std::map<int, bool> m_vDontRenderFace;
	std::vector<Vertex3f> m_vVerticesPrime;

	Vertex3f ConfigOffsetChapter;

};//end BSP

#endif //BSP_H
