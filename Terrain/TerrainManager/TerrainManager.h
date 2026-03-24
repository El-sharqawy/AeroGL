#ifndef __TERRAIN_MANAGER_H__
#define __TERRAIN_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>
#include "Math/Vectors/Vector3.h"

// Use forward declarations if possible to prevent circular includes
typedef struct STerrainMap* TerrainMap;
typedef struct STerrainRenderer* TerrainRenderer;
typedef struct STexture* Texture;

typedef struct STerrainManagerEditor
{
	// Map Creation Vars
	char* szMapName;
	int32_t mapWidth;
	int32_t mapDepth;

	// Brush Vars
	GLint brushType;
	GLint brushShape;
	GLint brushStrength;
	GLint brushMaxStrength;
	GLint brushSize;
	GLint brushMaxSize;

	// Edit Vars
	GLint editX;
	GLint editZ;
	GLint subCellX;
	GLint subCellZ;
	GLint editTerrainNumX;
	GLint editTerrainNumZ;
	Vector3 v3PickingPoint;

	bool isEditingTerrain;
	bool isEditingHeight;
} STerrainManagerEditor;

typedef struct STerrainManager
{
	// Editor state — lives/dies with manager
	STerrainManagerEditor editor;

	TerrainMap pTerrainMap;

	// renderer
	TerrainRenderer terarinRenderer;
	Texture terrainTex;
	bool isMapReady;
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

// Map Load
bool TerrainManager_LoadMap(TerrainManager pTerrainManager, char* szMapName);
bool TerrainManager_SaveMap(TerrainManager pTerrainManager);

// Map Editor Part
// Map Creation
void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName);
void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth);
bool TerrainManager_CreateMap(TerrainManager pTerrainManager);

// Singleton ~
const TerrainManager GetTerrainManager();
static TerrainManager psTerrainManager;

#ifdef __cplusplus
}
#endif

#endif // __TERRAIN_MANAGER_H__