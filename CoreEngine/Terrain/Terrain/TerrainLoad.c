#include "Terrain.h"
#include "../TerrainMap/TerrainMap.h"
#include "../../Core/CoreUtils.h"
#include "../../Math/Grids/FloatGrid.h"

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

	// Safety for overwriting

	if (access(szHeightMapFile, 0) != -1)
	{
		syserr("Aborting Create: A heightmap already exists at %s", szHeightMapFile);
		// return false; // Don't overwrite without asking!
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
	const uint32_t MAGIC_NUMBER = TERRAIN_MAGIC_NUMBER; // Grid in Hex
	const uint32_t VERSION_NUMBER = TERRAIN_VERSION_NUMBER; // Terrain Version number (1)

	if (fwrite(&MAGIC_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Magic Number");
	}
	else if (fwrite(&VERSION_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Version Number");
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

bool Terrain_LoadHeightMap(Terrain pTerrain, const char* szTerrainsFolder)
{
	char szHeightMapFile[MAX_STRING_LEN] = { 0 };
	int32_t written = snprintf(szHeightMapFile, sizeof(szHeightMapFile), "%s/HeightMap.raw", szTerrainsFolder);

	// Check if the name was truncated
	if (written >= sizeof(szHeightMapFile))
	{
		syserr("Path name is too long.");
		return (false);
	}

	// read our file
	FILE* fHeightMap = fopen(szHeightMapFile, "rb");
	if (fHeightMap == NULL)
	{
		syserr("Error loading heightmap file %s", szHeightMapFile);
		return (false);
	}

	// 1. Check Magic Number
	uint32_t MAGIC_NUMBER = 0;
	if (fread(&MAGIC_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to read Magic Number");
		fclose(fHeightMap);
		return (false);
	}

	if (MAGIC_NUMBER != TERRAIN_MAGIC_NUMBER)
	{
		syserr("Invalid file format: %s", szHeightMapFile);
		fclose(fHeightMap);
		return (false);
	}

	// 2. Check Version Number
	uint32_t VERSION_NUMBER = 0;
	if (fread(&VERSION_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to read Version Number");
		fclose(fHeightMap);
		return (false);
	}

	if (VERSION_NUMBER != TERRAIN_VERSION_NUMBER)
	{
		syserr("Invalid file version: %s", szHeightMapFile);
		fclose(fHeightMap);
		return (false);
	}

	if (pTerrain->heightMap)
	{
		FloatGrid_Destroy(&pTerrain->heightMap);
	}

	// Create our grid
	if (!FloatGrid_Initialize(&pTerrain->heightMap, HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE))
	{
		syserr("Failed to Initialize Height map");
		fclose(fHeightMap);
		return (false);
	}

	int32_t fCols = 0, fRows = 0;
	bool success = false;

	// Read metadata (Columns and Rows)
	if (fread(&fCols, sizeof(int32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to read Columns Number");
	}
	else if (fread(&fRows, sizeof(int32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to read Rows Number");
	}
	else if (fread(pTerrain->heightMap->pArray, FloatGrid_GetBytesSize(pTerrain->heightMap), 1, fHeightMap) != 1)
	{
		syserr("Failed to read HeightMap Data");
	}
	else
	{
		//  Verify if file dimensions match engine dimensions
		if (fCols != pTerrain->heightMap->cols || fRows != pTerrain->heightMap->rows)
		{
			syserr("Warning: File dimensions (%dx%d) mismatch engine (%dx%d)", fCols, fRows, pTerrain->heightMap->cols, pTerrain->heightMap->rows);
		}

		success = true;
	}

	// Validate the data
	if (success)
	{
		for (size_t i = 0; i < pTerrain->heightMap->size; ++i)
		{
			float h = pTerrain->heightMap->pArray[i];
			// isfinite checks if the number is not NaN and not Infinity
			if (!isfinite(h))
			{
				pTerrain->heightMap->pArray[i] = 0.0f; // Reset bad data to 0
			}
		}
	}

	fclose(fHeightMap);
	return (success);
}

bool Terrain_SaveHeightMap(Terrain pTerrain, const char* szTerrainsFolder)
{
	if (pTerrain->heightMap == NULL)
	{
		return Terrain_CreateHeightMap(pTerrain, szTerrainsFolder);
	}

	char szHeightMapFile[MAX_STRING_LEN] = { 0 };
	char szHeightMapFileBackUP[MAX_STRING_LEN] = { 0 };
	int32_t written = snprintf(szHeightMapFileBackUP, sizeof(szHeightMapFileBackUP), "%s/HeightMap.raw.bak", szTerrainsFolder); // write to external file, in case we interrupt 

	// Check if the name was truncated, BackUP Method
	if (written >= sizeof(szHeightMapFileBackUP))
	{
		syserr("Path name is too long.");
		return false;
	}

	// Original File
	written = snprintf(szHeightMapFile, sizeof(szHeightMapFile), "%s/HeightMap.raw", szTerrainsFolder);
	if (written >= sizeof(szHeightMapFile))
	{
		syserr("Path name is too long.");
		return false;
	}

	FILE* fHeightMap = fopen(szHeightMapFileBackUP, "wb");
	if (fHeightMap == NULL)
	{
		syserr("Error opening heightmap file %s", szHeightMapFileBackUP);
		return (false);
	}

	bool success = false;

	// Write a "Magic Number" to identify the file type (Optional but professional)
	const uint32_t MAGIC_NUMBER = TERRAIN_MAGIC_NUMBER; // Grid in Hex
	const uint32_t VERSION_NUMBER = TERRAIN_VERSION_NUMBER; // Terrain Version number (1)

	if (fwrite(&MAGIC_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Magic Number");
	}
	else if (fwrite(&VERSION_NUMBER, sizeof(uint32_t), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Version Number");
	}
	else if (fwrite(&pTerrain->heightMap->cols, sizeof(pTerrain->heightMap->cols), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Columns Number");
	}
	else if (fwrite(&pTerrain->heightMap->rows, sizeof(pTerrain->heightMap->rows), 1, fHeightMap) != 1)
	{
		syserr("Failed to write Rows Number");
	}
	else if (fwrite(pTerrain->heightMap->pArray, FloatGrid_GetBytesSize(pTerrain->heightMap), 1, fHeightMap) != 1)
	{
		syserr("Failed to write HeightMap Data");
	}
	else
	{
		success = true;
	}

	fclose(fHeightMap);

	if (success)
	{
		// 1. Delete the old existing HeightMap.raw so we can replace it
		// (On some OS/Filesystems, rename fails if the target already exists)
		if (remove(szHeightMapFile) != 0)
		{
			// Do Nothing for now
		}

		if (rename(szHeightMapFileBackUP, szHeightMapFile) != 0)
		{
			syserr("Failed to promote backup to original heightmap file.");
			success = false; // If rename fails, the save wasn't fully successful
		}
	}
	else
	{
		// If the write failed, delete the partial/corrupt backup file
		if (remove(szHeightMapFileBackUP) != 0)
		{
			// do nothing for now
		}
	}

	return success;
}
