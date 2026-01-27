#include "TerrainMap.h"
#include "../../Core/CoreUtils.h"
#include "../../Lib/cJSON.h"

bool TerrainMap_CreateFolder(TerrainMap pTerrainMap, char* szMapName)
{
	if (!szMapName || strlen(szMapName) == 0)
	{
		syserr("Map name cannot be empty.");
		return (false);
	}

	// check Parent Folder
	if (!IsDirectoryExists(MAP_ASSET_PATH))
	{
		syslog("Attemp to Create Parent Maps Folder");
		errno_t result = MKDIR(MAP_ASSET_PATH);
		if (result != 0)
		{
			char buffer[100];
			// Use strerror to get a human-readable message from the error code
			if (strerror_s(buffer, sizeof(buffer), result) == 0)
			{
				syserr("Error Creating Map Folder: %s (Code: %d)", buffer, result);
			}
			else
			{
				syserr("Failed to retrieve error message (Code: %d)", result);
			}
			return (false);
		}
	}

	//  Create a buffer large enough for the full path
	char fullPath[MAX_STRING_LEN];

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullPath, sizeof(fullPath), "%s%s", MAP_ASSET_PATH, szMapName);

	// Check if the name was truncated
	if (written >= sizeof(fullPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	if (!MakeDirectory(fullPath))
	{
		syserr("Failed to Create Directory: %s", fullPath);
		return (false);
	}

	TerrainMap_SetMapName(pTerrainMap, szMapName);
	TerrainMap_SetMapDir(pTerrainMap, fullPath);

	return (true);
}

bool TerrainMap_CreateSettingsFile(TerrainMap pTerrainMap)
{
	if (pTerrainMap->szMapDir == NULL)
	{
		syserr("TerrainMap_CreateSettingsFile: Map Directory is NULL!");
		return (false);
	}

	if (!IsDirectoryExists(pTerrainMap->szMapDir))
	{
		syserr("TerrainMap_CreateSettingsFile: Map Directory doesn't exist!");
		return (false);
	}

	//  Create a buffer large enough for the full path
	char fullPath[MAX_STRING_LEN];

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullPath, sizeof(fullPath), "%s/%s", pTerrainMap->szMapDir, "AnubisMap.json");
	// Check if the name was truncated
	if (written >= sizeof(fullPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	// Create 
	cJSON* mainObject = cJSON_CreateObject();
	if (mainObject == NULL)
	{
		return (false);
	}

	if (cJSON_AddStringToObject(mainObject, "ScriptType", "AnubisMapSettings") == NULL)
	{
		syserr("Failed to Allocate Memory for Settings String");
		cJSON_Delete(mainObject);
		return (false);
	}

	cJSON* mapDataArr = cJSON_AddObjectToObject(mainObject, "MapData");
	if (mapDataArr == NULL)
	{
		syserr("Failed to Allocate Memory for Map Settings Object");
		cJSON_Delete(mainObject);
		return (false);
	}

	cJSON* mapSizeArr = cJSON_AddObjectToObject(mapDataArr, "MapSize");
	if (mapSizeArr == NULL)
	{
		syserr("Failed to Allocate Memory for Map Size Object");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Add Map Size
	if (cJSON_AddNumberToObject(mapSizeArr, "x", pTerrainMap->terrainsXCount) == NULL)
	{
		syserr("Failed to Add Map Size X");
		cJSON_Delete(mainObject);
		return (false);
	}
	if (cJSON_AddNumberToObject(mapSizeArr, "z", pTerrainMap->terrainsZCount) == NULL)
	{
		syserr("Failed to Add Map Size Z");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Add Map Name
	if (cJSON_AddStringToObject(mapDataArr, "MapName", pTerrainMap->szMapName) == NULL)
	{
		syserr("Failed to Add Map Name");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Into JSON File
	char *string = cJSON_Print(mainObject);
	if (string == NULL)
	{
		syserr("Failed to Allocate Memory for Settings Buffer");
		cJSON_Delete(mainObject);
		return (false);
	}

	FILE* pSettings = fopen(fullPath, "w");
	if (pSettings == NULL)
	{
		syserr("Error opening Settings File %s (Check Path if valid)", fullPath);
		cJSON_Delete(mainObject);
		free(string);
		return (false);
	}

	// Print contents and free string
	fprintf(pSettings, string);
	fclose(pSettings);

	cJSON_Delete(mainObject);
	free(string);

	return (true);
}

bool TerrainMap_CreateMap(char* szMapName, int32_t terrainsX, int32_t terrainsZ)
{
	// Create New Map for creation only
	TerrainMap pNewMap = NULL;
	if (!TerrainMap_Initialize(&pNewMap))
	{
		return (false);
	}

	if (!TerrainMap_CreateFolder(pNewMap, szMapName))
	{
		TerrainMap_Destroy(&pNewMap);
		syserr("Failed to Create Map Folder");
		return (false);
	}

	// Setup Map Deminsions
	TerrainMap_SetDeminsions(pNewMap, terrainsX, terrainsZ);

	// Create Settings File
	if (!TerrainMap_CreateSettingsFile(pNewMap))
	{
		TerrainMap_Destroy(&pNewMap);
		syserr("Failed to Create Settings File")
		return (false);
	}

	// Create Terrains Among X-Z Axis
	for (int32_t iTerrZ = 0; iTerrZ < pNewMap->terrainsZCount; iTerrZ++)
	{
		for (int32_t iTerrX = 0; iTerrX < pNewMap->terrainsXCount; iTerrX++)
		{
			if (!Terrain_CreateFiles(pNewMap, iTerrX, iTerrZ))
			{
				TerrainMap_Destroy(&pNewMap);
				syserr("Failed to Create Terrain Files At coord (%d, %d) for Dir %s", iTerrX, iTerrZ, pNewMap->szMapDir);
				return (false);
			}
		}
	}

	// Free Memory
	TerrainMap_Destroy(&pNewMap);
	return (true);
}