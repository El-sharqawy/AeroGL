#include "Terrain.h"
#include "Stdafx.h"
#include "../TerrainMap/TerrainMap.h"
#include "../TerrainData.h"
#include "../TerrainPatch.h"
#include "../../Math/Grids/FloatGrid.h"
#include "../../PipeLine/Texture.h"
#include "../../Math/Matrix/Matrix3.h"

bool Terrain_Initialize(Terrain* ppTerrain)
{
	if (ppTerrain == NULL)
	{
		syserr("ppTerrain is NULL (invalid address)");
		return false;
	}

	*ppTerrain = engine_new_zero(STerrain, 1, MEM_TAG_TERRAIN);

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

	engine_delete(pTerrain);

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
		syserr("No Terrain Allocated");
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
			if (!TerrainPatch_Initialize(&pTerrainPatch, pTerrain, iPatchNum))
			{
				syserr("Failed to Initialize Patch %d", iPatchNum);
				return (false);
			}
			
			Vector_PushBack(pTerrain->terrainPatches, pTerrainPatch);
		}
	}

	Terrain_UpdatePatches(pTerrain);
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

	float fPatchXSizeMeters = PATCH_XSIZE * ENGINE_CELL_SIZE;
	float fPatchZSizeMeters = PATCH_ZSIZE * ENGINE_CELL_SIZE;

	float fPatchStartX = (float)(pTerrain->terrainXCoord * XSIZE * ENGINE_CELL_SIZE) + (float)(iPatchStartX * ENGINE_CELL_SIZE);
	float fPatchStartZ = (float)(pTerrain->terrainZCoord * ZSIZE * ENGINE_CELL_SIZE) + (float)(iPatchStartZ * ENGINE_CELL_SIZE);

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
	TerrainMesh mesh = pTerrainPatch->terrainMesh;

	Vector_Clear(mesh->pVertices);
	Vector_Clear(mesh->pIndices);

	// Normals Calculations
	Matrix4 model = TransformGetMatrix(&pTerrainPatch->terrainMesh->transform);
	Matrix3 mat3Model = Matrix3_InitMatrix4(model);
	Matrix3 invModel = Matrix3_Inverse(mat3Model);
	Matrix3 normalMatrix = Matrix3_TransposeN(invModel);

	// vertex color
	Vector4 color;
	color.r = (float)(iPatchNum % 8) / 8.0f;      // Red: 0,0.125,0.25...
	color.g = (float)(iPatchNum / 8 % 8) / 8.0f;  // Green: cycles every 8
	color.b = 0.5f + 0.5f * sinf(iPatchNum * 0.3f);  // Blue: rainbow
	color.a = 1.0f;

	// loop through each vertices
	for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
	{
		fPatchStartX = fOriginalPatchStartX;

		for (GLint iX = iPatchStartX; iX <= iPatchStartX + PATCH_XSIZE; iX++)
		{	
			// syslog("iPatchIndex: %d, fX: %f, fZ: %f", iPatchNum, fPatchStartX, fPatchStartZ);
			STerrainVertex vertex = { 0 };

			vertex.v3Position = Vector3D(fPatchStartX, 0.0f, fPatchStartZ);
			vertex.v2TexCoords = Vector2D((fPatchStartX - fOriginalPatchStartX) / fPatchXSizeMeters, (fPatchStartZ - fOriginalPatchStartZ) / fPatchZSizeMeters);
			vertex.v3Normals = Vector3D(0.0f, 1.0f, 0.0f);
			vertex.v4Color = color;

			TerrainMesh_AddVertex(mesh, vertex);

			fPatchStartX += (GLfloat)ENGINE_CELL_SIZE;
		}

		fPatchStartZ += (GLfloat)ENGINE_CELL_SIZE;
	}

	TerrainPatch_InitializeIndices(pTerrainPatch);
}

void Terrain_Update(Terrain pTerrain)
{
	if (!pTerrain)
	{
		return;
	}

	printf("Debug: pTerrain address is %p heightMap Address is %p\n", (void*)pTerrain, (void*)pTerrain->heightMap);

	if (!pTerrain->heightMap)
	{
		syserr("Height map is not Initialized Yet!");
		return;
	}

	// Update HeightMap Buffer
	if (pTerrain->heightMap->isDirty)
	{
		int32_t writeIdx = (pTerrain->currentRing + 1) % 3;

		// Microsecond wait (triple buffer = almost instant)
		GLenum waitResult = glClientWaitSync(pTerrain->fences[writeIdx], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);  // 1ms timeout

		if (waitResult == GL_TIMEOUT_EXPIRED)
		{
			syslog("Terrain fence timeout — GPU overloaded?");
			return;
		}

		glDeleteSync(pTerrain->fences[writeIdx]);

		GLfloat* slicePtr = (float*)pTerrain->mapPtrs[writeIdx];

		// CPU height calc -> direct write!
		memcpy(slicePtr, pTerrain->heightMap->pArray, pTerrain->sliceBytes);

		pTerrain->fences[writeIdx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		pTerrain->currentRing = writeIdx;

		FloatGrid_SetDirty(pTerrain->heightMap, false);
	}
}

void Terrain_SetParentMap(Terrain pTerrain, STerrainMap* pParentMap)
{
	pTerrain->parentMap = pParentMap;
}

TerrainMap Terrain_GetParentMap(Terrain pTerrain)
{
	return (pTerrain->parentMap);
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

