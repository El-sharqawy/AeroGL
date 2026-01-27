#ifndef __TERRAIN_MANAGER_H__
#define __TERRAIN_MANAGER_H__

#include "../TerrainMap/TerrainMap.h"
#include "../../Renderer/TerrainRenderer.h"

typedef struct STerrainManager
{
	TerrainMap pTerrainMap;
	bool isMapReady;

	// Map Creation Part
	char* szMapName;
	int32_t mapWidth;
	int32_t mapDepth;

	// renderer
	TerrainRenderer terarinRenderer;
	bool bNeedsUpdate;
} STerrainManager;

typedef struct STerrainManager* TerrainManager;

bool TerrainManager_Initialize(TerrainManager* ppTerrainManager);
void TerrainManager_Destroy(TerrainManager* ppTerrainManager);
void TerrainManager_Clear(TerrainManager pTerrainManager);

void TerrainManager_Update(TerrainManager pTerrainManager);
void TerrainManager_Render(TerrainManager pTerrainManager);

// Map Creation
void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName);
void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth);
void TerrainManager_CreateMap(TerrainManager pTerrainManager);

// Singleton ~
const TerrainManager GetTerrainManager();
static TerrainManager psTerrainManager;

#endif // __TERRAIN_MANAGER_H__