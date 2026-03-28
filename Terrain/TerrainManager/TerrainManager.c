#include "Stdafx.h"
#include "TerrainManager.h"
#include "Terrain/TerrainMap/TerrainMap.h"
#include "Renderer/TerrainRenderer.h"
#include "PipeLine/Texture.h"

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

	psTerrainManager->terrainTex->isBindless = false;

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

	if (pManager->editor.szMapName)
	{
		engine_delete(pManager->editor.szMapName);
	}

	TerrainRenderer_Destroy(&pManager->terarinRenderer);
	
	TerrainMap_Destroy(&pManager->pTerrainMap);

	Texture_Destroy(&pManager->terrainTex);

	// Destroy Manager
	engine_delete(pManager);

	psTerrainManager = NULL;
	*ppTerrainManager = NULL;
}

void TerrainManager_Clear()
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	if (terrMgr->pTerrainMap)
	{
		TerrainMap_Clear(terrMgr->pTerrainMap);
		TerrainRenderer_Reset(terrMgr->terarinRenderer);

		terrMgr->isMapReady = false;
	}
}

void TerrainManager_Update()
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	if (!terrMgr->isMapReady)
	{
		return;
	}

	if (terrMgr->bNeedsUpdate)
	{
		if (terrMgr->pTerrainMap && terrMgr->pTerrainMap->isReady)
		{
			// Upload GPU Data
			TerrainRenderer_UploadGPUData(terrMgr->terarinRenderer);
			TerrainMap_Update(terrMgr->pTerrainMap);
			terrMgr->bNeedsUpdate = false;
		}
	}
}

void TerrainManager_Render()
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	if (terrMgr->isMapReady)
	{
		TerrainRenderer_Render(terrMgr->terarinRenderer);
	}
}

const TerrainManager GetTerrainManager()
{
	return (psTerrainManager);
}
