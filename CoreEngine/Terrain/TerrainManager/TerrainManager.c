#include "TerrainManager.h"
#include "../TerrainMap/TerrainMap.h"
#include "../../Renderer/TerrainRenderer.h"
#include "../../Stdafx.h"
#include "../../PipeLine/Texture.h"

bool TerrainManager_Initialize(TerrainManager* ppTerrainManager)
{
	if (ppTerrainManager == NULL)
	{
		syserr("ppTerrainManager is NULL (invalid address)");
		return false;
	}

	*ppTerrainManager = engine_new_zero(STerrainManager, 1, MEM_TAG_TERRAIN);

	if (!(*ppTerrainManager))
	{
		syserr("Failed to Allocate Memory for TerrainManager");
		return (false);
	}

	psTerrainManager = *ppTerrainManager;

	// Create an empty map
	if (!TerrainMap_Initialize(&psTerrainManager->pTerrainMap))
	{
		syserr("Failed to Initialize Terrain Map");
		TerrainManager_Destroy(ppTerrainManager);
		return (false);  // Cleanup everything above
	}

	if (!Texture_Initialize(&psTerrainManager->terrainTex))
	{
		TerrainManager_Destroy(ppTerrainManager);
		syserr("Failed to Initialize terrain texture");
		return (false);
	}

	psTerrainManager->terrainTex->isBindless = true;

	if (!Texture_Load(psTerrainManager->terrainTex, "Assets/Textures/grass01.png"))
	{
		TerrainManager_Destroy(ppTerrainManager);
		syserr("Failed to Load terrain texture");
		return (false);
	}

	psTerrainManager->isMapReady = false;

	return (true);
}

void TerrainManager_Destroy(TerrainManager* ppTerrainManager)
{
	if (!ppTerrainManager|| !(*ppTerrainManager))
	{
		return;
	}

	TerrainManager pManager = *ppTerrainManager;

	if (pManager->szMapName)
	{
		engine_delete(pManager->szMapName);
	}

	TerrainRenderer_Destroy(&pManager->terarinRenderer);
	
	TerrainMap_Destroy(&pManager->pTerrainMap);

	Texture_Destroy(&pManager->terrainTex);

	// Destroy Manager
	engine_delete(pManager);

	*ppTerrainManager = NULL;
}

void TerrainManager_Clear(TerrainManager pTerrainManager)
{
	if (pTerrainManager->pTerrainMap)
	{
		TerrainMap_Clear(pTerrainManager->pTerrainMap);
		TerrainRenderer_Reset(pTerrainManager->terarinRenderer);

		pTerrainManager->isMapReady = false;
	}
}

void TerrainManager_Update(TerrainManager pTerrainManager)
{
	if (!psTerrainManager->isMapReady)
	{
		return;
	}

	if (pTerrainManager->bNeedsUpdate)
	{
		if (psTerrainManager->pTerrainMap && psTerrainManager->pTerrainMap->isReady)
		{
			// Upload GPU Data
			TerrainRenderer_UploadGPUData(psTerrainManager->terarinRenderer);
			pTerrainManager->bNeedsUpdate = false;
		}
	}
}

void TerrainManager_Render(TerrainManager pTerrainManager)
{
	if (pTerrainManager->isMapReady)
	{
		TerrainRenderer_Render(pTerrainManager->terarinRenderer);
	}
}

void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName)
{
	pTerrainManager->szMapName = engine_strdup(szMapName, MEM_TAG_STRINGS);
}

void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth)
{
	// Number of Terrains Among X-Z Axis
	pTerrainManager->mapWidth = mapWidth;
	pTerrainManager->mapDepth = mapDepth;
}

bool TerrainManager_CreateMap(TerrainManager pTerrainManager)
{
	if (pTerrainManager->szMapName == NULL)
	{
		syserr("You need to enter Map Name");
		return (false);
	}

	if (pTerrainManager->mapWidth == 0 || pTerrainManager->mapDepth == 0)
	{
		syserr("You need to enter Map Deminsions (x, z)");
		return (false);
	}

	// Create an empty map
	if (!TerrainMap_CreateMap(pTerrainManager->szMapName, pTerrainManager->mapWidth, pTerrainManager->mapDepth))
	{
		syserr("Failed to Initialize Terrain Map");
		return (false);  // Cleanup everything above ?
	}

	return (true);
}

bool TerrainManager_LoadMap(TerrainManager pTerrainManager, char* szMapName)
{
	if (pTerrainManager->pTerrainMap)
	{
		TerrainMap_Clear(pTerrainManager->pTerrainMap);
	}

	if (!TerrainMap_LoadMap(pTerrainManager->pTerrainMap, szMapName))
	{
		syserr("Failed to Load Map %s", szMapName);
		return (false);
	}

	if (pTerrainManager->pTerrainMap == NULL)
	{
		syserr("Failed to Load Map");
		return (false);
	}

	// Initialize Renderer
	if (!TerrainRenderer_Initialize(&psTerrainManager->terarinRenderer, "Terrain Renderer", pTerrainManager->pTerrainMap->terrainsXCount, pTerrainManager->pTerrainMap->terrainsZCount))
	{
		syserr("Failed to Create Terrain Renderer");
		return (false);
	}

	psTerrainManager->isMapReady = true;
	psTerrainManager->bNeedsUpdate = true;

	return (true);
}

bool TerrainManager_SaveMap(TerrainManager pTerrainManager)
{
	if (!pTerrainManager)
	{
		/// ????? not sure if this condition is useful .. maybe if the code is part of the game (still need terrain manager to load maps)
		return (false);
	}

	if (pTerrainManager->isMapReady == false)
	{
		syserr("Map is not Ready, you Need Map Ready to Save");
		return (false);
	}

	return (TerrainMap_SaveMap(pTerrainManager->pTerrainMap));
}

const TerrainManager GetTerrainManager()
{
	return (psTerrainManager);
}
