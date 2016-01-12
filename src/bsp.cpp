#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <GL/glew.h>
#include "bsp.h"
#include "entities.h"
#include "ConfigXML.h"
#include "TextureLoader.h"


map <string, Texture> textures;
map <string, vector<pair<Vertex3f,string> > > landmarks;
map <string, vector<string> > dontRenderModel;
map <string, Vertex3f> offsets;

//Correct UV coordinates
static inline COORDS calcCoords(Vertex3f v, Vertex3f vs, Vertex3f vt, float sShift, float tShift){
	COORDS ret;
	ret.u = sShift + vs.x*v.x + vs.y*v.y + vs.z*v.z; 
	ret.v = tShift + vt.x*v.x + vt.y*v.y + vt.z*v.z;
	return ret;
}

BSP::BSP(const std::vector<std::string> &szGamePaths, const string &filename, const MapEntry &sMapEntry){
	string id = sMapEntry.m_szName;

	uint8_t gammaTable[256];
	for(int i=0;i<256;i++)
		gammaTable[i] = pow(i / 255.0, 1.0/3.0) * 255;

	//Light map atlas
	lmapAtlas = new uint8_t[1024*1024*3];

	ifstream inBSP;

	// Try to open the file from all known gamepaths.
	for (size_t i = 0; i < szGamePaths.size(); i++) {
		if (!inBSP.is_open()) {
			inBSP.open(szGamePaths[i] + filename.c_str(), ios::binary);
		}
	}

	// If the BSP wasn't found in any of the gamepaths...
	if(!inBSP.is_open()){ cerr << "Can't open BSP " << filename << "." << endl; return;}
	
	//Check BSP version
	BSPHEADER bHeader;
	inBSP.read((char*)&bHeader, sizeof(bHeader));
	if(bHeader.nVersion != 30){ cerr << "BSP version is not 30 (" << filename << ")." << endl; return;}
	
	//Read Entities
	inBSP.seekg(bHeader.lump[LUMP_ENTITIES].nOffset, ios::beg);
	char *bff = new char[bHeader.lump[LUMP_ENTITIES].nLength];
	inBSP.read(bff, bHeader.lump[LUMP_ENTITIES].nLength);
	parseEntities(bff,id,sMapEntry);
	delete []bff;
	
	//Read Models and hide some faces
	BSPMODEL *models = new BSPMODEL[bHeader.lump[LUMP_MODELS].nLength/(int)sizeof(BSPMODEL)];
	inBSP.seekg(bHeader.lump[LUMP_MODELS].nOffset, ios::beg);	
	inBSP.read((char*)models, bHeader.lump[LUMP_MODELS].nLength);
	
	map <int, bool> dontRenderFace;
	for(unsigned int i=0;i<dontRenderModel[id].size();i++){
		int modelId = atoi(dontRenderModel[id][i].substr(1).c_str());
		int startingFace = models[modelId].iFirstFace;
		for(int j=0;j<models[modelId].nFaces;j++){
			//if(modelId == 57) cout << j+startingFace << endl;
			dontRenderFace[j+startingFace] = true;
		}
	}	
	
	
	//Read Vertices
	vector <Vertex3f> vertices;
	inBSP.seekg(bHeader.lump[LUMP_VERTICES].nOffset, ios::beg);		
	for(int i=0;i<bHeader.lump[LUMP_VERTICES].nLength/12;i++){
		Vertex3f v;
		inBSP.read((char*)&v, sizeof(v));
		vertices.push_back(v);
	}
	
	//Read Edges	
	BSPEDGE *edges = new BSPEDGE[bHeader.lump[LUMP_EDGES].nLength/(int)sizeof(BSPEDGE)];
	inBSP.seekg(bHeader.lump[LUMP_EDGES].nOffset, ios::beg);	
	inBSP.read((char*)edges, bHeader.lump[LUMP_EDGES].nLength);
	
	//Read Surfedges
	vector <Vertex3f> verticesPrime;
	inBSP.seekg(bHeader.lump[LUMP_SURFEDGES].nOffset, ios::beg);
	for(int i=0;i<bHeader.lump[LUMP_SURFEDGES].nLength/(int)sizeof(int);i++){
		int e;
		inBSP.read((char*)&e, sizeof(e));
		verticesPrime.push_back(vertices[edges[e>0?e:-e].iVertex3f[e>0?0:1]]);
	}
	
	//Read Lightmaps
	inBSP.seekg(bHeader.lump[LUMP_LIGHTING].nOffset, ios::beg);
	int size = bHeader.lump[LUMP_LIGHTING].nLength;
	uint8_t *lmap = new uint8_t[size];
	inBSP.read((char*)lmap, size);
	vector <LMAP> lmaps;
	
	//Read Textures
	inBSP.seekg(bHeader.lump[LUMP_TEXTURES].nOffset, ios::beg);
	BSPTEXTUREHEADER theader;
	inBSP.read((char*)&theader, sizeof(theader));
	int *texOffSets = new int[theader.nMipTextures];
	inBSP.read((char*)texOffSets, theader.nMipTextures*sizeof(int));
	
	vector <string> texNames;
	
	uint8_t *dataDr  = new uint8_t[512 * 512];     // Raw texture data.
	uint8_t *dataUp  = new uint8_t[512 * 512 * 4]; // 32 bit texture.
	uint8_t *dataPal = new uint8_t[256 * 3];       // 256 color pallete.

	for(unsigned int i = 0; i < theader.nMipTextures; i++) {
		inBSP.seekg(bHeader.lump[LUMP_TEXTURES].nOffset+texOffSets[i], ios::beg);
		
		TextureInfo sTextureInfo;
		inBSP.read((char*)&sTextureInfo, sizeof(TextureInfo));

		// First appearance of the texture.
		if (textures.count(sTextureInfo.szName) == 0) {
			Texture sTexture;
			sTexture.iWidth = sTextureInfo.iWidth;
			sTexture.iHeight = sTextureInfo.iHeight;

			glGenTextures(1, &sTexture.iTextureId);
			glBindTexture(GL_TEXTURE_2D, sTexture.iTextureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);

			// Sizes of each mipmap.
			const int dimensionsSquared[4] = { 1,4,16,64 };
			const int dimensions[4] = { 1,2,4,8 };

			// Read each mipmap.
			for (int mip = 3; mip >= 0; mip--) {
				if (sTextureInfo.iOffsets[0] == 0 || sTextureInfo.iOffsets[1] == 0 || sTextureInfo.iOffsets[2] == 0 || sTextureInfo.iOffsets[3] == 0) {
					std::cout << "Texture found, but no mipmaps. " << sTextureInfo.szName << std::endl;
					break;
				}

				inBSP.seekg(bHeader.lump[LUMP_TEXTURES].nOffset + texOffSets[i] + sTextureInfo.iOffsets[mip], ios::beg);
				inBSP.read((char*)dataDr, sTextureInfo.iWidth * sTextureInfo.iHeight / dimensionsSquared[mip]);

				if (mip == 3) {
					// Read the pallete (comes after last mipmap).
					uint16_t dummy;
					inBSP.read((char*)&dummy, 2);
					inBSP.read((char*)dataPal, 256 * 3);
				}

				for (uint32_t y = 0; y < sTextureInfo.iHeight / dimensions[mip]; y++) {
					for (uint32_t x = 0; x < sTextureInfo.iWidth / dimensions[mip]; x++) {
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4]     = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3];
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 1] = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3 + 1];
						dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 2] = dataPal[dataDr[y * sTextureInfo.iWidth / dimensions[mip] + x] * 3 + 2];

						// Do full transparency on blue pixels.
						if (dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4] == 0
							&& dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 1] == 0
							&& dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 2] == 255
							) {
							dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 3] = 0;
						}
						else {
							dataUp[(x + y * sTextureInfo.iWidth / dimensions[mip]) * 4 + 3] = 255;
						}
					}
				}

				glTexImage2D(GL_TEXTURE_2D, mip, GL_RGBA, sTextureInfo.iWidth / dimensions[mip], sTextureInfo.iHeight / dimensions[mip], 0, GL_RGBA, GL_UNSIGNED_BYTE, dataUp);
			}

			textures[sTextureInfo.szName]=sTexture;
			
			/* If mipmap offsets = 0
			Create new texture in its place with dummy values.
				Texture n;
				n.iTextureId = 0;
				n.iWidth = 1;
				n.iHeight = 1;
				textures[sTextureInfo.szName]=n;
			*/
		}
		texNames.push_back(sTextureInfo.szName);
	}
	
	//Read Texture information
	inBSP.seekg(bHeader.lump[LUMP_TEXINFO].nOffset, ios::beg);
	BSPTEXTUREINFO *btfs = new BSPTEXTUREINFO[bHeader.lump[LUMP_TEXINFO].nLength/(int)sizeof(BSPTEXTUREINFO)];
	inBSP.read((char*)btfs, bHeader.lump[LUMP_TEXINFO].nLength);	
	
	//Read Faces and lightmaps
	inBSP.seekg(bHeader.lump[LUMP_FACES].nOffset, ios::beg);
	
	float *minUV = new float[bHeader.lump[LUMP_FACES].nLength/(int)sizeof(BSPFACE)*2];
	float *maxUV = new float[bHeader.lump[LUMP_FACES].nLength/(int)sizeof(BSPFACE)*2];
	
	for(int i=0;i<bHeader.lump[LUMP_FACES].nLength/(int)sizeof(BSPFACE);i++){
		BSPFACE f;
		inBSP.read((char*)&f, sizeof(f));
		BSPTEXTUREINFO b = btfs[f.iTextureInfo];
		string faceTexName = texNames[b.iMiptex];
		
		minUV[i*2] = minUV[i*2+1] = 99999;
		maxUV[i*2] = maxUV[i*2+1] = -99999;
		
		for(int j=2,k=1;j<f.nEdges;j++,k++){	
			Vertex3f v1 = verticesPrime[f.iFirstEdge], v2 = verticesPrime[f.iFirstEdge+k], v3 = verticesPrime[f.iFirstEdge+j];
			COORDS c1 = calcCoords(v1, b.vS, b.vT, b.fSShift, b.fTShift),
				   c2 = calcCoords(v2, b.vS, b.vT, b.fSShift, b.fTShift),
				   c3 = calcCoords(v3, b.vS, b.vT, b.fSShift, b.fTShift);
			
			minUV[i*2] = min(minUV[i*2], c1.u); minUV[i*2+1] = min(minUV[i*2+1], c1.v);
			minUV[i*2] = min(minUV[i*2], c2.u); minUV[i*2+1] = min(minUV[i*2+1], c2.v);
			minUV[i*2] = min(minUV[i*2], c3.u); minUV[i*2+1] = min(minUV[i*2+1], c3.v);
			
			maxUV[i*2] = max(maxUV[i*2], c1.u); maxUV[i*2+1] = max(maxUV[i*2+1], c1.v);
			maxUV[i*2] = max(maxUV[i*2], c2.u); maxUV[i*2+1] = max(maxUV[i*2+1], c2.v);
			maxUV[i*2] = max(maxUV[i*2], c3.u); maxUV[i*2+1] = max(maxUV[i*2+1], c3.v);
		}
		
		int lmw = ceil(maxUV[i*2]/16) - floor(minUV[i*2]/16) + 1;
		int lmh = ceil(maxUV[i*2+1]/16) - floor(minUV[i*2+1]/16) + 1;
		
		if(lmw > 17 || lmh > 17) lmw = lmh = 1;
		LMAP l; l.w = lmw; l.h = lmh; l.offset = lmap+f.nLightmapOffset;
		lmaps.push_back(l);
	}
		
	int lmapRover[1024]; memset(lmapRover, 0, 1024*4);

	//Light map "rover" algorithm from Quake 2 (http://fabiensanglard.net/quake2/quake2_opengl_renderer.php)
	for(unsigned int i=0;i<lmaps.size();i++){
		int best=1024, best2;

		for(int a=0;a<1024-lmaps[i].w;a++){
			best2 = 0;
			int j;
			for(j=0;j<lmaps[i].w;j++){
				if(lmapRover[a+j] >= best)
					break;
				if(lmapRover[a+j] > best2)
					best2 = lmapRover[a+j];
			}
			if(j == lmaps[i].w){
				lmaps[i].finalX = a;
				lmaps[i].finalY = best = best2;
			}
		}

		if(best + lmaps[i].h > 1024){
			cout << "Lightmap atlas is too small (" << filename <<")." << endl;
			break;
		}

		for(int a=0;a<lmaps[i].w;a++)
			lmapRover[lmaps[i].finalX + a] = best + lmaps[i].h;
		
		int finalX = lmaps[i].finalX;
		int finalY = lmaps[i].finalY;
		
		#define ATXY(_x,_y) ((_x)+((_y)*1024))*3
		#define LMXY(_x,_y) ((_x)+((_y)*lmaps[i].w))*3
		for(int y=0;y<lmaps[i].h;y++)
		for(int x=0;x<lmaps[i].w;x++){
			lmapAtlas[ATXY(finalX+x, finalY+y)+0] = gammaTable[lmaps[i].offset[LMXY(x,y)+0]];
			lmapAtlas[ATXY(finalX+x, finalY+y)+1] = gammaTable[lmaps[i].offset[LMXY(x,y)+1]];
			lmapAtlas[ATXY(finalX+x, finalY+y)+2] = gammaTable[lmaps[i].offset[LMXY(x,y)+2]];
		}
	}

	//Load the actual triangles
	
	inBSP.seekg(bHeader.lump[LUMP_FACES].nOffset, ios::beg);
	for(int i=0;i<bHeader.lump[LUMP_FACES].nLength/(int)sizeof(BSPFACE);i++){
		
		
		BSPFACE f;
		inBSP.read((char*)&f, sizeof(f));
		
		if(dontRenderFace[i]) continue;
		
		BSPTEXTUREINFO b = btfs[f.iTextureInfo];
		
		string faceTexName = texNames[b.iMiptex];
		
		//Calculate light map uvs
		int lmw = ceil(maxUV[i*2]/16) - floor(minUV[i*2]/16) + 1;
		int lmh = ceil(maxUV[i*2+1]/16) - floor(minUV[i*2+1]/16) + 1;
		
		if(lmw > 17) continue;
		if(lmh > 17) continue;
		
		float mid_poly_s = (minUV[i*2] + maxUV[i*2])/2.0f;
		float mid_poly_t = (minUV[i*2+1] + maxUV[i*2+1])/2.0f;
		float mid_tex_s = (float)lmw / 2.0f;
		float mid_tex_t = (float)lmh / 2.0f;
		float fX = lmaps[i].finalX;
		float fY = lmaps[i].finalY;
		Texture t = textures[faceTexName];
		
		vector <VECFINAL>*vt = &texturedTris[faceTexName].triangles;
		
		for(int j=2,k=1;j<f.nEdges;j++,k++){	
			Vertex3f v1 = verticesPrime[f.iFirstEdge], v2 = verticesPrime[f.iFirstEdge+k], v3 = verticesPrime[f.iFirstEdge+j];
			COORDS c1 = calcCoords(v1, b.vS, b.vT, b.fSShift, b.fTShift),
				   c2 = calcCoords(v2, b.vS, b.vT, b.fSShift, b.fTShift),
				   c3 = calcCoords(v3, b.vS, b.vT, b.fSShift, b.fTShift);
			
			COORDS c1l, c2l, c3l;
						
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
			
			c1.u /= t.iWidth; c2.u /= t.iWidth; c3.u /= t.iWidth;
			c1.v /= t.iHeight; c2.v /= t.iHeight; c3.v /= t.iHeight;
			
			v1.FixHand();
			v2.FixHand();
			v3.FixHand();
			
			vt->push_back(VECFINAL(v1,c1,c1l));
			vt->push_back(VECFINAL(v2,c2,c2l));
			vt->push_back(VECFINAL(v3,c3,c3l));
		}
		texturedTris[faceTexName].texId = textures[faceTexName].iTextureId;
	}

	
	delete []btfs;
	delete []texOffSets;
	delete []edges;
	
	inBSP.close();
	
	glGenTextures(1, &lmapTexId);		
	glBindTexture(GL_TEXTURE_2D, lmapTexId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, lmapAtlas);
	delete []lmapAtlas;
	
	bufObjects = new GLuint[texturedTris.size()];
	glGenBuffers(texturedTris.size(), bufObjects);
	
	int i=0;
	totalTris=0;
	for(map <string, TEXSTUFF >::iterator it = texturedTris.begin();it != texturedTris.end();it++, i++){	
		glBindBuffer(GL_ARRAY_BUFFER, bufObjects[i]);
		glBufferData(GL_ARRAY_BUFFER, (*it).second.triangles.size()*sizeof(VECFINAL), (void*)&(*it).second.triangles[0], GL_STATIC_DRAW);
		totalTris += (*it).second.triangles.size();
	}
	
	mapId = id;
		
}

void BSP::calculateOffset(){
	if(offsets.count(mapId) != 0){
		offset = offsets[mapId];
	}else{
		if(mapId == "c0a0"){
			//Origin for other maps
			offsets[mapId] = Vertex3f(0,0,0);
		}else{
			float ox=0,oy=0,oz=0;
			bool found=false;
			for(map <string, vector<pair<Vertex3f,string> > >::iterator it = landmarks.begin(); it != landmarks.end();it++){
				if((*it).second.size() > 1){
					for(size_t i=0;i<(*it).second.size();i++){
						if((*it).second[i].second == mapId){
							if(i == 0){
								if(offsets.count((*it).second[i+1].second) != 0){
									Vertex3f c1 = (*it).second[i].first;
									Vertex3f c2 = (*it).second[i+1].first;
									Vertex3f c3 = offsets[(*it).second[i+1].second];
									ox = + c2.x + c3.x - c1.x;
									oy = + c2.y + c3.y - c1.y;
									oz = + c2.z + c3.z - c1.z;
									
									found=true;
									cout << "Matched " << (*it).second[i].second << " " << (*it).second[i+1].second << endl;
									break;
								}
							}else{
								if(offsets.count((*it).second[i-1].second) != 0){
									Vertex3f c1 = (*it).second[i].first;
									Vertex3f c2 = (*it).second[i-1].first;
									Vertex3f c3 = offsets[(*it).second[i-1].second];
									ox = + c2.x + c3.x - c1.x;
									oy = + c2.y + c3.y - c1.y;
									oz = + c2.z + c3.z - c1.z;
									
									found=true;
									cout << "Matched " << (*it).second[i].second << " " << (*it).second[i-1].second << endl;
									break;
								}
							}
							
						}
					}
				}
			}
			if(!found){
				cout << "Cant find matching landmarks for " << mapId << endl;  
			}
			offsets[mapId] = Vertex3f(ox,oy,oz);
		}
	}
}

void BSP::render(){
	//Calculate map offset based on landmarks
	calculateOffset();
	
	glPushMatrix();
	glTranslatef(offset.x + ConfigOffsetChapter.x, offset.y + ConfigOffsetChapter.y, offset.z + ConfigOffsetChapter.z);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glActiveTextureARB(GL_TEXTURE1_ARB); 
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, lmapTexId);

	glClientActiveTextureARB(GL_TEXTURE1_ARB); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
	
	int i=0;
	for(map <string, TEXSTUFF >::iterator it = texturedTris.begin();it != texturedTris.end();it++, i++){
		//Don't render some dummy triangles (triggers and such)
		if((*it).first != "aaatrigger" && (*it).first != "origin" && (*it).first != "clip" && (*it).first != "sky" && (*it).first[0]!='{' && (*it).second.triangles.size() != 0){
			//if(mapId == "c1a0e.bsp") cout << (*it).first << endl;
			glBindBuffer(GL_ARRAY_BUFFER, bufObjects[i]);
			
			/////T0
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, (*it).second.texId);
			
			glClientActiveTextureARB(GL_TEXTURE0_ARB); 
			glTexCoordPointer(2, GL_FLOAT, sizeof(VECFINAL), (char*)NULL+4*3);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY); 
			
			////T1
			glClientActiveTextureARB(GL_TEXTURE1_ARB); 
			glTexCoordPointer(2, GL_FLOAT, sizeof(VECFINAL), (char*)NULL+4*5);
			
			glVertexPointer(3, GL_FLOAT, sizeof(VECFINAL), (void*)0);
			glDrawArrays(GL_TRIANGLES, 0, (*it).second.triangles.size());
		}
	}
	glPopMatrix();
}

void BSP::SetChapterOffset(const float x, const float y, const float z)
{
	ConfigOffsetChapter.x = x;
	ConfigOffsetChapter.y = y;
	ConfigOffsetChapter.z = z;
}
