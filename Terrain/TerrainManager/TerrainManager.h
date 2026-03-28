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
void TerrainManager_Clear();

void TerrainManager_Update();
void TerrainManager_Render();

// Manager Editor
bool TerrainManager_CreateMap();
bool TerrainManager_LoadMap(char* szMapName);
bool TerrainManager_SaveMap();

// Manager Editor Map Accessors
void TerrainManager_SetMapName(const char* szMapName);
void TerrainManager_SetMapDeminsions(int32_t mapWidth, int32_t mapDepth);

// Manager Editor Brush Accessors
void TerrainManager_SetBrushType(GLint brushType);
void TerrainManager_SetBrushShape(GLint brushShape);
void TerrainManager_SetBrushStrength(GLint brushStrength);
void TerrainManager_SetBrushMaxStrength(GLint brushMaxStrength);
void TerrainManager_SetBrushSize(GLint brushSize);
void TerrainManager_SetBrushMaxSize(GLint brushMaxSize);

// Manager Editor Edit Accessors
void TerrainManager_SetEditX(GLint editX);
void TerrainManager_SetEditZ(GLint editZ);
void TerrainManager_SetEditXZ(GLint editX, GLint editZ);
void TerrainManager_SetSubCellX(GLint subCellX);
void TerrainManager_SetSubCellZ(GLint subCellZ);
void TerrainManager_SetSubCellXZ(GLint subCellX, GLint subCellZ);
void TerrainManager_SetEditTerrainNumX(GLint editTerrainNumX);
void TerrainManager_SetEditTerrainNumZ(GLint editTerrainNumZ);
void TerrainManager_SetEditTerrainNumXZ(GLint editTerrainNumX, GLint editTerrainNumZ);
void TerrainManager_SetPickingPoint(Vector3 v3PickingPoint);

void TerrainManager_SetIsEditingTerrain(bool isEditingTerrain);
void TerrainManager_SetIsEditingHeight(bool isEditingHeight);

// Singleton ~
const TerrainManager GetTerrainManager();
static TerrainManager psTerrainManager;

#ifdef __cplusplus
}
#endif

#endif // __TERRAIN_MANAGER_H__