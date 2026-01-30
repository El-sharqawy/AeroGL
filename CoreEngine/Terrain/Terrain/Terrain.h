#ifndef __TERRAIN_H__
#define __TERRAIN_H__

#include <stdbool.h>
#include <stdint.h>

#include "../TerrainData.h"
#include "../../Lib/Vector.h"
#include "../../Math/Matrix/Matrix4.h"
#include "../../Math/Transform.h"

typedef struct STerrain
{
	Vector terrainPatches;	// PATCH_XCOUNT * PATCH_ZCOUNT, as size of TerrainPatch
	Matrix4 patchesMetrices[TERRAIN_PATCH_COUNT];    // CPU-side storage
	int32_t terrainIndex;	// Terrain Index in our world
	int32_t terrainXCoord;	// Terrain Num Among X Axis
	int32_t terrainZCoord;	// Terrain Num Among Z Axis
	bool isInitialized;		// Terrain is Initialized?
	bool bIsReady;			// Terrain is ready to render ?
	STransform transform;	// Terrain Position
	int32_t baseGlobalPatchIndex;

	struct STerrainMap* pParentMap;
	struct SFloatGrid* heightMap;
} STerrain;

typedef struct STerrain* Terrain;

bool Terrain_Initialize(Terrain* ppTerrain);
void Terrain_Destroy(Terrain* ppTerrain);
void Terrain_DestroyPtr(Terrain pTerrain);

bool Terrain_Load(Terrain pTerrain);
void Terrain_SetTerrainCoords(Terrain pTerrain, int32_t iTerrX, int32_t iTerrZ);
void Terrain_SetTerrainIndex(Terrain pTerrain, int32_t iTerrainIndex);

bool Terrain_InitializePatches(Terrain pTerrain);

void Terrain_UpdatePatches(Terrain pTerrain);
void Terrain_UpdatePatch(Terrain pTerrain, int32_t iPatchNumX, int32_t iPatchNumZ);

void Terrain_SetParentMap(Terrain pTerrain, struct STerrainMap* pParentMap);
struct STerrainMap* Terrain_GetParentMap(Terrain pTerrain);

float GetHeightMapValue(Terrain pTerrain, int32_t x, int32_t z);

// Terrains Loading And Creating
bool Terrain_CreateFiles(struct STerrainMap* pParentMap, int32_t iTerrainX, int32_t iTerrainZ);
bool Terrain_CreateHeightMap(Terrain pTerrain, const char* szTerrainsFolder);

bool Terrain_LoadHeightMap(Terrain pTerrain, const char* szTerrainsFolder);
bool Terrain_SaveHeightMap(Terrain pTerrain, const char* szTerrainsFolder);

#endif // __TERRAIN_H__