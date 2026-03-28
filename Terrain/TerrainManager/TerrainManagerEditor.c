#include "Stdafx.h"
#include "TerrainManager.h"
#include "Terrain/TerrainMap/TerrainMap.h"
#include "Renderer/TerrainRenderer.h"

// Creating Map Part
bool TerrainManager_CreateMap()
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return (false);
	}

	if (terrMgr->editor.szMapName == NULL)
	{
		syserr("You need to enter Map Name");
		return (false);
	}

	if (terrMgr->editor.mapWidth == 0 || terrMgr->editor.mapDepth == 0)
	{
		syserr("You need to enter Map Deminsions (x, z)");
		return (false);
	}

	// Create an empty map
	if (!TerrainMap_CreateMap(terrMgr->editor.szMapName, terrMgr->editor.mapWidth, terrMgr->editor.mapDepth))
	{
		syserr("Failed to Initialize Terrain Map");
		return (false);  // Cleanup everything above ?
	}

	return (true);
}

bool TerrainManager_LoadMap(char* szMapName)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return (false);
	}

	if (terrMgr->pTerrainMap)
	{
		TerrainMap_Clear(terrMgr->pTerrainMap);
	}

	if (!TerrainMap_LoadMap(terrMgr->pTerrainMap, szMapName))
	{
		syserr("Failed to Load Map %s", szMapName);
		return (false);
	}

	if (terrMgr->pTerrainMap == NULL)
	{
		syserr("Failed to Load Map");
		return (false);
	}

	// Initialize Renderer
	if (!TerrainRenderer_Initialize(&terrMgr->terarinRenderer, "Terrain Renderer", terrMgr->pTerrainMap->terrainsXCount, terrMgr->pTerrainMap->terrainsZCount))
	{
		syserr("Failed to Create Terrain Renderer");
		return (false);
	}

	terrMgr->isMapReady = true;
	terrMgr->bNeedsUpdate = true;

	return (true);
}

bool TerrainManager_SaveMap()
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return (false);
	}

	if (!terrMgr)
	{
		/// ????? not sure if this condition is useful .. maybe if the code is part of the game (still need terrain manager to load maps)
		return (false);
	}

	if (terrMgr->isMapReady == false)
	{
		syserr("Map is not Ready, you Need Map Ready to Save");
		return (false);
	}

	return (TerrainMap_SaveMap(terrMgr->pTerrainMap));
}

// Editing Map Part
