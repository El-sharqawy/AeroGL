#include "TerrainMap.h"

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
		*ppTerrainMap = tracked_calloc(1, sizeof(STerrainMap));
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
		tracked_free(pTerrainMap->szMapName);
	}

	if (pTerrainMap->szMapDir)
	{
		tracked_free(pTerrainMap->szMapDir);
	}

	tracked_free(pTerrainMap);

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
			tracked_free(pTerrainMap->szMapName);
			pTerrainMap->szMapName = NULL;
		}
		if (pTerrainMap->szMapDir)
		{
			tracked_free(pTerrainMap->szMapDir);
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
		tracked_free(pTerrainMap->szMapName);
		pTerrainMap->szMapName = NULL;
	}

	pTerrainMap->szMapName = tracked_strdup(szMapName);
}

void TerrainMap_SetMapDir(TerrainMap pTerrainMap, const char* szMapDir)
{
	if (pTerrainMap->szMapDir)
	{
		tracked_free(pTerrainMap->szMapDir);
		pTerrainMap->szMapDir = NULL;
	}

	pTerrainMap->szMapDir = tracked_strdup(szMapDir);
}
