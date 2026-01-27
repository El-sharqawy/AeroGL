#include "TerrainRenderer.h"
#include "../Buffers/TerrainBuffer.h"
#include "../Renderer/StateManager.h"
#include "../Terrain/TerrainPatch.h"

bool TerrainRenderer_Initialize(TerrainRenderer* ppTerrainRenderer, GLCamera pCamera, const char* szRendererName)
{
	// Initialize Renderer
	*ppTerrainRenderer = (TerrainRenderer)tracked_calloc(1, sizeof(STerrainRenderer));
	
	// Validity Check
	TerrainRenderer pRenderer = *ppTerrainRenderer;
	if (!pRenderer)
	{
		syserr("Failed to Allocate memory for TerrainRenderer");
		return false;
	}

	pRenderer->szRendererName = tracked_strdup(szRendererName);
	pRenderer->pCamera = pCamera;

	// will Initialize it to render Triangles, with TERRAIN_PATCH_COUNT meshes as base Num
	if (!TerrainRenderer_InitGLBuffers(pRenderer, GL_TRIANGLES, TERRAIN_PATCH_COUNT * 10))
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
		tracked_free(pRenderer->szRendererName);
	}

	TerrainRenderer_DestroyGLBuffers(pRenderer);
	
	tracked_free(pRenderer);

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

	Shader_AttachShader(pTerrainRenderer->pTerrainShader, "Assets/Shaders/terrain_shader.vert");
	Shader_AttachShader(pTerrainRenderer->pTerrainShader, "Assets/Shaders/terrain_shader.frag");
	Shader_LinkProgram(pTerrainRenderer->pTerrainShader);

	// Initialize GPU Buffers
	if (!TerrainBuffer_Initialize(&pTerrainRenderer->pTerrainBuffer))
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

	GLsizeiptr SSBOSize = capacity * sizeof(Matrix4);
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

void TerrainRenderer_UploadGPUData(TerrainRenderer pTerrainRenderer, TerrainMap pTerrainMap)
{
	if (pTerrainMap->isReady == false)
	{
		syserr("Map is not Ready");
		return;
	}

	// Reset buffer and commands
	IndirectBufferObject_Clear(pTerrainRenderer->pIndirectBuffer);

	Matrix4* gpuMatrices = (Matrix4*)pTerrainRenderer->pRendererSSBO->pBufferData;

	int32_t terrainsZNum = pTerrainMap->terrainsZCount;
	int32_t terrainsXNum = pTerrainMap->terrainsXCount;

	uint32_t globalPatchIndex = 0;

	for (int32_t iTerrNumZ = 0; iTerrNumZ < terrainsZNum; iTerrNumZ++)
	{
		for (int32_t iTerrNumX = 0; iTerrNumX < terrainsXNum; iTerrNumX++)
		{
			int32_t iTerrainIndex = iTerrNumZ * terrainsXNum + iTerrNumX;
			Terrain pTerrain = VECTOR_GET(pTerrainMap->terrains, iTerrainIndex, Terrain);
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
					TerrainPatch terrainPatch = VECTOR_GET(pTerrain->terrainPatches, iPatchIndex, TerrainPatch);  // Single line!

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

					// Store Model Matrix in the GPU Array
					gpuMatrices[pTerrain->baseGlobalPatchIndex + iPatchIndex] = pTerrain->patchesMetrices[iPatchIndex];

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

void TerrainRenderer_Render(TerrainRenderer pTerrainRenderer, TerrainMap pTerrainMap)
{
	if (pTerrainMap == NULL)
	{
		return;
	}

	if (pTerrainMap->isReady == false)
	{
		return;
	}

	PushState(GetStateManager());

	BindTerrainBufferVAO(GetStateManager(), pTerrainRenderer->pTerrainBuffer);
	BindShader(GetStateManager(), pTerrainRenderer->pTerrainShader);
	SetCapability(GetStateManager(), CAP_DEPTH_TEST, true);
	SetCapability(GetStateManager(), CAP_CULL_FACE, true);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	SetCapability(GetStateManager(), CAP_BLEND, false);
	SetBlendFunc(GetStateManager(), GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (IsGLVersionHigher(4, 5))
	{
		TerrainRenderer_RenderIndirect(pTerrainRenderer);
	}
	else
	{
		TerrainRenderer_RenderLegacy(pTerrainRenderer);
	}

	PopState(GetStateManager());
}

void TerrainRenderer_RenderIndirect(TerrainRenderer pTerrainRenderer)
{
	ShaderStorageBufferObject_Bind(pTerrainRenderer->pRendererSSBO);

	// Execute all commands in one GPU call
	IndirectBufferObject_Draw(pTerrainRenderer->pIndirectBuffer, pTerrainRenderer->primitiveType);
}

void TerrainRenderer_RenderLegacy(TerrainRenderer pTerrainRenderer)
{
	PushState(GetStateManager());

	BindTerrainBufferVAO(GetStateManager(), pTerrainRenderer->pTerrainBuffer);
	BindShader(GetStateManager(), pTerrainRenderer->pTerrainShader);

	/*for (int i = 0; i < pTerrainRenderer->terrain->terrainPatches->count; i++)
	{
		TerrainPatch terrainPatch = VECTOR_GET(pTerrainRenderer->terrain->terrainPatches, i, TerrainPatch);  // Single line!

		if (!terrainPatch)
		{  // Check pointer valid + points to struct
			continue;
		}

		TerrainMesh mesh = terrainPatch->terrainMesh;;  // Dereference: Mesh3D* -> actual struct ptr

		if (!mesh || mesh->vertexCount == 0)
		{
			continue;
		}

		// Set model matrix as uniform (instead of SSBO)
		Matrix4 modelMatrix = TransformGetMatrix(&mesh->transform);
		Shader_SetMat4(pTerrainRenderer->pTerrainShader, "u_matModel", modelMatrix);

		// Draw this mesh
		glDrawElementsBaseVertex(
			pTerrainRenderer->primitiveType,
			(GLsizei)mesh->indexCount,
			GL_UNSIGNED_INT,
			(void*)(mesh->indexOffset * sizeof(GLuint)),  // Index offset
			(GLint)mesh->vertexOffset                      // Vertex offset
		);
	}*/

	PopState(GetStateManager());
}

void TerrainRenderer_Reset(TerrainRenderer pTerrainRenderer)
{
	// Reset buffer and commands
	TerrainBuffer_Reset(pTerrainRenderer->pTerrainBuffer);
	IndirectBufferObject_Clear(pTerrainRenderer->pIndirectBuffer);
}
