#include "TerrainRenderer.h"
#include "../Stdafx.h"
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
	if (!TerrainRenderer_InitGLBuffers(pRenderer, GL_TRIANGLES, TERRAIN_PATCH_COUNT * iTerrainX * iTerrainZ))
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

bool TerrainRenderer_InitGLBuffers(TerrainRenderer pTerrainRenderer, GLenum glType, GLsizeiptr capacity)
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

	GLsizeiptr SSBOSize = capacity * sizeof(SPatchData);
	if (!ShaderStorageBufferObject_Initialize(&pTerrainRenderer->pRendererSSBO, SSBOSize, 0, "Terrain SSBO"))
	{
		syserr("Failed to Create Shader Storage Buffer");
		TerrainRenderer_Destroy(&pTerrainRenderer);
		return (false);
	}

	pTerrainRenderer->primitiveType = glType;

	return (true);
}

void TerrainRenderer_DestroyGLBuffers(TerrainRenderer pTerrainRenderer)
{
	IndirectBufferObject_Destroy(&pTerrainRenderer->pIndirectBuffer);
	ShaderStorageBufferObject_Destroy(&pTerrainRenderer->pRendererSSBO);
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

	SPatchData* gpuData = (SPatchData*)pTerrainRenderer->pRendererSSBO->pBufferData;

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

					// Capture buffer offsets
					terrainMesh->vertexOffset = GetTerrainBufferVertexOffset(pTerrainRenderer->pTerrainBuffer);
					terrainMesh->indexOffset = GetTerrainBufferIndexOffset(pTerrainRenderer->pTerrainBuffer);

					terrainMesh->meshMatrixIndex = pTerrain->baseGlobalPatchIndex + iPatchIndex;

					// Upload mesh data (advances buffer offsets)
					TerrainBuffer_UploadData(pTerrainRenderer->pTerrainBuffer, terrainMesh);

					terrainPatch->patchVerticesOffset = terrainMesh->vertexOffset;
					terrainPatch->patchIndicesOffset = terrainMesh->indexOffset;

					// ... inside the patch loops ...
					uint32_t ssboIndex = pTerrain->baseGlobalPatchIndex + iPatchIndex;

					// Store Model Matrix in the GPU Array
					if (pTerrainRenderer->pRendererSSBO->isPersistent)
					{
						gpuData[ssboIndex].modelMatrix = S_Matrix4_Identity;
						// This is the magic: Every patch in this terrain points to this heightmap
						gpuData[ssboIndex].heightMapHandle = pTerrain->pHeightMapTexture->textureHandle;
						gpuData[ssboIndex].heightScale = 1.0f; // Or whatever your scale is
						gpuData[ssboIndex].terrainCoords[0] = pTerrain->terrainXCoord;
						gpuData[ssboIndex].terrainCoords[1] = pTerrain->terrainZCoord;

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

	TerrainMap pTerrainMap = GetTerrainManager()->pTerrainMap;

	StateManager_PushState(GetStateManager());

	StateManager_BindTerrainBufferVAO(GetStateManager(), pTerrainRenderer->pTerrainBuffer);
	StateManager_BindShader(GetStateManager(), pTerrainRenderer->pTerrainShader);
	Shader_SetInt(pTerrainRenderer->pTerrainShader, "heightMapSize", HEIGHTMAP_RAW_XSIZE);

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
	ShaderStorageBufferObject_Bind(pTerrainRenderer->pRendererSSBO);

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
					Matrix4 modelMatrix = pTerrain->patchesMetrices[iPatchIndex];
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
