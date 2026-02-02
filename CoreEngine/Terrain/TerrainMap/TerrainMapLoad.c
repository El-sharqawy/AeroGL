#include "TerrainMap.h"
#include "../../Core/CoreUtils.h"
#include "../../Lib/cJSON.h"
#include <assert.h>

bool TerrainMap_CreateFolder(TerrainMap pTerrainMap, char* szMapName)
{
	if (!szMapName || strlen(szMapName) == 0)
	{
		syserr("Map name cannot be empty.");
		return (false);
	}

	// check Parent Folder
	if (!IsDirectoryExists(terrainMapsFolder))
	{
		syslog("Attemp to Create Parent Maps Folder");
		errno_t result = MKDIR(terrainMapsFolder);
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
	char fullPath[MAX_STRING_LEN] = { 0 };

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullPath, sizeof(fullPath), "%s%s", terrainMapsFolder, szMapName);

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
	char fullPath[MAX_STRING_LEN] = { 0 };

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

	if (cJSON_AddStringToObject(mainObject, "ScriptType", terrainMapScriptType) == NULL)
	{
		syserr("Failed to Allocate Memory for Settings String");
		cJSON_Delete(mainObject);
		return (false);
	}

	if (cJSON_AddNumberToObject(mainObject, "Version", TERRAIN_VERSION_NUMBER) == NULL)
	{
		syserr("Failed to Add Terrain Version");
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

	// Add Map Dir
	if (cJSON_AddStringToObject(mapDataArr, "MapDir", pTerrainMap->szMapDir) == NULL)
	{
		syserr("Failed to Add Map Directory");
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

bool TerrainMap_LoadMap(TerrainMap pTerrainMap, char* szMapName)
{
	if (!IsDirectoryExists(terrainMapsFolder))
	{
		syserr("TerrainMap_LoadMap: Maps Directory doesn't exist!");
		return (false);
	}

	if (!pTerrainMap)
	{
		syserr("Failed to Find Allocated Map");
		return (false);
	}

	if (pTerrainMap->isReady)
	{
		TerrainMap_Clear(pTerrainMap);
		syslog("Cleared Current Map Data")
	}

	//  Create a buffer large enough for the full path
	char fullMapPath[MAX_STRING_LEN] = { 0 };

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullMapPath, sizeof(fullMapPath), "%s%s", terrainMapsFolder, szMapName);

	// Check if the name was truncated
	if (written >= sizeof(fullMapPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	if (!IsDirectoryExists(fullMapPath))
	{
		syserr("TerrainMap_LoadMap: Map Directory doesn't exist!");
		return (false);
	}

	if (!TerrainMap_LoadSettingsFile(pTerrainMap, fullMapPath))
	{
		syserr("TerrainMap_LoadMap: Map Settings doesn't exist!");
		return (false);
	}

	// Setup Terrains Vector with size of given map?
	if (!Vector_InitCapacity(&pTerrainMap->terrains, sizeof(Terrain), pTerrainMap->terrainsXCount * pTerrainMap->terrainsZCount, false))
	{
		syserr("Failed to Initialize Terrain Map Vector");
		return (false);
	}

	pTerrainMap->terrains->destructor = Terrain_Destroy;

	// Create Terrains Among X-Z Axis
	for (int32_t iTerrZ = 0; iTerrZ < pTerrainMap->terrainsZCount; iTerrZ++)
	{
		for (int32_t iTerrX = 0; iTerrX < pTerrainMap->terrainsXCount; iTerrX++)
		{
			if (!TerrainMap_LoadTerrain(pTerrainMap, iTerrX, iTerrZ))
			{
				syserr("Failed to Load Terrain At (%d, %d)", iTerrX, iTerrZ);
				return (false);
			}
		}
	}

	pTerrainMap->isReady = true;
	syslog("Loaded Map %s Size %dx%d", pTerrainMap->szMapName, pTerrainMap->terrainsXCount, pTerrainMap->terrainsZCount);
	return (true);
}

bool TerrainMap_LoadSettingsFile(TerrainMap pTerrainMap, const char* szFullPath)
{
	if (!IsDirectoryExists(szFullPath))
	{
		syserr("TerrainMap_LoadSettingsFile: Map Directory %s doesn't exist!", szFullPath);
		return (false);
	}

	//  Create a buffer large enough for the full path
	char fullSettingsPath[MAX_STRING_LEN] = { 0 };

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullSettingsPath, sizeof(fullSettingsPath), "%s/%s", szFullPath, "AnubisMap.json");
	// Check if the name was truncated
	if (written >= sizeof(fullSettingsPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	// Parse JSON File
	syslog("Loading Settings File: %s", fullSettingsPath);
	FILE* fSettingsFile = fopen(fullSettingsPath, "rb");
	if (fSettingsFile == NULL)
	{
		syserr("Failed to Open Settings File %s", fullSettingsPath);
		return (false);
	}

	// Moving pointer to end
	if (fseek(fSettingsFile, 0, SEEK_END) != 0)
	{
		syserr("Failed to Seek the end of settings file");
		fclose(fSettingsFile);
		return (false);
	}

	long len = ftell(fSettingsFile); // get the file buffer length, starting from last character in the file
	if (len == -1L)
	{
		syserr("Failed to Get settings file length");
		fclose(fSettingsFile);
		return (false);
	}

	if (fseek(fSettingsFile, 0, SEEK_SET) != 0)
	{
		syserr("Failed to Seek the start of settings file");
		fclose(fSettingsFile);
		return (false);
	}


	char* settingsFileData = tracked_malloc(len + 1);
	if (fread(settingsFileData, 1, len, fSettingsFile) != len)
	{
		syserr("Failed to Read the settings file");
		tracked_free(settingsFileData);
		fclose(fSettingsFile);
		return (false);
	}

	fclose(fSettingsFile);
	settingsFileData[len] = '\0'; // Add NULL terminator

	cJSON* settingsJson = cJSON_Parse(settingsFileData);
	tracked_free(settingsFileData); // Free early since cJSON makes its own internal copy

	if (settingsJson == NULL)
	{
		const char* error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			syserr("Error Parsing Settings File Error: %s", error_ptr);
		}
		return (false);
	}

	// Load Script Type
	const cJSON* scriptType = cJSON_GetObjectItemCaseSensitive(settingsJson, "ScriptType");
	if (!cJSON_IsString(scriptType) || (scriptType->valuestring == NULL)) 
	{
		syserr("Error Parsing Settings File Error: Failed to Load Script Type");
		cJSON_Delete(settingsJson);
		return (false);
	}

	if (_stricmp(scriptType->valuestring, terrainMapScriptType) != 0)
	{
		syserr("Error Parsing Settings File Error: Invalid Map Script Type");
		cJSON_Delete(settingsJson);
		return (false);
	}

	// Load Version Type
	const cJSON* terrainVersion = cJSON_GetObjectItemCaseSensitive(settingsJson, "Version");
	if (!cJSON_IsNumber(terrainVersion) || (terrainVersion->valueint == 0))
	{
		syserr("Error Parsing Settings File Error: Failed to Terrain Version");
		cJSON_Delete(settingsJson);
		return (false);
	}

	if (terrainVersion->valueint != TERRAIN_VERSION_NUMBER)
	{
		syserr("Error Parsing Settings File Error: Invalid Script Version");
		cJSON_Delete(settingsJson);
		return (false);
	}

	// Load MapData
	const cJSON* mapData = cJSON_GetObjectItemCaseSensitive(settingsJson, "MapData");
	if (!cJSON_IsObject(mapData))
	{
		syserr("Error Parsing Settings File Error: Failed to Load Map Data");
		cJSON_Delete(settingsJson);
		return (false);
	}

	// Load Map Size
	const cJSON* mapSize = cJSON_GetObjectItemCaseSensitive(mapData, "MapSize");
	if (!cJSON_IsObject(mapSize))
	{
		syserr("Error Parsing Settings File Error: Failed to Load Map Size");
		cJSON_Delete(settingsJson);
		return (false);
	}

	const cJSON* mapSizeX = cJSON_GetObjectItemCaseSensitive(mapSize, "x");
	if (!cJSON_IsNumber(mapSizeX) || (mapSizeX->valueint == 0))
	{
		syserr("Error Parsing Settings File Error: Failed to Load Map Width");
		cJSON_Delete(settingsJson);
		return (false);
	}

	const cJSON* mapSizeZ = cJSON_GetObjectItemCaseSensitive(mapSize, "z");
	if (!cJSON_IsNumber(mapSizeZ) || (mapSizeZ->valueint == 0))
	{
		syserr("Error Parsing Settings File Error: Failed to Load Map Depth");
		cJSON_Delete(settingsJson);
		return (false);
	}

	TerrainMap_SetDeminsions(pTerrainMap, mapSizeX->valueint, mapSizeZ->valueint);

	// Load Map Name
	const cJSON* mapName = cJSON_GetObjectItemCaseSensitive(mapData, "MapName");
	if (!cJSON_IsString(mapName) || (mapName->valuestring == NULL))
	{
		syserr("Error Parsing Settings File Error: Failed to Map Name");
		cJSON_Delete(settingsJson);
		return (false);
	}

	TerrainMap_SetMapName(pTerrainMap, mapName->valuestring);

	const cJSON* mapPath = cJSON_GetObjectItemCaseSensitive(mapData, "MapDir");
	if (!cJSON_IsString(mapPath) || (mapPath->valuestring == NULL))
	{
		syserr("Error Parsing Settings File Error: Failed to Map Path");
		cJSON_Delete(settingsJson);
		return (false);
	}

	TerrainMap_SetMapDir(pTerrainMap, mapPath->valuestring);

	cJSON_Delete(settingsJson);
	return (true);
}

bool TerrainMap_LoadTerrain(TerrainMap pTerrainMap, int32_t iTerrainX, int32_t iTerrainZ)
{
	if (TerrainMap_IsTerrainLoaded(pTerrainMap, iTerrainX, iTerrainZ))
	{
		return (true);
	}

	Terrain pTerrain = NULL;
	int32_t iTerrainIndex = iTerrainZ * 1000 + iTerrainX;

	if (!Terrain_Initialize(&pTerrain))
	{
		syserr("Failed to Create Terrain at coord (%d, %d)", iTerrainX, iTerrainZ);
		return (false);
	}

	Terrain_SetParentMap(pTerrain, pTerrainMap);
	Terrain_SetTerrainCoords(pTerrain, iTerrainX, iTerrainZ);
	Terrain_SetTerrainIndex(pTerrain, iTerrainIndex);

	// Initialize Terrain Vectors
	if (!Terrain_Load(pTerrain))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Failed to Load Terrrain");
		return (false);
	}

	//  Create a buffer large enough for the full path
	char fullTerrainPath[MAX_STRING_LEN] = { 0 };

	// Use snprintf to combine the path and name safely
	int32_t written = snprintf(fullTerrainPath, sizeof(fullTerrainPath), "%s/%06d", pTerrainMap->szMapDir, iTerrainIndex);
	// Check if the name was truncated
	if (written >= sizeof(fullTerrainPath))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Path name is too long.");
		return false;
	}

	// Load HeightMap
	if (!Terrain_LoadHeightMap(pTerrain, fullTerrainPath))
	{
		Terrain_Destroy(&pTerrain);
		return (false);
	}
	
	// Initialize Terrain HeightMap Tex
	if (!Terrain_LoadHeightMapTexture(pTerrain))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Failed to Initialize Terrain HeightMap Texture");
		return (false);
	}

	// Initialize Patches after HeightMap
	if (!Terrain_InitializePatches(pTerrain))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Failed to Initialize Terrain Patches");
		return (false);
	}

	pTerrain->bIsReady = true;

	Vector_PushBack(pTerrainMap->terrains, pTerrain);
	return (true);
}

bool TerrainMap_IsTerrainLoaded(TerrainMap pTerrainMap, int32_t iTerrainX, int32_t iTerrainZ)
{
	if (pTerrainMap == NULL)
	{
		syserr("TerrainMap is NULL (did you forget to allocate memory?)");
		return (false);
	}

	if (pTerrainMap->terrains == NULL)
	{
		syserr("TerrainMap Terrains Vector is NULL (did you forget to set it up?)");
		return (false);
	}

	for (int32_t i = 0; i < pTerrainMap->terrains->count; i++)
	{
		Terrain pSearchTerrain = Vector_GetPtr(pTerrainMap->terrains, i);

		if (pSearchTerrain->terrainXCoord == iTerrainX && pSearchTerrain->terrainZCoord == iTerrainZ)
		{
			return (true);
		}
	}

	return (false);
}

bool TerrainMap_SaveMap(TerrainMap pTerrainMap)
{
	if (!pTerrainMap)
	{
		return (false); // ??
	}

	if (pTerrainMap->szMapDir == NULL)
	{
		syserr("TerrainMap_SaveMap: Map Directory is NULL!");
		return (false);
	}

	if (!IsDirectoryExists(pTerrainMap->szMapDir))
	{
		syserr("TerrainMap_SaveMap: Map Directory doesn't exist!");
		return (false);
	}

	if (!TerrainMap_SaveSettingsFile(pTerrainMap))
	{
		syserr("Failed to Save Map %s Settings File", pTerrainMap->szMapName);
		return (false);
	}

	// Create Terrains Among X-Z Axis
	for (int32_t iTerrZ = 0; iTerrZ < pTerrainMap->terrainsZCount; iTerrZ++)
	{
		for (int32_t iTerrX = 0; iTerrX < pTerrainMap->terrainsXCount; iTerrX++)
		{
			if (!TerrainMap_SaveTerrain(pTerrainMap, iTerrX, iTerrZ))
			{
				syserr("Failed to Save Terrain At (%d, %d)", iTerrX, iTerrZ);
				return (false);
			}
		}
	}

	syslog("Saved Map %s Size %dx%d", pTerrainMap->szMapName, pTerrainMap->terrainsXCount, pTerrainMap->terrainsZCount);
	return (true);
}

bool TerrainMap_SaveSettingsFile(TerrainMap pTerrainMap)
{
	if (!pTerrainMap->isReady)
	{
		syserr("Map is not Ready");
		return (false);
	}

	//  Create a buffer large enough for the full path
	char fullPath[MAX_STRING_LEN] = { 0 };

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

	// Write Script Type
	if (cJSON_AddStringToObject(mainObject, "ScriptType", terrainMapScriptType) == NULL)
	{
		syserr("Failed to Allocate Memory for Settings String");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Version
	if (cJSON_AddNumberToObject(mainObject, "Version", TERRAIN_VERSION_NUMBER) == NULL)
	{
		syserr("Failed to Add Terrain Version");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Data
	cJSON* mapDataArr = cJSON_AddObjectToObject(mainObject, "MapData");
	if (mapDataArr == NULL)
	{
		syserr("Failed to Allocate Memory for Map Settings Object");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Size
	cJSON* mapSizeArr = cJSON_AddObjectToObject(mapDataArr, "MapSize");
	if (mapSizeArr == NULL)
	{
		syserr("Failed to Allocate Memory for Map Size Object");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Size Width
	if (cJSON_AddNumberToObject(mapSizeArr, "x", pTerrainMap->terrainsXCount) == NULL)
	{
		syserr("Failed to Add Map Size X");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Size Depth
	if (cJSON_AddNumberToObject(mapSizeArr, "z", pTerrainMap->terrainsZCount) == NULL)
	{
		syserr("Failed to Add Map Size Z");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Name
	if (cJSON_AddStringToObject(mapDataArr, "MapName", pTerrainMap->szMapName) == NULL)
	{
		syserr("Failed to Add Map Name");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Map Dir
	if (cJSON_AddStringToObject(mapDataArr, "MapDir", pTerrainMap->szMapDir) == NULL)
	{
		syserr("Failed to Add Map Directory");
		cJSON_Delete(mainObject);
		return (false);
	}

	// Write Into JSON File
	char* string = cJSON_Print(mainObject);
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

bool TerrainMap_SaveTerrain(TerrainMap pTerrainMap, int32_t iTerrainX, int32_t iTerrainZ)
{
	if (TerrainMap_IsTerrainLoaded(pTerrainMap, iTerrainX, iTerrainZ) == false)
	{
		syserr("Terrain is not loaded!");
		return (false);
	}

	Terrain pTerrain = Vector_GetPtr(pTerrainMap->terrains, iTerrainZ * pTerrainMap->terrainsXCount + iTerrainX);

	if (pTerrain == NULL)
	{
		syserr("Failed to Find Terrain at coord (%d, %d)", iTerrainX, iTerrainZ);
		return (false);
	}


	//  Create a buffer large enough for the full path
	char fullTerrainPath[MAX_STRING_LEN] = { 0 };
	
	// Use snprintf to combine the path and name safely
	int32_t iTerrainIndex = iTerrainZ * 1000 + iTerrainX;
	int32_t written = snprintf(fullTerrainPath, sizeof(fullTerrainPath), "%s/%06d", pTerrainMap->szMapDir, iTerrainIndex);
	// Check if the name was truncated
	if (written >= sizeof(fullTerrainPath))
	{
		syserr("Path name is too long.");
		return false;
	}

	// Load HeightMap
	if (!Terrain_SaveHeightMap(pTerrain, fullTerrainPath))
	{
		syserr("Failed to Save HeightMap");
		return (false);
	}

	return (true);
}