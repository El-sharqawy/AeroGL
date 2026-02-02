#include "Terrain.h"
#include "../TerrainMap/TerrainMap.h"
#include "../TerrainData.h"
#include "../TerrainPatch.h"
#include "../../Math/Grids/FloatGrid.h"
#include "../../PipeLine/Texture.h"
#include "../../Math/Matrix/Matrix3.h"

#include <memory.h>

bool Terrain_Initialize(Terrain* ppTerrain)
{
	if (ppTerrain == NULL)
	{
		syserr("ppTerrain is NULL (invalid address)");
		return false;
	}

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

	Texture_Destroy(&pTerrain->pHeightMapTexture);

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
		syserr("No Terrain Allocated")
		return (false);
	}

	for (int32_t iPatchNumZ = 0; iPatchNumZ < PATCH_ZCOUNT; iPatchNumZ++)
	{
		for (int32_t iPatchNumX = 0; iPatchNumX < PATCH_XCOUNT; iPatchNumX++)
		{
			GLint iPatchNum = iPatchNumZ * PATCH_XCOUNT + iPatchNumX;
			Vector4 color;
			color.r = (float)(iPatchNum % 8) / 8.0f;      // Red: 0,0.125,0.25...
			color.g = (float)(iPatchNum / 8 % 8) / 8.0f;  // Green: cycles every 8
			color.b = 0.5f + 0.5f * sinf(iPatchNum * 0.3f);  // Blue: rainbow
			color.a = 1.0f;

			TerrainPatch pTerrainPatch = NULL;
			if (!TerrainPatch_Initialize(&pTerrainPatch, pTerrain->pParentMap, iPatchNum))
			{
				syserr("Failed to Initialize Patch %d", iPatchNum)
				return (false);
			}

			// ex: 1 * 32 = 32 .. 2 * 32 = 64 .. etc
			int32_t iPatchStartX = iPatchNumX * PATCH_XSIZE;
			int32_t iPatchStartZ = iPatchNumZ * PATCH_ZSIZE;

			float fPatchXSizeMeters = PATCH_XSIZE * PATCH_CELL_SIZE;
			float fPatchZSizeMeters = PATCH_ZSIZE * PATCH_CELL_SIZE;

			float fPatchStartX = (float)(pTerrain->terrainXCoord * XSIZE * PATCH_CELL_SIZE) + (float)(iPatchStartX * PATCH_CELL_SIZE);
			float fPatchStartZ = (float)(pTerrain->terrainZCoord * ZSIZE * PATCH_CELL_SIZE) + (float)(iPatchStartZ * PATCH_CELL_SIZE);

			float fOriginalPatchStartX = fPatchStartX;
			float fOriginalPatchStartZ = fPatchStartZ;

			TerrainMesh mesh = pTerrainPatch->terrainMesh;
			int32_t width = pTerrainPatch->patchWidth;
			int32_t depth = pTerrainPatch->patchDepth;

			Vector_Reserve(mesh->pVertices, (width + 1) * (depth + 1));
			Vector_Reserve(mesh->pIndices, (width * depth * 6));

			// Normals Calculations
			Matrix4 model = TransformGetMatrix(&pTerrainPatch->terrainMesh->transform);
			Matrix3 mat3Model = Matrix3_InitMatrix4(model);
			Matrix3 invModel = Matrix3_Inverse(mat3Model);
			Matrix3 normalMatrix = Matrix3_TransposeN(invModel);

			// loop through each vertices
			for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
			{
				fPatchStartX = fOriginalPatchStartX;

				for (GLint iX = iPatchStartX; iX <= iPatchStartX + PATCH_XSIZE; iX++)
				{
					STerrainVertex vertex = { 0 };

					vertex.v3Position = Vector3D(fPatchStartX, 0.0f, fPatchStartZ);
					vertex.v2TexCoords = Vector2D((fPatchStartX - fOriginalPatchStartX) / fPatchXSizeMeters, (fPatchStartZ - fOriginalPatchStartZ) / fPatchZSizeMeters);
					vertex.v3Normals = Vector3D(0.0f, 1.0f, 0.0f);
					vertex.v4Color = color;

					TerrainMesh_AddVertex(mesh, vertex);

					fPatchStartX++;
				}

				fPatchStartZ++;
			}

			syslog("Initialized %zu vertices", pTerrainPatch->terrainMesh->pVertices->count);
			TerrainPatch_InitializeIndices(pTerrainPatch);
			
			// Since we are baking the worldPos into the vertices, 
			// the matrix for the shader must be Identity.
			pTerrain->patchesMetrices[iPatchNum] = S_Matrix4_Identity;

			Vector_PushBack(pTerrain->terrainPatches, pTerrainPatch);
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
	TerrainPatch pTerrainPatch = Vector_GetPtr(pTerrain->terrainPatches, iPatchNum);
	if (pTerrainPatch == NULL)
	{
		syserr("Patch %d not Initialized", iPatchNum);
		return;
	}

	// Clear Elements
	Vector_Clear(pTerrain->terrainPatches);

	// loop through each vertices
	for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
	{
		fPatchStartX = fOriginalPatchStartX;

		for (GLint iX = iPatchStartX; iX <= iPatchStartX + PATCH_XSIZE; iX++)
		{	
			// syslog("iPatchIndex: %d, fX: %f, fZ: %f", iPatchNum, fPatchStartX, fPatchStartZ);
			Vector3 worldPos = Vector3D(fPatchStartX, 0.0f, fPatchStartZ);
			TerrainPatch terrainPatch = TerrainPatch_Create(pTerrain, iPatchNum, iPatchNumX, iPatchNumZ, worldPos, PATCH_CELL_SIZE);

			if (terrainPatch == NULL)
			{
				syserr("Failed to Generate Terrain %d Patch %d", pTerrain->terrainIndex, iPatchNum);
				break;
			}

			terrainPatch->terrainMesh->pVertices;

			// Since we are baking the worldPos into the vertices, 
			// the matrix for the shader must be Identity.
			pTerrain->patchesMetrices[iPatchNum] = S_Matrix4_Identity;

			Vector_PushBack(pTerrain->terrainPatches, terrainPatch);

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

