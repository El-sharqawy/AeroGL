#include "TerrainRenderer.h"
#include "Stdafx.h"
#include "../Buffers/TerrainBuffer.h"
#include "../PipeLine/StateManager.h"
#include "../Terrain/TerrainPatch.h"
#include "../PipeLine/Texture.h"

bool TerrainRenderer_Initialize(TerrainRenderer* ppTerrainRenderer, const char* szRendererName, int32_t iTerrainX, int32_t iTerrainZ)
{
	if (ppTerrainRenderer == NULL)
	{
		syserr("ppTerrainRenderer is NULL (invalid address)");
		return false;
	}

	// Initialize Renderer
	*ppTerrainRenderer = engine_new_zero(STerrainRenderer, 1, MEM_TAG_RENDERING);

	// Validity Check
	TerrainRenderer pRenderer = *ppTerrainRenderer;
	if (!pRenderer)
	{
		syserr("Failed to Allocate memory for TerrainRenderer");
		return false;
	}

	pRenderer->szRendererName = engine_strdup(szRendererName, MEM_TAG_STRINGS);
	pRenderer->pCamera = GetEngine()->camera;

	// will Initialize it to render Triangles, with TERRAIN_PATCH_COUNT meshes as base Num
	if (!TerrainRenderer_InitGLBuffers(pRenderer, GL_TRIANGLES, iTerrainX, iTerrainZ))
	{
		// Clean Up Memory
		TerrainRenderer_Destroy(&pRenderer);

		syserr("Failed to Create Terrain Group Buffers");
		return (false);
	}

	return (true);
}

void TerrainRenderer_Destroy(TerrainRenderer* ppTerrainRenderer)
{
	if (!ppTerrainRenderer || !*ppTerrainRenderer)
	{
		return;
	}

	TerrainRenderer pRenderer = *ppTerrainRenderer;

	if (pRenderer->szRendererName)
	{
		engine_delete(pRenderer->szRendererName);
	}

	TerrainRenderer_DestroyGLBuffers(pRenderer);
	
	engine_delete(pRenderer);

	*ppTerrainRenderer = NULL;
}

bool TerrainRenderer_InitGLBuffers(TerrainRenderer pTerrainRenderer, GLenum glType, GLint iTerrainX, GLint iTerrainZ)
{
	// Shader Initialization
	if (!Shader_Initialize(&pTerrainRenderer->pTerrainShader, "Terrain Shader"))
	{
		// Clean Up Memory
		TerrainRenderer_Destroy(&pTerrainRenderer);

		syserr("Failed to Create Terrain Shader");
		return (false);
	}

	Shader_SetInjection(pTerrainRenderer->pTerrainShader, true);
	Shader_AttachShader(pTerrainRenderer->pTerrainShader, "Assets/Shaders/terrain_shader.vert");
	Shader_AttachShader(pTerrainRenderer->pTerrainShader, "Assets/Shaders/terrain_shader.frag");
	Shader_LinkProgram(pTerrainRenderer->pTerrainShader);

	// Initialize GPU Buffers
	GLsizeiptr capacity = TERRAIN_PATCH_COUNT * iTerrainX * iTerrainZ;
	if (!TerrainBuffer_Initialize(&pTerrainRenderer->pTerrainBuffer, capacity))
	{
		// Clean Up Memory
		TerrainRenderer_Destroy(&pTerrainRenderer);

		syserr("Failed to Create Terrain Buffer");
		return (false);
	}

	// Initialize Indirect Buffer
	if (!IndirectBufferObject_Initialize(&pTerrainRenderer->pIndirectBuffer, capacity))
	{
		syserr("Failed to Create Indirect Buffer");
		TerrainRenderer_Destroy(&pTerrainRenderer);
		return (false);
	}

	int32_t totalTerrains = iTerrainX * iTerrainZ;
	GLsizeiptr terrainSSBOSize = totalTerrains * sizeof(STerrainGPUData);
	if (!ShaderStorageBufferObject_Initialize(&pTerrainRenderer->pTerrainRendererSSBO, terrainSSBOSize, SSBO_BP_TERRAIN_DATA, "Terrain SSBO"))
	{
		syserr("Failed to Create Shader Storage Buffer For Terrains");
		TerrainRenderer_Destroy(&pTerrainRenderer);
		return (false);
	}

	int32_t totalPatches = totalTerrains * TERRAIN_PATCH_COUNT; // e.g., 4 * 64
	GLsizeiptr patchSSBOSize = totalPatches * sizeof(STerrainGPUData);
	if (!ShaderStorageBufferObject_Initialize(&pTerrainRenderer->pPatchRendererSSBO, patchSSBOSize, SSBO_BP_PATCHES_DATA, "Patch SSBO"))
	{
		syserr("Failed to Create Shader Storage Buffer For Patches");
		TerrainRenderer_Destroy(&pTerrainRenderer);
		return (false);
	}

	int32_t totalHeightMap = totalTerrains * (HEIGHTMAP_RAW_XSIZE * HEIGHTMAP_RAW_ZSIZE) * sizeof(GLfloat);
	if (!ShaderStorageBufferObject_Initialize(&pTerrainRenderer->pHeightMapSSBO, totalHeightMap, SSBO_BP_TERRAIN_HEIGHTMAP, "Global HeightMap SSBO"))
	{
		syserr("Failed to Create Shader Storage Buffer For Patches");
		TerrainRenderer_Destroy(&pTerrainRenderer);
		return (false);
	}

	pTerrainRenderer->primitiveType = glType;

	return (true);
}

void TerrainRenderer_DestroyGLBuffers(TerrainRenderer pTerrainRenderer)
{
	IndirectBufferObject_Destroy(&pTerrainRenderer->pIndirectBuffer);
	ShaderStorageBufferObject_Destroy(&pTerrainRenderer->pTerrainRendererSSBO);
	ShaderStorageBufferObject_Destroy(&pTerrainRenderer->pPatchRendererSSBO);
	ShaderStorageBufferObject_Destroy(&pTerrainRenderer->pHeightMapSSBO);
	Shader_Destroy(&pTerrainRenderer->pTerrainShader);
	TerrainBuffer_Destroy(&pTerrainRenderer->pTerrainBuffer);
}

void TerrainRenderer_UploadGPUData(TerrainRenderer pTerrainRenderer)
{
	if (GetTerrainManager()->isMapReady == false)
	{
		syserr("Map is not Ready");
		return;
	}

	TerrainMap pTerrainMap = GetTerrainManager()->pTerrainMap;

	// Reset buffer and commands
	IndirectBufferObject_Clear(pTerrainRenderer->pIndirectBuffer);

	STerrainGPUData* terrainGPUData = (STerrainGPUData*)pTerrainRenderer->pTerrainRendererSSBO->pBufferData;
	SPatchGPUData* patchGPUData = (SPatchGPUData*)pTerrainRenderer->pPatchRendererSSBO->pBufferData;
	GLfloat* heightMapGPUData = (GLfloat*)pTerrainRenderer->pHeightMapSSBO->pBufferData;

	int32_t terrainsZNum = pTerrainMap->terrainsZCount;
	int32_t terrainsXNum = pTerrainMap->terrainsXCount;

	uint32_t globalPatchIndex = 0;
	GLsizeiptr heightMapOffset = 0; // height map offset in the global .. per terrain

	for (int32_t iTerrNumZ = 0; iTerrNumZ < terrainsZNum; iTerrNumZ++)
	{
		for (int32_t iTerrNumX = 0; iTerrNumX < terrainsXNum; iTerrNumX++)
		{
			int32_t iTerrainIndex = iTerrNumZ * terrainsXNum + iTerrNumX;
			Terrain pTerrain = Vector_GetPtr(pTerrainMap->terrains, iTerrainIndex);
			if (!pTerrain)
			{
				syserr("Error At Terrain %d", iTerrainIndex);
				break;
			}

			pTerrain->baseGlobalPatchIndex = globalPatchIndex;  // Terrain 0,0: 0
			pTerrain->globalOffset = heightMapOffset;  // Byte offset


			if (pTerrainRenderer->pHeightMapSSBO->isPersistent)
			{
				for (int32_t i = 0; i < 3; i++)
				{
					pTerrain->mapPtrs[i] = (char*)heightMapGPUData + heightMapOffset;
				}
			}

			// Store Model Matrix in the GPU Array
			if (pTerrainRenderer->pTerrainRendererSSBO->isPersistent)
			{
				// This is the magic: Every patch in this terrain points to this heightmap
				terrainGPUData[iTerrainIndex].heightOffset = (uint32_t)pTerrain->globalOffset / sizeof(GLfloat);
				terrainGPUData[iTerrainIndex].terrainCoords[0] = pTerrain->terrainXCoord;
				terrainGPUData[iTerrainIndex].terrainCoords[1] = pTerrain->terrainZCoord;
			}

			for (int32_t iPatchZ = 0; iPatchZ < PATCH_ZCOUNT; iPatchZ++)
			{
				for (int32_t iPatchX = 0; iPatchX < PATCH_XCOUNT; iPatchX++)
				{
					int32_t iPatchIndex = iPatchZ * PATCH_XCOUNT + iPatchX;
					TerrainPatch terrainPatch = Vector_GetPtr(pTerrain->terrainPatches, iPatchIndex);  // Single line!

					if (!terrainPatch || !terrainPatch->terrainMesh || terrainPatch->terrainMesh->vertexCount == 0)
					{
						syserr("vertex count is 0 for mesh Index %d", iPatchIndex);
						continue;
					}

					TerrainMesh terrainMesh = terrainPatch->terrainMesh;

					// Capture buffer offsets
					terrainMesh->vertexOffset = TerrainBuffer_GetVertexOffset(pTerrainRenderer->pTerrainBuffer);
					terrainMesh->indexOffset = TerrainBuffer_GetIndexOffset(pTerrainRenderer->pTerrainBuffer);

					terrainMesh->meshMatrixIndex = pTerrain->baseGlobalPatchIndex + iPatchIndex;

					// Upload mesh data (advances buffer offsets)
					TerrainBuffer_UploadData(pTerrainRenderer->pTerrainBuffer, terrainMesh);

					terrainPatch->patchVerticesOffset = terrainMesh->vertexOffset;
					terrainPatch->patchIndicesOffset = terrainMesh->indexOffset;

					// ... inside the patch loops ...
					uint32_t ssboIndex = pTerrain->baseGlobalPatchIndex + iPatchIndex;

					// Store Model Matrix in the GPU Array
					if (pTerrainRenderer->pPatchRendererSSBO->isPersistent)
					{
						// This is the magic: Every patch in this terrain points to this heightmap
						patchGPUData[ssboIndex].terrainIndex = iTerrainIndex;
						patchGPUData[ssboIndex].localPatchID = iPatchIndex;
					}

					// Add indirect command with correct offsets
					IndirectBufferObject_AddCommand(
						pTerrainRenderer->pIndirectBuffer,
						(GLuint)terrainMesh->indexCount,
						1,
						(GLuint)terrainMesh->indexOffset,			// Correct offset!
						(GLuint)terrainMesh->vertexOffset,			// Correct offset!
						pTerrain->baseGlobalPatchIndex + iPatchIndex					// baseInstance = index into SSBO!
					);
				}
			}

			globalPatchIndex += PATCH_ZCOUNT * PATCH_XCOUNT;
			heightMapOffset += pTerrain->sliceBytes;
		}
	}

	// Upload all model matrices to SSBO, Upload only meshes we have made, at offset 0 without need to reallocate
	// ShaderStorageBufferObject_Update(group->pRendererSSBO, group->modelsMetrices, pTerrainRenderer->meshCounter * sizeof(Matrix4), 0, false);

	// Upload indirect commands
	IndirectBufferObject_Upload(pTerrainRenderer->pIndirectBuffer);
}

void TerrainRenderer_Render(TerrainRenderer pTerrainRenderer)
{
	if (GetTerrainManager()->isMapReady == false)
	{
		syserr("Map is not Ready");
		return;
	}

	// TerrainMap pTerrainMap = GetTerrainManager()->pTerrainMap;

	StateManager_PushState(GetStateManager());

	StateManager_BindTerrainBufferVAO(GetStateManager(), pTerrainRenderer->pTerrainBuffer);
	StateManager_BindShader(GetStateManager(), pTerrainRenderer->pTerrainShader);

	// Set model matrix as uniform (instead of SSBO)
	Shader_SetFloat(pTerrainRenderer->pTerrainShader, "ENGINE_CELL_SIZE", (float)ENGINE_CELL_SIZE);
	Shader_SetVec2(pTerrainRenderer->pTerrainShader, "TERRAIN_SIZE", Vector2Di(TERRAIN_XSIZE, TERRAIN_ZSIZE));
	Shader_SetInt(pTerrainRenderer->pTerrainShader, "TERRAIN_HEIGHTMAP_RAW_XSIZE", HEIGHTMAP_RAW_XSIZE);

	StateManager_SetCapability(GetStateManager(), CAP_DEPTH_TEST, true);
	StateManager_SetCapability(GetStateManager(), CAP_CULL_FACE, true);
	StateManager_SetFrontFace(GetStateManager(), GL_CCW);
	StateManager_SetCullFace(GetStateManager(), GL_BACK);

	StateManager_SetCapability(GetStateManager(), CAP_BLEND, false);
	StateManager_SetBlendFunc(GetStateManager(), GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (IsGLVersionHigher(4, 5))
	{
		TerrainRenderer_RenderIndirect(pTerrainRenderer);
	}
	else
	{
		TerrainRenderer_RenderLegacy(pTerrainRenderer);
	}

	StateManager_PopState(GetStateManager());
}

void TerrainRenderer_RenderIndirect(TerrainRenderer pTerrainRenderer)
{
	ShaderStorageBufferObject_Bind(pTerrainRenderer->pTerrainRendererSSBO);
	ShaderStorageBufferObject_Bind(pTerrainRenderer->pPatchRendererSSBO);
	ShaderStorageBufferObject_Bind(pTerrainRenderer->pHeightMapSSBO);

	// Execute all commands in one GPU call
	IndirectBufferObject_Draw(pTerrainRenderer->pIndirectBuffer, pTerrainRenderer->primitiveType);
}

void TerrainRenderer_RenderLegacy(TerrainRenderer pTerrainRenderer)
{
	if (GetTerrainManager()->isMapReady == false)
	{
		syserr("Map is not Ready");
		return;
	}

	TerrainMap pTerrainMap = GetTerrainManager()->pTerrainMap;

	StateManager_PushState(GetStateManager());

	StateManager_BindTerrainBufferVAO(GetStateManager(), pTerrainRenderer->pTerrainBuffer);
	StateManager_BindShader(GetStateManager(), pTerrainRenderer->pTerrainShader);

	int32_t terrainsZNum = pTerrainMap->terrainsZCount;
	int32_t terrainsXNum = pTerrainMap->terrainsXCount;

	uint32_t globalPatchIndex = 0;

	for (int32_t iTerrNumZ = 0; iTerrNumZ < terrainsZNum; iTerrNumZ++)
	{
		for (int32_t iTerrNumX = 0; iTerrNumX < terrainsXNum; iTerrNumX++)
		{
			int32_t iTerrainIndex = iTerrNumZ * terrainsXNum + iTerrNumX;
			Terrain pTerrain = Vector_GetPtr(pTerrainMap->terrains, iTerrainIndex);
			if (!pTerrain)
			{
				syserr("Error At Terrain %d", iTerrainIndex);
				break;
			}

			pTerrain->baseGlobalPatchIndex = globalPatchIndex;  // Terrain 0,0: 0

			for (int32_t iPatchZ = 0; iPatchZ < PATCH_ZCOUNT; iPatchZ++)
			{
				for (int32_t iPatchX = 0; iPatchX < PATCH_XCOUNT; iPatchX++)
				{
					int32_t iPatchIndex = iPatchZ * PATCH_XCOUNT + iPatchX;
					TerrainPatch terrainPatch = Vector_GetPtr(pTerrain->terrainPatches, iPatchIndex);  // Single line!

					if (!terrainPatch || !terrainPatch->terrainMesh || terrainPatch->terrainMesh->vertexCount == 0)
					{
						syserr("vertex count is 0 for mesh Index %d", iPatchIndex);
						continue;
					}

					TerrainMesh terrainMesh = terrainPatch->terrainMesh;

					// Set model matrix as uniform (instead of SSBO)
					Shader_SetMat4(pTerrainRenderer->pTerrainShader, "u_matModel", S_Matrix4_Identity);
					Shader_SetInt(pTerrainRenderer->pTerrainShader, "u_vertex_DrawID", terrainMesh->meshMatrixIndex);

					// Draw this mesh
					glDrawElementsBaseVertex(
						pTerrainRenderer->primitiveType,
						(GLsizei)terrainMesh->indexCount,
						GL_UNSIGNED_INT,
						(void*)(terrainMesh->indexOffset * sizeof(GLuint)),  // Index offset
						(GLint)terrainMesh->vertexOffset                      // Vertex offset
					);
				}
			}
		}
	}

	StateManager_PopState(GetStateManager());
}

void TerrainRenderer_Reset(TerrainRenderer pTerrainRenderer)
{
	// Reset buffer and commands
	TerrainBuffer_Reset(pTerrainRenderer->pTerrainBuffer);
	IndirectBufferObject_Clear(pTerrainRenderer->pIndirectBuffer);
}
