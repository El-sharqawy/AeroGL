#ifndef __TERRAIN_MANAGER_H__
#define __TERRAIN_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>

// Use forward declarations if possible to prevent circular includes
typedef struct STerrainMap* TerrainMap;
typedef struct STerrainRenderer* TerrainRenderer;

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

#ifdef __cplusplus
extern "C" {
#endif
bool TerrainManager_Initialize(TerrainManager* ppTerrainManager);
void TerrainManager_Destroy(TerrainManager* ppTerrainManager);
void TerrainManager_Clear(TerrainManager pTerrainManager);

void TerrainManager_Update(TerrainManager pTerrainManager);
void TerrainManager_Render(TerrainManager pTerrainManager);

// Map Creation
void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName);
void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth);
bool TerrainManager_CreateMap(TerrainManager pTerrainManager);

// Map Load
bool TerrainManager_LoadMap(TerrainManager pTerrainManager, char* szMapName);

// Singleton ~
const TerrainManager GetTerrainManager();
static TerrainManager psTerrainManager;

#ifdef __cplusplus
}
#endif

#endif // __TERRAIN_MANAGER_H__