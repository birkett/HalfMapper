/*
 * halfmapper, a renderer for GoldSrc maps and chapters.
 *
 * Copyright(C) 2014  Gonzalo Åvila "gzalo" Alterach
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
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "bsp.h"
#include "ConfigXML.h"
#include "TextureLoader.h"
#include "VideoSystem.h"


// These are global for now - needed so maps can know eachothers position.
std::map<std::string, Vertex3f> offsets;
std::map<std::string, std::vector<std::pair<Vertex3f, std::string>>> vLandmarks;


/**
 * Contructor.
 * \param szGamePaths List of game paths to attempt loading from.
 * \param sMapEntry   MapEntry structure, containing config data for the current map.
 * \param videosystem Pointer to the video system.
 */
BSP::BSP(const std::vector<std::string> &szGamePaths, const MapEntry &sMapEntry, VideoSystem* &videosystem)
{
	this->m_szMapID     = sMapEntry.m_szName;
	this->m_VideoSystem = videosystem;

	// Try to open the file from all known gamepaths.
	for (size_t i = 0; i < szGamePaths.size(); i++) {
		if (!this->m_sBSPFile.is_open()) {
			this->m_sBSPFile.open(szGamePaths[i] + "maps/" + this->m_szMapID + ".bsp", std::ios::binary);
		}
	}

	// If the BSP wasn't found in any of the gamepaths...
	if (!this->m_sBSPFile.is_open()) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't open BSP", this->m_szMapID.c_str());
		return;
	}

	// Check BSP version.
	this->m_sBSPFile.read((char*)&this->m_sBSPHeader, sizeof(this->m_sBSPHeader));

	if (!this->IsValidBSPHeader()) {
		return;
	}

	// Read Entities.
	this->ParseEntities(sMapEntry);

	// Read Models and hide some faces.
	this->LoadModels();

	// Load surfaces, edges and surfedges.
	this->LoadSurfEdges();

	// Read Textures.
	this->LoadTextures();

	// Read Texture information.
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_TEXINFO].iOffset, std::ios::beg);
	m_btfs = new BSPTextureInfo[this->m_sBSPHeader.lump[LUMP_TEXINFO].iLength / (int)sizeof(BSPTextureInfo)];
	this->m_sBSPFile.read((char*)m_btfs, this->m_sBSPHeader.lump[LUMP_TEXINFO].iLength);

	minUV = new float[this->m_sBSPHeader.lump[LUMP_FACES].iLength/(int)sizeof(BSPFace)*2];
	maxUV = new float[this->m_sBSPHeader.lump[LUMP_FACES].iLength/(int)sizeof(BSPFace)*2];

	// Read Faces and lightmaps.
	this->LoadFacesAndLightMaps();

	this->GenerateLightMaps();

	// Load the actual triangles.
	this->LoadTris();

	this->m_sBSPFile.close();

	// Calculate map offset based on landmarks.
	this->CalculateOffset();

	bufObjects = new unsigned int[texturedTris.size()];

	this->m_VideoSystem->CreateBuffer(texturedTris.size(), bufObjects);

	int i     = 0;
	totalTris = 0;
	for(std::map<std::string, TrianglePointMap>::iterator it = texturedTris.begin(); it != texturedTris.end(); ++it, i++) {
		this->m_VideoSystem->AddDataToBuffer(bufObjects[i], (*it).second.triangles.size() * sizeof(VectorFinal), (void*)&(*it).second.triangles[0]);
		totalTris += (*it).second.triangles.size();
	}

}//end BSP::BSP()


/**
 * Destructor.
 */
BSP::~BSP()
{
	delete[] bufObjects;
	delete[] minUV;
	delete[] maxUV;
	delete[] lmap;

}//end BSP::~BSP()


/**
 * Render this map.
 */
void BSP::Render()
{
	this->m_VideoSystem->BeginFrame(offsets[this->m_szMapID].m_fX + ConfigOffsetChapter.m_fX, offsets[this->m_szMapID].m_fY + ConfigOffsetChapter.m_fY, offsets[this->m_szMapID].m_fZ + ConfigOffsetChapter.m_fZ, lmapTexId);

	int i = 0;
	for (std::map<std::string, TrianglePointMap>::iterator it = texturedTris.begin(); it != texturedTris.end(); ++it, i++) {
		// Don't render some dummy triangles (triggers and such).
		if ((*it).first != "aaatrigger"
			&& (*it).first != "origin"
			&& (*it).first != "clip"
			&& (*it).first != "sky"
			&& (*it).first[0] != '{'
			&& (*it).second.triangles.size() != 0
			) {
			this->m_VideoSystem->PushData(bufObjects[i], (*it).second.texId, sizeof(VectorFinal), (*it).second.triangles.size());
		}
	}

	this->m_VideoSystem->EndFrame();

}//end BSP::Render()


/**
 * Set the offset, common to all maps in the same chapter.
 * \param x X offset.
 * \param y Y offset.
 * \param z Z offset.
 */
void BSP::SetChapterOffset(const float x, const float y, const float z)
{
	ConfigOffsetChapter.m_fX = x;
	ConfigOffsetChapter.m_fY = y;
	ConfigOffsetChapter.m_fZ = z;

}//end BSP::SetChapterOffset()


/**
 * Calculate correct coordinates.
 * \param v      Base position vector.
 * \param vs     S position vector.
 * \param vt     T position vector.
 * \param sShift Offset in the S direction.
 * \param tShift Offset in the T direction.
 */
inline UVCoordinates BSP::CalculateCoordinates(Vertex3f v, Vertex3f vs, Vertex3f vt, float sShift, float tShift)
{
	UVCoordinates ret;
	ret.u = sShift + (vs.m_fX * v.m_fX) + (vs.m_fY * v.m_fY) + (vs.m_fZ * v.m_fZ);
	ret.v = tShift + (vt.m_fX * v.m_fX) + (vt.m_fY * v.m_fY) + (vt.m_fZ * v.m_fZ);

	return ret;

}//end CalculateCoordinates()


/**
 * Calculate the map offset, using landmarks + chater and single map offsets.
 */
void BSP::CalculateOffset()
{
	float ox = 0.0f;
	float oy = 0.0f;
	float oz = 0.0f;

	bool found = false;

	for (std::map<std::string, std::vector<std::pair<Vertex3f, std::string>>>::iterator it = vLandmarks.begin(); it != vLandmarks.end(); ++it) {
		if ((*it).second.size() > 1) {
			for (size_t i = 0; i < (*it).second.size(); i++) {
				if( (*it).second[i].second == this->m_szMapID) {
					if (i == 0) {
						if (offsets.count((*it).second[i+1].second) != 0) {
							Vertex3f c1 = (*it).second[i].first;
							Vertex3f c2 = (*it).second[i+1].first;
							Vertex3f c3 = offsets[(*it).second[i+1].second];
							ox = + c2.m_fX + c3.m_fX - c1.m_fX;
							oy = + c2.m_fY + c3.m_fY - c1.m_fY;
							oz = + c2.m_fZ + c3.m_fZ - c1.m_fZ;

							found = true;
							Logger::GetInstance()->AddMessage(E_INFO, "Matched", (*it).second[i].second.c_str(), (*it).second[i + 1].second.c_str());
							break;
						}
					}
					else {
						if (offsets.count((*it).second[i-1].second) != 0) {
							Vertex3f c1 = (*it).second[i].first;
							Vertex3f c2 = (*it).second[i-1].first;
							Vertex3f c3 = offsets[(*it).second[i-1].second];
							ox = + c2.m_fX + c3.m_fX - c1.m_fX;
							oy = + c2.m_fY + c3.m_fY - c1.m_fY;
							oz = + c2.m_fZ + c3.m_fZ - c1.m_fZ;

							found = true;
							Logger::GetInstance()->AddMessage(E_INFO, "Matched", (*it).second[i].second, (*it).second[i - 1].second);
							break;
						}
					}
				}
			}
		}
	}
	if (!found) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Can't find landmarks for", this->m_szMapID.c_str());
	}
	offsets[this->m_szMapID] = Vertex3f(ox, oy, oz);

}//end BSP::CalculateOffset()


/**
 * Check if this is a valid v30 BSP.
 */
bool BSP::IsValidBSPHeader()
{
	if (this->m_sBSPHeader.iVersion != 30) {
		Logger::GetInstance()->AddMessage(E_ERROR, "BSP version is not 30:", this->m_szMapID.c_str());
		return false;
	}

	return true;

}//end BSP::IsValidBSPHeader()


/**
 * Parse the entitities in the map.
 * \param sMapEntry MapEntry structure.
 */
void BSP::ParseEntities(const MapEntry &sMapEntry)
{
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_ENTITIES].iOffset, std::ios::beg);
	char *szEntitiesLump = new char[this->m_sBSPHeader.lump[LUMP_ENTITIES].iLength];
	this->m_sBSPFile.read(szEntitiesLump, this->m_sBSPHeader.lump[LUMP_ENTITIES].iLength);

	std::stringstream ss(szEntitiesLump);

	int status = 0;

	std::string origin, targetname, landmark, modelname;
	bool isLandMark = false, isChangeLevel = false, isTeleport = false;

	std::map<std::string, int> changelevels;
	std::map<std::string, Vertex3f> ret;

	while (ss.good()) {
		std::string str;
		getline(ss, str);
		if (status == 0) {
			if (str == "{") {
				status = 1, isLandMark = false, isChangeLevel = false, isTeleport = false;
			}
			else {
				if (ss.good())
					Logger::GetInstance()->AddMessage(E_ERROR, "Missing stuff in entity:", str.c_str());
			}
		}
		else if (status == 1) {
			if (str == "}") {
				status = 0;
				if (isLandMark) {
					float x, y, z;
					sscanf(origin.c_str(), "%f %f %f", &x, &y, &z);
					Vertex3f v(x, y, z);
					v.FixHand();

					if (sMapEntry.m_szOffsetTargetName == targetname) {
						// Apply map offsets from the config, to fix landmark positions.
						v.m_fX += sMapEntry.m_fOffsetX;
						v.m_fY += sMapEntry.m_fOffsetY;
						v.m_fZ += sMapEntry.m_fOffsetZ;
					}

					ret[targetname] = v;
				}
				else if (isChangeLevel && (landmark.size() > 0)) {
					changelevels[landmark] = 1;
				}
				if (isTeleport || isChangeLevel) {
					this->m_vDontRenderModel[this->m_szMapID].push_back(modelname);
				}
			}
			else {
				if (str == "\"classname\" \"info_landmark\"") {
					isLandMark = true;
				}
				if (str == "\"classname\" \"trigger_changelevel\"") {
					isChangeLevel = true;
				}
				if (str == "\"classname\" \"trigger_teleport\""
					|| str == "\"classname\" \"func_pendulum\""
					|| str == "\"classname\" \"trigger_transition\""
					|| str == "\"classname\" \"trigger_hurt\""
					|| str == "\"classname\" \"func_train\""
					|| str == "\"classname\" \"func_door_rotating\""
				) {
					isTeleport = true;
				}
				if (str.substr(0, 8) == "\"origin\"") {
					origin = str.substr(10);
					origin.erase(origin.size() - 1);
				}
				if (str.substr(0, 7) == "\"model\"") {
					modelname = str.substr(9);
					modelname.erase(modelname.size() - 1);
				}
				if (str.substr(0, 12) == "\"targetname\"") {
					targetname = str.substr(14);
					targetname.erase(targetname.size() - 1);
				}
				if (str.substr(0, 10) == "\"landmark\"") {
					landmark = str.substr(12);
					landmark.erase(landmark.size() - 1);
				}
			}
		}
	}

	for (std::map<std::string, Vertex3f>::iterator it = ret.begin(); it != ret.end(); ++it) {
		if (changelevels.count((*it).first) != 0) {
			vLandmarks[(*it).first].push_back(make_pair((*it).second, this->m_szMapID));
		}
	}

	delete[] szEntitiesLump;

}//end BSP::ParseEntities()


/**
 * Load the faces and lightmaps.
 */
void BSP::LoadFacesAndLightMaps()
{
	// Read Lightmaps.
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_LIGHTING].iOffset, std::ios::beg);
	int size = this->m_sBSPHeader.lump[LUMP_LIGHTING].iLength;
	lmap = new uint8_t[size];
	this->m_sBSPFile.read((char*)lmap, size);

	// Read faces.
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_FACES].iOffset, std::ios::beg);

	for (int i = 0; i < this->m_sBSPHeader.lump[LUMP_FACES].iLength / (int)sizeof(BSPFace); i++) {
		BSPFace f;
		this->m_sBSPFile.read((char*)&f, sizeof(f));

		if (f.iLightmapOffset > size) {
			Logger::GetInstance()->AddMessage(E_ERROR, "Lightmap offset too large:", "Map,", this->m_szMapID, "Lightmap size,", size, "Requested offset,", f.iLightmapOffset);
			f.iLightmapOffset = 0;
		}

		BSPTextureInfo b = m_btfs[f.iTextureInfo];

		minUV[i * 2] = minUV[i * 2 + 1] = 99999;
		maxUV[i * 2] = maxUV[i * 2 + 1] = -99999;

		for (int j = 2, k = 1; j < f.iEdges; j++, k++) {
			Vertex3f v1 = this->m_vVerticesPrime[f.iFirstEdge];
			Vertex3f v2 = this->m_vVerticesPrime[f.iFirstEdge + k];
			Vertex3f v3 = this->m_vVerticesPrime[f.iFirstEdge + j];

			UVCoordinates c1 = this->CalculateCoordinates(v1, b.vS, b.vT, b.fSShift, b.fTShift);
			UVCoordinates c2 = this->CalculateCoordinates(v2, b.vS, b.vT, b.fSShift, b.fTShift);
			UVCoordinates c3 = this->CalculateCoordinates(v3, b.vS, b.vT, b.fSShift, b.fTShift);

			minUV[i * 2] = std::min(minUV[i * 2], c1.u); minUV[i * 2 + 1] = std::min(minUV[i * 2 + 1], c1.v);
			minUV[i * 2] = std::min(minUV[i * 2], c2.u); minUV[i * 2 + 1] = std::min(minUV[i * 2 + 1], c2.v);
			minUV[i * 2] = std::min(minUV[i * 2], c3.u); minUV[i * 2 + 1] = std::min(minUV[i * 2 + 1], c3.v);

			maxUV[i * 2] = std::max(maxUV[i * 2], c1.u); maxUV[i * 2 + 1] = std::max(maxUV[i * 2 + 1], c1.v);
			maxUV[i * 2] = std::max(maxUV[i * 2], c2.u); maxUV[i * 2 + 1] = std::max(maxUV[i * 2 + 1], c2.v);
			maxUV[i * 2] = std::max(maxUV[i * 2], c3.u); maxUV[i * 2 + 1] = std::max(maxUV[i * 2 + 1], c3.v);
		}

		int lmw = (int)(ceil(maxUV[i * 2]     / 16) - floor(minUV[i * 2]     / 16) + 1);
		int lmh = (int)(ceil(maxUV[i * 2 + 1] / 16) - floor(minUV[i * 2 + 1] / 16) + 1);

		if (lmw > 17 || lmh > 17) {
			lmw = lmh = 1;
		}

		Lightmap l;
		l.m_iWidth = lmw;
		l.m_iHeight = lmh;
		l.offset = lmap + f.iLightmapOffset;

		lmaps.push_back(l);
	}

}//end BSP::LoadFacesAndLightMaps()


/**
 * Generate the lightmap.
 */
void BSP::GenerateLightMaps()
{
	//Light map atlas
	lmapAtlas = new uint8_t[1024 * 1024 * 3];

	uint8_t gammaTable[256];
	for (int i = 0; i < 256; i++) {
		gammaTable[i] = pow(i / 255.0, 1.0 / 3.0) * 255;
	}

	int lmapRover[1024];
	memset(lmapRover, 0, 1024 * sizeof(int));

	//Light map "rover" algorithm from Quake 2 (http://fabiensanglard.net/quake2/quake2_opengl_renderer.php)
	for (unsigned int i = 0; i < lmaps.size(); i++) {
		int best  = 1024;
		int best2 = 0;

		for (int a = 0; a < 1024 - lmaps[i].m_iWidth; a++) {
			int j = 0;

			for (j = 0; j < lmaps[i].m_iWidth; j++) {
				if (lmapRover[a + j] >= best) {
					break;
				}
				if (lmapRover[a + j] > best2) {
					best2 = lmapRover[a + j];
				}
			}
			if (j == lmaps[i].m_iWidth) {
				lmaps[i].m_iFinalX = a;
				lmaps[i].m_iFinalY = best = best2;
			}
		}

		if (best + lmaps[i].m_iHeight > 1024) {
			Logger::GetInstance()->AddMessage(E_ERROR, "Lightmap atlas is too small:", this->m_szMapID.c_str());
			break;
		}

		for (int a = 0; a < lmaps[i].m_iWidth; a++) {
			lmapRover[lmaps[i].m_iFinalX + a] = best + lmaps[i].m_iHeight;
		}

		int finalX = lmaps[i].m_iFinalX;
		int finalY = lmaps[i].m_iFinalY;

		#define ATXY(_x,_y) ((_x) + ((_y) * 1024))       *3
		#define LMXY(_x,_y) ((_x) + ((_y) * lmaps[i].m_iWidth)) *3

		for (int y = 0; y < lmaps[i].m_iHeight; y++) {
			for (int x = 0; x < lmaps[i].m_iWidth; x++) {
				lmapAtlas[ATXY(finalX + x, finalY + y) + 0] = gammaTable[lmaps[i].offset[LMXY(x, y) + 0]];
				lmapAtlas[ATXY(finalX + x, finalY + y) + 1] = gammaTable[lmaps[i].offset[LMXY(x, y) + 1]];
				lmapAtlas[ATXY(finalX + x, finalY + y) + 2] = gammaTable[lmaps[i].offset[LMXY(x, y) + 2]];
			}
		}
	}

	lmapTexId = this->m_VideoSystem->CreateTexture(true);
	this->m_VideoSystem->AddTexture(0, 1024, 1024, lmapAtlas, true);

	delete[] lmapAtlas;

}//end BSP::GenerateLightmap()


/**
 * Load models from the map.
 */
void BSP::LoadModels()
{
	BSPModel *models = new BSPModel[this->m_sBSPHeader.lump[LUMP_MODELS].iLength / (int)sizeof(BSPModel)];
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_MODELS].iOffset, std::ios::beg);
	this->m_sBSPFile.read((char*)models, this->m_sBSPHeader.lump[LUMP_MODELS].iLength);

	for (size_t i = 0; i < this->m_vDontRenderModel[this->m_szMapID].size(); i++) {
		int modelId = atoi(this->m_vDontRenderModel[this->m_szMapID][i].substr(1).c_str());
		int startingFace = models[modelId].iFirstFace;
		for (int j = 0; j < models[modelId].nFaces; j++) {
			this->m_vDontRenderFace[j + startingFace] = true;
		}
	}

	delete[] models;

}//end BSP::LoadModels()


/**
 * Load surfaces and edges.
 */
void BSP::LoadSurfEdges()
{
	// Read Vertices.
	std::vector<Vertex3f> vertices;
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_VERTICES].iOffset, std::ios::beg);
	for (int i = 0; i < this->m_sBSPHeader.lump[LUMP_VERTICES].iLength / 12; i++) {
		Vertex3f v;
		this->m_sBSPFile.read((char*)&v, sizeof(v));
		vertices.push_back(v);
	}

	// Read Edges.
	BSPEdge *edges = new BSPEdge[this->m_sBSPHeader.lump[LUMP_EDGES].iLength / (int)sizeof(BSPEdge)];
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_EDGES].iOffset, std::ios::beg);
	this->m_sBSPFile.read((char*)edges, this->m_sBSPHeader.lump[LUMP_EDGES].iLength);

	// Read Surfedges.
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_SURFEDGES].iOffset, std::ios::beg);
	for (int i = 0; i < this->m_sBSPHeader.lump[LUMP_SURFEDGES].iLength / (int)sizeof(int); i++) {
		int e;
		this->m_sBSPFile.read((char*)&e, sizeof(e));
		this->m_vVerticesPrime.push_back(vertices[edges[e>0 ? e : -e].iVertex3f[e>0 ? 0 : 1]]);
	}

	delete[] edges;

}//end BSP::LoadSurfEdges()


/**
 * Load textures stored in the map.
 */
void BSP::LoadTextures()
{
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_TEXTURES].iOffset, std::ios::beg);
	BSPTextureHeader theader;
	this->m_sBSPFile.read((char*)&theader, sizeof(theader));
	int *texOffSets = new int[theader.nMipTextures];
	this->m_sBSPFile.read((char*)texOffSets, theader.nMipTextures * sizeof(int));

	uint32_t* iOffsets = new uint32_t[theader.nMipTextures];

	for (uint32_t i = 0; i < theader.nMipTextures; i++) {
		iOffsets[i] = this->m_sBSPHeader.lump[LUMP_TEXTURES].iOffset + texOffSets[i];
	}

	// Save the texture names in order that they will be loaded. Need them later for binding.
	for (unsigned int i = 0; i < theader.nMipTextures; i++) {
		this->m_sBSPFile.seekg(iOffsets[i], std::ios::beg);
		TextureInfo sTextureInfo;
		this->m_sBSPFile.read((char*)&sTextureInfo, sizeof(TextureInfo));
		this->m_vTexNames.push_back(sTextureInfo.szName);
	}

	TextureLoader::GetInstance()->LoadTextures(theader.nMipTextures, iOffsets, this->m_sBSPFile, this->m_VideoSystem);

	delete[] iOffsets;
	delete[] texOffSets;

}//end BSP::LoadTextures()


/**
 * Load the actual map geometry.
 */
void BSP::LoadTris()
{
	this->m_sBSPFile.seekg(this->m_sBSPHeader.lump[LUMP_FACES].iOffset, std::ios::beg);
	for (int i = 0; i < this->m_sBSPHeader.lump[LUMP_FACES].iLength / (int)sizeof(BSPFace); i++) {
		BSPFace f;
		this->m_sBSPFile.read((char*)&f, sizeof(f));

		if (this->m_vDontRenderFace[i]) {
			continue;
		}

		BSPTextureInfo b = m_btfs[f.iTextureInfo];

		std::string faceTexName = this->m_vTexNames[b.iMiptex];

		//Calculate light map uvs
		int lmw = (int)(ceil(maxUV[i * 2]     / 16) - floor(minUV[i * 2]     / 16) + 1);
		int lmh = (int)(ceil(maxUV[i * 2 + 1] / 16) - floor(minUV[i * 2 + 1] / 16) + 1);

		if (lmw > 17 || lmh > 17) {
			continue;
		}

		float mid_poly_s = (minUV[i * 2] + maxUV[i * 2]) / 2.0f;
		float mid_poly_t = (minUV[i * 2 + 1] + maxUV[i * 2 + 1]) / 2.0f;

		float mid_tex_s = (float)lmw / 2.0f;
		float mid_tex_t = (float)lmh / 2.0f;

		float fX = (float)lmaps[i].m_iFinalX;
		float fY = (float)lmaps[i].m_iFinalY;

		Texture sTex = TextureLoader::GetInstance()->m_vLoadedTextures[faceTexName];

		std::vector<VectorFinal>*vt = &texturedTris[faceTexName].triangles;

		for (int j = 2, k = 1; j < f.iEdges; j++, k++) {
			Vertex3f v1 = this->m_vVerticesPrime[f.iFirstEdge];
			Vertex3f v2 = this->m_vVerticesPrime[f.iFirstEdge + k];
			Vertex3f v3 = this->m_vVerticesPrime[f.iFirstEdge + j];

			UVCoordinates c1 = this->CalculateCoordinates(v1, b.vS, b.vT, b.fSShift, b.fTShift);
			UVCoordinates c2 = this->CalculateCoordinates(v2, b.vS, b.vT, b.fSShift, b.fTShift);
			UVCoordinates c3 = this->CalculateCoordinates(v3, b.vS, b.vT, b.fSShift, b.fTShift);

			UVCoordinates c1l, c2l, c3l;

			c1l.u = mid_tex_s + (c1.u - mid_poly_s) / 16.0f;
			c2l.u = mid_tex_s + (c2.u - mid_poly_s) / 16.0f;
			c3l.u = mid_tex_s + (c3.u - mid_poly_s) / 16.0f;
			c1l.v = mid_tex_t + (c1.v - mid_poly_t) / 16.0f;
			c2l.v = mid_tex_t + (c2.v - mid_poly_t) / 16.0f;
			c3l.v = mid_tex_t + (c3.v - mid_poly_t) / 16.0f;

			c1l.u += fX; c2l.u += fX; c3l.u += fX;
			c1l.v += fY; c2l.v += fY; c3l.v += fY;

			c1l.u /= 1024.0; c2l.u /= 1024.0; c3l.u /= 1024.0;
			c1l.v /= 1024.0; c2l.v /= 1024.0; c3l.v /= 1024.0;

			c1.u /= sTex.iWidth; c2.u /= sTex.iWidth; c3.u /= sTex.iWidth;
			c1.v /= sTex.iHeight; c2.v /= sTex.iHeight; c3.v /= sTex.iHeight;

			v1.FixHand();
			v2.FixHand();
			v3.FixHand();

			vt->push_back(VectorFinal(v1, c1, c1l));
			vt->push_back(VectorFinal(v2, c2, c2l));
			vt->push_back(VectorFinal(v3, c3, c3l));
		}
		texturedTris[faceTexName].texId = TextureLoader::GetInstance()->m_vLoadedTextures[faceTexName].iTextureId;
	}

	delete[] m_btfs;

}//end BSP::LoadTris()
