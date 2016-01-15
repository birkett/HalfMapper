#ifndef BSP_H
#define BSP_H

#include <map>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

//Extracted from http://hlbsp.sourceforge.net/index.php?content=bspdef

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
#define MAXTEXTURENAME 16
#define MIPLEVELS 4

struct MapEntry; // Dont include ConfigXML.h here.
struct Texture;
class  VideoSystem;

struct Vertex3f
{
	Vertex3f()
	{
		x = y = z = 0.0f;
	}

	Vertex3f(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	void FixHand()
	{
		float swapY = y;
		y = z;
		z = swapY;
		x = -x;
	}

	float x;
	float y;
	float z;

};


struct BSPLUMP{
	int32_t nOffset; // File offset to data
	int32_t nLength; // Length of data
};
struct BSPHeader{
	int32_t nVersion;           // Must be 30 for a valid HL BSP file
	BSPLUMP lump[HEADER_LUMPS]; // Stores the directory of lumps
};
struct BSPFACE{
	uint16_t iPlane;          // Plane the face is parallel to
	uint16_t nPlaneSide;      // Set if different normals orientation
	uint32_t iFirstEdge;      // Index of the first surfedge
	uint16_t nEdges;          // Number of consecutive surfedges
	uint16_t iTextureInfo;    // Index of the texture info structure
	uint8_t nStyles[4];       // Specify lighting styles
	uint32_t nLightmapOffset; // Offsets into the raw lightmap data
};
struct BSPEDGE{
	uint16_t iVertex3f[2]; // Indices into Vertex3f array
};
struct BSPTEXTUREINFO{
	Vertex3f vS;
	float fSShift;    // Texture shift in s direction
	Vertex3f vT;
	float fTShift;    // Texture shift in t direction
	uint32_t iMiptex; // Index into textures array
	uint32_t nFlags;  // Texture flags, seem to always be 0
};
struct BSPTEXTUREHEADER{
	uint32_t nMipTextures; // Number of BSPMIPTEX structures
};

#define MAX_MAP_HULLS 4
struct BSPMODEL{
    float nMins[3], nMaxs[3];          // Defines bounding box
    Vertex3f vOrigin;                  // Coordinates to move the // coordinate system
    int32_t iHeadnodes[MAX_MAP_HULLS]; // Index into nodes array
    int32_t nVisLeafs;                 // ???
    int32_t iFirstFace, nFaces;        // Index and count into faces
};

struct COORDS{
	float u, v;
};
struct VECFINAL{
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

struct LMAP{
	unsigned char *offset; int w,h;
	int finalX, finalY;
};

struct TEXSTUFF{
	vector <VECFINAL> triangles;
	int texId;
};

class BSP{
	public:
		BSP(const std::vector<std::string> &szGamePaths, const MapEntry &sMapEntry, VideoSystem* &videosystem);
		~BSP();
		void Render();
		size_t totalTris;
		void SetChapterOffset(const float x, const float y, const float z);

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
		map <string, TEXSTUFF > texturedTris;
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
};

#endif
