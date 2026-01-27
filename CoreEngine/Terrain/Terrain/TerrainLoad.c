#include "Terrain.h"
#include "../TerrainMap/TerrainMap.h"
#include "../../Core/CoreUtils.h"

bool Terrain_CreateFiles(STerrainMap* pParentMap, int32_t iTerrainX, int32_t iTerrainZ)
{
	Terrain pTerrain = NULL;
	int32_t iTerrainIndex = iTerrainZ * 1000 + iTerrainX;

	if (!Terrain_Initialize(&pTerrain))
	{
		syserr("Failed to Create Terrain at coord (%d, %d)", iTerrainX, iTerrainZ);
		Terrain_Destroy(&pTerrain);
		return (false);
	}

	Terrain_SetTerrainCoords(pTerrain, iTerrainX, iTerrainZ);
	Terrain_SetTerrainIndex(pTerrain, iTerrainIndex);

	char szTerrainPath[MAX_STRING_LEN] = { 0 };
	int32_t written = snprintf(szTerrainPath, sizeof(szTerrainPath), "%s/%06d", pParentMap->szMapDir, iTerrainIndex);

	// Check if the name was truncated
	if (written >= sizeof(szTerrainPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	syslog("TerrainFolder: %s", szTerrainPath);

	if (!MakeDirectory(szTerrainPath))
	{
		syserr("Failed to Create Directory: %s", szTerrainPath);
		return (false);
	}

	// Create Terrain Properties (Objects, objs Pos, etc)
	if (!Terrain_CreateHeightMap(pTerrain, szTerrainPath))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Failed to Create HeightMap for Terrain (%d, %d)", iTerrainX, iTerrainZ);
		return (false);
	}

	Terrain_Destroy(&pTerrain);
	return (true);
}

bool Terrain_CreateHeightMap(Terrain pTerrain, const char* szTerrainsFolder)
{
	char szHeightMapFile[MAX_STRING_LEN] = { 0 };

	int32_t written = snprintf(szHeightMapFile, sizeof(szHeightMapFile), "%s/HeightMap.raw", szTerrainsFolder);

	// Check if the name was truncated
	if (written >= sizeof(szHeightMapFile))
	{
		syserr("Path name is too long.");
		return false;
	}

	FloatGrid pHeightMap = NULL;
	if (!FloatGrid_Initialize(&pHeightMap, HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE)) // Init with ZEROs
	{
		syserr("Failed to Allocate HeightMap");
		return (false);
	}

	FILE* fHeightMap = fopen(szHeightMapFile, "wb");
	if (fHeightMap == NULL)
	{
		FloatGrid_Destroy(&pHeightMap);
		syserr("Error opening heightmap file %s", szHeightMapFile);
		return (false);
	}

	bool success = false;

	// Write a "Magic Number" to identify the file type (Optional but professional)
	const uint32_t MAGIC_NUMBER = 0x47726964; // Grid in Hex
	if (fwrite(&MAGIC_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Magic Number");
	}
	else if (fwrite(&pHeightMap->cols, sizeof(pHeightMap->cols), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Columns Number");
	}
	else if (fwrite(&pHeightMap->rows, sizeof(pHeightMap->rows), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Rows Number");
	}
	else if (fwrite(pHeightMap->pArray, FloatGrid_GetBytesSize(pHeightMap), 1, fHeightMap) != 1)
	{
		syserr("Failed to write HeightMap Data");
	}
	else
	{
		success = true;
	}

	FloatGrid_Destroy(&pHeightMap);
	fclose(fHeightMap);
	return (success);
}
