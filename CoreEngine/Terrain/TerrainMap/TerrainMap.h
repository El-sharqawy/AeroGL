#ifndef __TERRAIN_MAP_H__
#define __TERRAIN_MAP_H__

#include "../Terrain/Terrain.h"

typedef struct STerrainMap
{
	Vector terrains;
	int32_t terrainsXCount; // number of terrains among X Axis
	int32_t terrainsZCount; // Number of Terrains Among Z Axis
	bool isReady; // is the map ready to render

	char* szMapName;
	char* szMapDir;
} STerrainMap;

typedef struct STerrainMap* TerrainMap;

bool TerrainMap_Initialize(TerrainMap* ppTerrainMap);
void TerrainMap_Destroy(TerrainMap* ppTerrainMap);

void TerrainMap_Clear(TerrainMap pTerrainMap);

void TerrainMap_SetDeminsions(TerrainMap pTerrainMap, int32_t terrainsX, int32_t terrainsZ);
void TerrainMap_SetMapName(TerrainMap pTerrainMap, const char* szMapName);
void TerrainMap_SetMapDir(TerrainMap pTerrainMap, const char* szMapDir);

// Terrain Map Create
bool TerrainMap_CreateFolder(TerrainMap pTerrainMap, char* szMapName);
bool TerrainMap_CreateSettingsFile(TerrainMap pTerrainMap);
bool TerrainMap_CreateMap(char* szMapName, int32_t terrainsX, int32_t terrainsZ);

// Terrain Map Load
bool TerrainMap_LoadMap(TerrainMap pTerrainMap, char* szMapName);
bool TerrainMap_LoadSettingsFile(TerrainMap pTerrainMap, const char* szMapPath);
bool TerrainMap_LoadTerrain(TerrainMap pTerrainMap, int32_t iTerrainX, int32_t iTerrainZ);
bool TerrainMap_IsTerrainLoaded(TerrainMap pTerrainMap, int32_t iTerrainX, int32_t iTerrainZ);

#endif // __TERRAIN_MAP_H__