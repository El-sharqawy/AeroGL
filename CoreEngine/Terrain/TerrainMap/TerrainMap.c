#include "TerrainMap.h"

bool TerrainMap_Initialize(TerrainMap* ppTerrainMap)
{
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
		}
		if (pTerrainMap->szMapDir)
		{
			tracked_free(pTerrainMap->szMapDir);
		}

		// Init Terrains
		//if (!Vector_Init(&pTerrainMap->terrains, sizeof(Terrain)))
		//{
		//	TerrainMap_Destroy(&pTerrainMap);
		//	syserr("Failed to Initialize Terrain Map Vector");
		//}
	}
}

bool TerrainMap_Load(TerrainMap pTerrainMap)
{
	if (pTerrainMap->terrainsZCount == 0 && pTerrainMap->terrainsXCount == 0)
	{
		syserr("You need to set Map Deminsions");
		return (false);
	}

	// Init Terrains
	if (!Vector_InitCapacity(&pTerrainMap->terrains, sizeof(Terrain), pTerrainMap->terrainsXCount * pTerrainMap->terrainsZCount))
	{
		TerrainMap_Destroy(&pTerrainMap);
		syserr("Failed to Initialize Terrain Map Vector");
		return (false);
	}

	// Auto CleanUp method
	pTerrainMap->terrains->destructor = Terrain_DestroyPtr;

	// Create Terrains Among X-Z Axis
	for (int32_t iTerrZ = 0; iTerrZ < pTerrainMap->terrainsZCount; iTerrZ++)
	{
		for (int32_t iTerrX = 0; iTerrX < pTerrainMap->terrainsXCount; iTerrX++)
		{
			Terrain pTerrain = NULL;
			int32_t iTerrainIndex = iTerrZ * pTerrainMap->terrainsXCount + iTerrX;

			if (!Terrain_Initialize(&pTerrain))
			{
				syserr("Failed to Create Terrain at coord (%d, %d)", iTerrX, iTerrZ);
				TerrainMap_Destroy(&pTerrainMap);
				return (false);
			}

			Terrain_SetTerrainCoords(pTerrain, iTerrX, iTerrZ);
			Terrain_SetTerrainIndex(pTerrain, iTerrainIndex);

			if (!Terrain_Load(pTerrain))
			{
				syserr("Failed to Load Terrain at coord (%d, %d)", iTerrX, iTerrZ);
				TerrainMap_Destroy(&pTerrainMap);
				return (false);
			}

			// Initialize Patches
			if (!Terrain_InitializePatches(pTerrain))
			{
				Terrain_Destroy(&pTerrain);
				syserr("Failed to Initialize Terrain Patches");
				return (false);
			}

			Terrain_SetParentMap(pTerrain, pTerrainMap);
			Vector_PushBack(&pTerrainMap->terrains, &pTerrain);
		}
	}

	pTerrainMap->isReady = true;
	return (true);
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
	}

	pTerrainMap->szMapName = tracked_strdup(szMapName);
}

void TerrainMap_SetMapDir(TerrainMap pTerrainMap, const char* szMapDir)
{
	if (pTerrainMap->szMapDir)
	{
		tracked_free(pTerrainMap->szMapDir);
	}

	pTerrainMap->szMapDir = tracked_strdup(szMapDir);
}
