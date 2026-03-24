#include "Stdafx.h"
#include "TerrainManager.h"
#include "Terrain/TerrainMap/TerrainMap.h"

void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName)
{
	pTerrainManager->editor.szMapName = engine_strdup(szMapName, MEM_TAG_STRINGS);
}

void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth)
{
	// Number of Terrains Among X-Z Axis
	pTerrainManager->editor.mapWidth = mapWidth;
	pTerrainManager->editor.mapDepth = mapDepth;
}

bool TerrainManager_CreateMap(TerrainManager pTerrainManager)
{
	if (pTerrainManager->editor.szMapName == NULL)
	{
		syserr("You need to enter Map Name");
		return (false);
	}

	if (pTerrainManager->editor.mapWidth == 0 || pTerrainManager->editor.mapDepth == 0)
	{
		syserr("You need to enter Map Deminsions (x, z)");
		return (false);
	}

	// Create an empty map
	if (!TerrainMap_CreateMap(pTerrainManager->editor.szMapName, pTerrainManager->editor.mapWidth, pTerrainManager->editor.mapDepth))
	{
		syserr("Failed to Initialize Terrain Map");
		return (false);  // Cleanup everything above ?
	}

	return (true);
}