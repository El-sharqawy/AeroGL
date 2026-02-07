#include "TerrainMap.h"
#include "../../Stdafx.h"

bool TerrainMap_Initialize(TerrainMap* ppTerrainMap)
{
	if (ppTerrainMap == NULL)
	{
		syserr("ppTerrainMap is NULL (invalid address)");
		return false;
	}

	// Avoid Memory Leaks
	if (*ppTerrainMap == NULL)
	{
		*ppTerrainMap = engine_new_zero(STerrainMap, 1, MEM_TAG_TERRAIN);
	}

	if ((*ppTerrainMap) == NULL)
	{
		syserr("Failed to Allocate Memory for Terrain Map");
		return (false);
	}

	return (true);
}

void TerrainMap_Destroy(TerrainMap* ppTerrainMap)
{
	if (!ppTerrainMap || !*ppTerrainMap)
	{
		return;
	}

	TerrainMap pTerrainMap = *ppTerrainMap;

	Vector_Destroy(&pTerrainMap->terrains);

	if (pTerrainMap->szMapName)
	{
		engine_delete(pTerrainMap->szMapName);
	}

	if (pTerrainMap->szMapDir)
	{
		engine_delete(pTerrainMap->szMapDir);
	}

	engine_delete(pTerrainMap);

	*ppTerrainMap = NULL;
}

void TerrainMap_Clear(TerrainMap pTerrainMap)
{
	if (pTerrainMap)
	{
		pTerrainMap->isReady = false;

		// Clear
		Vector_Destroy(&pTerrainMap->terrains);
		if (pTerrainMap->szMapName)
		{
			engine_delete(pTerrainMap->szMapName);
			pTerrainMap->szMapName = NULL;
		}
		if (pTerrainMap->szMapDir)
		{
			engine_delete(pTerrainMap->szMapDir);
			pTerrainMap->szMapDir = NULL;
		}
	}
}

void TerrainMap_SetDeminsions(TerrainMap pTerrainMap, int32_t terrainsX, int32_t terrainsZ)
{
	pTerrainMap->terrainsXCount = terrainsX;
	pTerrainMap->terrainsZCount = terrainsZ;
}

void TerrainMap_SetMapName(TerrainMap pTerrainMap, const char* szMapName)
{
	if (pTerrainMap->szMapName)
	{
		engine_delete(pTerrainMap->szMapName);
		pTerrainMap->szMapName = NULL;
	}

	pTerrainMap->szMapName = engine_strdup(szMapName, MEM_TAG_STRINGS);
}

void TerrainMap_SetMapDir(TerrainMap pTerrainMap, const char* szMapDir)
{
	if (pTerrainMap->szMapDir)
	{
		engine_delete(pTerrainMap->szMapDir);
		pTerrainMap->szMapDir = NULL;
	}

	pTerrainMap->szMapDir = engine_strdup(szMapDir, MEM_TAG_STRINGS);
}
