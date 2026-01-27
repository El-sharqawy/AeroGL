#include "Terrain.h"
#include "../TerrainMap/TerrainMap.h"
#include "../TerrainPatch.h"
#include "../TerrainData.h"
#include <memory.h>

bool Terrain_Initialize(Terrain* ppTerrain)
{
	*ppTerrain = (Terrain)tracked_calloc(1, sizeof(STerrain));

	Terrain pTerrain = *ppTerrain;
	if (!pTerrain)
	{
		syserr("Failed to Allocate Memory for Terrain");
		return (false);
	}

	Terrain_SetTerrainCoords(pTerrain, -1, -1);

	return (true);
}

void Terrain_Destroy(Terrain* ppTerrain)
{
	if (!ppTerrain || !*ppTerrain)
	{
		return;
	}
	
	Terrain pTerrain = *ppTerrain;

	Vector_Destroy(&pTerrain->terrainPatches);

	FloatGrid_Destroy(&pTerrain->heightMap);

	tracked_free(pTerrain);

	*ppTerrain = NULL;
}

void Terrain_DestroyPtr(Terrain pTerrain)
{
	Terrain* ppTerrain = *(Terrain**)pTerrain;
	if (ppTerrain)
	{
		Terrain_Destroy(ppTerrain);
	}
}

bool Terrain_Load(Terrain pTerrain)
{
	if (pTerrain->terrainXCoord < 0 || pTerrain->terrainZCoord < 0)
	{
		syserr("You need to set Terrain coords")
		return (false);
	}

	if (!Vector_InitCapacity(&pTerrain->terrainPatches, sizeof(TerrainPatch), TERRAIN_PATCH_COUNT))
	{
		syserr("Failed to Initialize Terrain Patch Vector");
		return (false);
	}

	pTerrain->terrainPatches->destructor = TerrainPatch_DestroyPtr;

	memcpy(pTerrain->patchesMetrices, &S_Matrix4_Identity, sizeof(pTerrain->patchesMetrices));

	// Initialize transform to identity
	pTerrain->transform = TransformInit();  // Position (0,0,0), Scale (1,1,1), No rotation

	// Position the mesh in world space
	Vector3 terrainStartPos = Vector3D(TERRAIN_XSIZE * pTerrain->terrainXCoord, 0.0f, pTerrain->terrainZCoord * TERRAIN_ZSIZE);
	TransformSetPositionV(&pTerrain->transform, terrainStartPos);

	// Initialize HeightMap
	if (!FloatGrid_Initialize(&pTerrain->heightMap, HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE))
	{
		Terrain_Destroy(&pTerrain);
		syserr("Failed to Initialize HeightMap");
		return (false);
	}

	for (int32_t iZ = 50; iZ <= 55; iZ++)
	{
		for (int32_t iX = 50; iX <= 55; iX++)
		{
			FloatGrid_SetAt(pTerrain->heightMap, iZ, iX, 10.0f);
		}
	}

	pTerrain->bIsReady = true;

	return (true);
}

void Terrain_SetTerrainCoords(Terrain pTerrain, int32_t iTerrX, int32_t iTerrZ)
{
	if (!pTerrain)
	{
		return;
	}

	pTerrain->terrainXCoord = iTerrX;
	pTerrain->terrainZCoord = iTerrZ;
}

void Terrain_SetTerrainIndex(Terrain pTerrain, int32_t iTerrainIndex)
{
	if (!pTerrain)
	{
		return;
	}

	pTerrain->terrainIndex = iTerrainIndex;
}

bool Terrain_InitializePatches(Terrain pTerrain)
{
	if (!pTerrain)
	{
		return (false);
	}

	if (pTerrain->bIsReady)
	for (int32_t iPatchNumZ = 0; iPatchNumZ < PATCH_ZCOUNT; iPatchNumZ++)
	{
		for (int32_t iPatchNumX = 0; iPatchNumX < PATCH_XCOUNT; iPatchNumX++)
		{
			GLint iPatchNum = iPatchNumZ * PATCH_XCOUNT + iPatchNumX;

			Vector3 worldPos = Vector3D(iPatchNumX * PATCH_XSIZE * PATCH_CELL_SIZE, 0.0f, iPatchNumZ * PATCH_ZSIZE * PATCH_CELL_SIZE);
			TerrainPatch terrainPatch = TerrainPatch_Create(pTerrain, iPatchNum, iPatchNumX, iPatchNumZ, worldPos, PATCH_CELL_SIZE);

			if (terrainPatch == NULL)
			{
				syserr("Failed to Generate Terrain %d Patch %d", pTerrain->terrainIndex, iPatchNum);
				break;
			}

			Matrix4 terrainMat = TransformGetMatrix(&pTerrain->transform);						// parent
			Matrix4 patchMat = TransformGetMatrix(&terrainPatch->terrainMesh->transform);		// local patch
			Matrix4 finalMat = Matrix4_Mul(terrainMat, patchMat);								// parent * local

			pTerrain->patchesMetrices[iPatchNum] = finalMat;
			Vector_PushBack(&pTerrain->terrainPatches, &terrainPatch);
		}
	}

	// Terrain_UpdatePatches(pTerrain);
	return (true);
}

void Terrain_UpdatePatches(Terrain pTerrain)
{
	for (int32_t iPatchNumZ = 0; iPatchNumZ < PATCH_ZCOUNT; iPatchNumZ++)
	{
		for (int32_t iPatchNumX = 0; iPatchNumX < PATCH_XCOUNT; iPatchNumX++)
		{
			Terrain_UpdatePatch(pTerrain, iPatchNumX, iPatchNumZ);
		}
	}
}

void Terrain_UpdatePatch(Terrain pTerrain, int32_t iPatchNumX, int32_t iPatchNumZ)
{
	if (!pTerrain)
	{
		return;
	}

	int32_t iPatchNum = iPatchNumZ * PATCH_XCOUNT + iPatchNumX;

	// ex: 1 * 32 = 32 .. 2 * 32 = 64 .. etc
	int32_t iPatchStartX = iPatchNumX * PATCH_XSIZE;
	int32_t iPatchStartZ = iPatchNumZ * PATCH_ZSIZE;

	float fPatchXSizeMeters = PATCH_XSIZE * PATCH_CELL_SIZE;
	float fPatchZSizeMeters = PATCH_ZSIZE * PATCH_CELL_SIZE;

	float fPatchStartX = (float)(pTerrain->terrainXCoord * XSIZE * PATCH_CELL_SIZE) + (float)(iPatchStartX * PATCH_CELL_SIZE);
	float fPatchStartZ = (float)(pTerrain->terrainZCoord * ZSIZE * PATCH_CELL_SIZE) + (float)(iPatchStartZ * PATCH_CELL_SIZE);

	float fOriginalPatchStartX = fPatchStartX;
	float fOriginalPatchStartZ = fPatchStartZ;


	// Terrain Patch
	TerrainPatch pTerrainPatch = VECTOR_GET(pTerrain->terrainPatches, iPatchNum, TerrainPatch);
	if (pTerrainPatch == NULL)
	{
		syserr("Patch %d not Initialized", iPatchNum);
		return;
	}

	// loop through each vertices
	for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
	{
		fPatchStartX = fOriginalPatchStartX;

		for (GLint iX = iPatchStartX; iX <= iPatchStartX + PATCH_XSIZE; iX++)
		{
			// syslog("iPatchIndex: %d, fX: %f, fZ: %f", iPatchNum, fPatchStartX, fPatchStartZ);
			fPatchStartX += (GLfloat)PATCH_CELL_SIZE;
		}

		fPatchStartZ += (GLfloat)PATCH_CELL_SIZE;
	}
}

void Terrain_SetParentMap(Terrain pTerrain, STerrainMap* pParentMap)
{
	pTerrain->pParentMap = pParentMap;
}

TerrainMap Terrain_GetParentMap(Terrain pTerrain)
{
	return (pTerrain->pParentMap);
}

float GetHeightMapValue(Terrain pTerrain, int32_t x, int32_t z)
{
	// 1. MUST check if pTerrain exists before touching ->heightMap
	if (!pTerrain || !pTerrain->heightMap)
	{
		return 0.0f;
	}

	if (x < 0 || z < 0 || x > HEIGHTMAP_RAW_XSIZE - 1 || z > HEIGHTMAP_RAW_ZSIZE - 1)
	{
		syserr("Error (%d, %d) out of bounds", x, z);
		return (0.0f);
	}

	return FloatGrid_GetAt(pTerrain->heightMap, z, x); // Ignore the border
}

