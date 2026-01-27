#include "TerrainManager.h"
#include "../../Engine.h"

bool TerrainManager_Initialize(TerrainManager* ppTerrainManager)
{
	*ppTerrainManager = tracked_calloc(1, sizeof(STerrainManager));

	if (!(*ppTerrainManager))
	{
		syserr("Failed to Allocate Memory for TerrainManager");
		return (false);
	}

	psTerrainManager = *ppTerrainManager;

	// Initialize Renderer
	if (!TerrainRenderer_Initialize(&psTerrainManager->terarinRenderer, GetEngine()->camera, "Terrain Renderer"))
	{
		syserr("Failed to Create Terrain Renderer");
		TerrainManager_Destroy(ppTerrainManager);
		return (false);
	}

	// Create an empty map
	if (!TerrainMap_Initialize(&psTerrainManager->pTerrainMap))
	{
		syserr("Failed to Initialize Terrain Map");
		TerrainManager_Destroy(ppTerrainManager);
		return (false);  // Cleanup everything above
	}

	// Upload GPU Data
	// TerrainRenderer_UploadGPUData(psTerrainManager->terarinRenderer, psTerrainManager->pTerrainMap);

	TerrainMap_SetDeminsions(psTerrainManager->pTerrainMap, 1, 1);

	if (!TerrainMap_Load(psTerrainManager->pTerrainMap))
	{
		syserr("Failed to Load Map");
		TerrainManager_Destroy(ppTerrainManager);
		return (false);
	}

	psTerrainManager->isMapReady = true;
	psTerrainManager->bNeedsUpdate = true;

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
		tracked_free(pManager->szMapName);
	}

	TerrainRenderer_Destroy(&pManager->terarinRenderer);
	
	TerrainMap_Destroy(&pManager->pTerrainMap);

	// Destroy Manager
	tracked_free(pManager);

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
	if (pTerrainManager->bNeedsUpdate)
	{
		if (psTerrainManager->pTerrainMap && psTerrainManager->pTerrainMap->isReady)
		{
			// Upload GPU Data
			TerrainRenderer_UploadGPUData(psTerrainManager->terarinRenderer, psTerrainManager->pTerrainMap);
			pTerrainManager->bNeedsUpdate = false;
		}
	}
}

void TerrainManager_Render(TerrainManager pTerrainManager)
{
	if (pTerrainManager->isMapReady)
	{
		TerrainRenderer_Render(pTerrainManager->terarinRenderer, pTerrainManager->pTerrainMap);
	}
}

void TerrainManager_SetMapName(TerrainManager pTerrainManager, const char* szMapName)
{
	pTerrainManager->szMapName = tracked_strdup(szMapName);
}

void TerrainManager_SetMapDeminsions(TerrainManager pTerrainManager, int32_t mapWidth, int32_t mapDepth)
{
	// Number of Terrains Among X-Z Axis
	pTerrainManager->mapWidth = mapWidth;
	pTerrainManager->mapDepth = mapDepth;
}

void TerrainManager_CreateMap(TerrainManager pTerrainManager)
{
	if (!pTerrainManager->pTerrainMap)
	{
		syserr("Failed to Initialize Terrain Map, There is no Map");
		return;
	}

	if (pTerrainManager->szMapName == NULL)
	{
		syserr("You need to enter Map Name");
		return;
	}

	if (pTerrainManager->mapWidth == 0 || pTerrainManager->mapDepth == 0)
	{
		syserr("You need to enter Map Deminsions (x, z)");
		return;
	}

	// Create an empty map
	if (!TerrainMap_CreateMap(pTerrainManager->szMapName, pTerrainManager->mapWidth, pTerrainManager->mapDepth))
	{
		syserr("Failed to Initialize Terrain Map");
		return;  // Cleanup everything above
	}

	pTerrainManager->isMapReady = true;
	pTerrainManager->bNeedsUpdate = true;
}

const TerrainManager GetTerrainManager()
{
	return (psTerrainManager);
}
