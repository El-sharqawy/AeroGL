#include "Stdafx.h"
#include "TerrainManager.h"

// Manager Editor Map Accessors
void TerrainManager_SetMapName(const char* szMapName)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.szMapName = engine_strdup(szMapName, MEM_TAG_STRINGS);
}

void TerrainManager_SetMapDeminsions(int32_t mapWidth, int32_t mapDepth)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	// Number of Terrains Among X-Z Axis
	terrMgr->editor.mapWidth = mapWidth;
	terrMgr->editor.mapDepth = mapDepth;
}

// Manager Editor Brush Accessors
void TerrainManager_SetBrushType(GLint brushType)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushType = brushType;
}

void TerrainManager_SetBrushShape(GLint brushShape)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushShape = brushShape;
}

void TerrainManager_SetBrushStrength(GLint brushStrength)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushStrength = brushStrength;
}

void TerrainManager_SetBrushMaxStrength(GLint brushMaxStrength)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushMaxStrength = brushMaxStrength;
}

void TerrainManager_SetBrushSize(GLint brushSize)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushSize = brushSize;
}

void TerrainManager_SetBrushMaxSize(GLint brushMaxSize)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.brushMaxSize = brushMaxSize;
}

// Manager Editor Edit Accessors
void TerrainManager_SetEditX(GLint editX)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editX = editX;
}

void TerrainManager_SetEditZ(GLint editZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editZ = editZ;
}

void TerrainManager_SetEditXZ(GLint editX, GLint editZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editX = editX;
	terrMgr->editor.editZ = editZ;
}

void TerrainManager_SetSubCellX(GLint subCellX)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.subCellX = subCellX;
}

void TerrainManager_SetSubCellZ(GLint subCellZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.subCellZ = subCellZ;
}

void TerrainManager_SetSubCellXZ(GLint subCellX, GLint subCellZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.subCellX = subCellX;
	terrMgr->editor.subCellZ = subCellZ;
}

void TerrainManager_SetEditTerrainNumX(GLint editTerrainNumX)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editTerrainNumX = editTerrainNumX;
}

void TerrainManager_SetEditTerrainNumZ(GLint editTerrainNumZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editTerrainNumZ = editTerrainNumZ;
}

void TerrainManager_SetEditTerrainNumXZ(GLint editTerrainNumX, GLint editTerrainNumZ)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.editTerrainNumX = editTerrainNumX;
	terrMgr->editor.editTerrainNumZ = editTerrainNumZ;
}

void TerrainManager_SetPickingPoint(Vector3 v3PickingPoint)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.v3PickingPoint = v3PickingPoint;
}

void TerrainManager_SetIsEditingTerrain(bool isEditingTerrain)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.isEditingTerrain = isEditingTerrain;
}

void TerrainManager_SetIsEditingHeight(bool isEditingHeight)
{
	TerrainManager terrMgr = GetTerrainManager();
	if (!terrMgr)
	{
		return;
	}

	terrMgr->editor.isEditingHeight = isEditingHeight;
}
