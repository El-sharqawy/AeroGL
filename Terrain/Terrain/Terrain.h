#ifndef __TERRAIN_H__
#define __TERRAIN_H__

#include <stdbool.h>
#include <stdint.h>

#include "../TerrainData.h"
#include "AeroLib/Vector.h"
#include "Math/Matrix/Matrix4.h"
#include "Math/Transform.h"

typedef struct STerrain
{
	STransform transform;	// Terrain Position
	Vector terrainPatches;	// PATCH_XCOUNT * PATCH_ZCOUNT, as size of TerrainPatch
	int32_t terrainIndex;	// Terrain Index in our world
	int32_t terrainXCoord;	// Terrain Num Among X Axis
	int32_t terrainZCoord;	// Terrain Num Among Z Axis
	int32_t baseGlobalPatchIndex;

	struct STerrainMap* parentMap;
	struct SFloatGrid* heightMap;
	struct STexture* pHeightMapTexture;

	// Height map direct access, Triple MAPPING — 3 pointers to SAME texture backing storage
	void* mapPtrs[3];		// Local -> global offset
	GLsync fences[3];
	int32_t currentRing;
	size_t sliceBytes;		// HEIGHTMAP_RAW_XSIZE * HEIGHTMAP_RAW_ZSIZE * sizeof(float)
	size_t globalOffset;	// Set by renderer

	bool isInitialized;		// Terrain is Initialized?
	bool bIsReady;			// Terrain is ready to render ?
} STerrain;

typedef struct STerrain* Terrain;

bool Terrain_Initialize(Terrain* ppTerrain);
void Terrain_Destroy(Terrain* ppTerrain);
void Terrain_DestroyPtr(Terrain pTerrain);

void Terrain_SetTerrainCoords(Terrain pTerrain, int32_t iTerrX, int32_t iTerrZ);
void Terrain_SetTerrainIndex(Terrain pTerrain, int32_t iTerrainIndex);

bool Terrain_InitializePatches(Terrain pTerrain);

void Terrain_UpdatePatches(Terrain pTerrain);
void Terrain_UpdatePatch(Terrain pTerrain, int32_t iPatchNumX, int32_t iPatchNumZ);
void Terrain_Update(Terrain pTerrain);

void Terrain_SetParentMap(Terrain pTerrain, struct STerrainMap* pParentMap);
struct STerrainMap* Terrain_GetParentMap(Terrain pTerrain);

float GetHeightMapValue(Terrain pTerrain, int32_t x, int32_t z);

// Terrains Loading And Creating
bool Terrain_CreateFiles(struct STerrainMap* pParentMap, int32_t iTerrainX, int32_t iTerrainZ);
bool Terrain_CreateHeightMap(Terrain pTerrain, const char* szTerrainsFolder);

bool Terrain_LoadHeightMap(Terrain pTerrain, const char* szTerrainsFolder);
bool Terrain_SaveHeightMap(Terrain pTerrain, const char* szTerrainsFolder);

bool Terrain_Load(Terrain pTerrain);
bool Terrain_LoadHeightMapTexture(Terrain pTerrain);
bool Terrain_LoadHeightMapSSBO(Terrain pTerrain);

#endif // __TERRAIN_H__
