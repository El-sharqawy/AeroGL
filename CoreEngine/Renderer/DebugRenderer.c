#include "DebugRenderer.h"
#include <memory.h>
#include <glad/glad.h>
#include "StateManager.h"
#include "../Core/Camera.h"
#include "../Math/Transform.h"
#include "../Engine.h"

bool CreateDebugRenderer(DebugRenderer* ppDebugRenderer, GLCamera pCamera, const char* szRendererName)
{
	*ppDebugRenderer = (DebugRenderer)tracked_malloc(sizeof(SDebugRenderer));

	DebugRenderer pDebugRenderer = *ppDebugRenderer;

	if (!pDebugRenderer)
	{
		syserr("Failed to Allocate Memory for Debug Renderer");
		return (false);
	}

	// Initialize everything into zeros and NULLs
	memset(pDebugRenderer, 0, sizeof(SDebugRenderer));
	
	pDebugRenderer->szRendererName = tracked_strdup(szRendererName);
	pDebugRenderer->pCamera = pCamera;

	if (!Shader_Initialize(&pDebugRenderer->pShader, "DebuggingRendererShader"))
	{
		syserr("Failed to Create DebuggingRendererShader");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	Shader_AttachShader(pDebugRenderer->pShader, "Assets/Shaders/debug_shader.vert");
	Shader_AttachShader(pDebugRenderer->pShader, "Assets/Shaders/debug_shader.frag");
	Shader_LinkProgram(pDebugRenderer->pShader);

	// Initialize Vertex & Index Buffers
	if (!Mesh3DGLBuffer_Initialize(&pDebugRenderer->pDynamicGeometryBuffer))
	{
		syserr("Failed to Create DebuggingRendererShader");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	// Initialize Metrices SSBO
	GLsizeiptr SSBOSize = MAX_DEBUG_MESHES * sizeof(Matrix4);
	if (!ShaderStorageBufferObject_Initialize(&pDebugRenderer->pRendererSSBO, SSBOSize, 0, "DebugRenderer SSBO")) // bind to 0, not bind to 0 - 1 that will cause shader mismatch!
	{
		syserr("Failed to Create Shader Storage Buffer");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	// Initialize Metrices
	for (int i = 0; i < MAX_DEBUG_MESHES; i++)
	{
		pDebugRenderer->modelsMetrices[i] = S_Matrix4_Identity;
	}

	DebugRenderer_InitGroup(pDebugRenderer, DEBUG_LINES, GL_LINES, MAX_DEBUG_LINE_MESHES);
	DebugRenderer_InitGroup(pDebugRenderer, DEBUG_TRIANGLES, GL_TRIANGLES, MAX_DEBUG_MESHES);

	pDebugRenderer->v4DiffuseColor = Vector4F(1.0f);
	pDebugRenderer->v3PickingPoint = Vector3F(0.0f);

	GLBuffer_ResetBuffer(pDebugRenderer->pDynamicGeometryBuffer);
	InitializeDebuggingMeshes(pDebugRenderer, DEBUG_LINES); 
	InitializeDebuggingMeshes(pDebugRenderer, DEBUG_TRIANGLES);

	return (true);
}

bool DebugRenderer_InitGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type, GLenum glType, GLsizeiptr capacity)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	// Initialize Indirect Buffer
	if (!IndirectBufferObject_Initialize(&group->pIndirectBuffer, capacity))
	{
		syserr("Failed to Create Indirect Buffer");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	if (!Vector_Init(&group->meshes, sizeof(Mesh3D)))
	{
		syserr("NewVectorInit failed for meshes!");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);  // Cleanup buffers above
	}

	group->meshes->destructor = Mesh3D_PtrDestroy;

	// Initialize Meshes
	if (type == DEBUG_LINES)
	{
		Mesh3D mesh1 = Mesh3D_Create(glType);
		// Mesh3D mesh2 = Mesh3D_Create(glType);

		Vector_PushBack(&group->meshes, &mesh1);
		// Vector_PushBack(&group->meshes, &mesh2);
	}
	else
	{
		Mesh3D sunlightMesh = Mesh3D_Create(glType);
		Vector_PushBack(&group->meshes, &sunlightMesh);

		Mesh3D sphereModel = Mesh3D_Create(glType);
		Vector_PushBack(&group->meshes, &sphereModel);
	}

	group->primitiveType = glType;
	group->groupType = type;

	return (true);
}

void DebugRenderer_DestroyGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	Vector_Destroy(&group->meshes);
	IndirectBufferObject_Destroy(&group->pIndirectBuffer);
	ShaderStorageBufferObject_Destroy(&pDebugRenderer->pRendererSSBO);
}

void DestroyDebugRenderer(DebugRenderer* ppDebugRenderer)
{
	if (!ppDebugRenderer || !*ppDebugRenderer)
	{
		return;
	}

	DebugRenderer pDebugRenderer = *ppDebugRenderer;

	if (pDebugRenderer->szRendererName)
	{
		tracked_free(pDebugRenderer->szRendererName);
	}

	DebugRenderer_DestroyGroup(pDebugRenderer, DEBUG_LINES);
	DebugRenderer_DestroyGroup(pDebugRenderer, DEBUG_TRIANGLES);

	Shader_Destroy(&pDebugRenderer->pShader);
	GLBuffer_DestroyBuffer(&pDebugRenderer->pDynamicGeometryBuffer);

	tracked_free(pDebugRenderer);

	*ppDebugRenderer = NULL;
}

void InitializeDebuggingMeshes(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	if (type == DEBUG_LINES)
	{
		Mesh3D mesh1 = VECTOR_GET(group->meshes, 0, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr
		Mesh3D_MakeAxis(mesh1, Vector3F(0.0f), 10.0f);

		// Mesh3D mesh2 = VECTOR_GET(group->meshes, 1, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr
		// Mesh3D_MakeWireSphere3D(mesh2, pDebugRenderer->v3PickingPoint, 1.0f, 64, 64, pDebugRenderer->v4DiffuseColor, false);
	}
	else
	{
		Vector3 sunPos = Vector3D(0.0f, 10.0f, 0.0f); // this will be light position
		Mesh3D sunlightMesh = VECTOR_GET(group->meshes, 0, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr
		Mesh3D_MakeSphere3D(sunlightMesh, sunPos, 1.0f, 64, 64, pDebugRenderer->v4DiffuseColor);
		Mesh3D_SetName(sunlightMesh, "TheSun");

		Vector3 sphereModelPos = Vector3D(0.0f, 1.0f, 0.0f); // this will be model position
		Mesh3D sphereModel = VECTOR_GET(group->meshes, 1, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr

		SetRenderColor(pDebugRenderer, Vector4D(0.5f, 0.5f, 1.0f, 1.0f));
		Mesh3D_MakeSphere3D(sphereModel, sphereModelPos, 1.0f, 64, 64, pDebugRenderer->v4DiffuseColor);
		Mesh3D_SetName(sphereModel, "Sphere");
	}

	// Reset buffer and commands
	IndirectBufferObject_Clear(group->pIndirectBuffer);
	
	Matrix4* gpuMatrices = (Matrix4*)pDebugRenderer->pRendererSSBO->pBufferData;
	// One single upload to the GPU
	for (int i = 0; i < group->meshes->count; i++)
	{
		Mesh3D mesh = VECTOR_GET(group->meshes, i, Mesh3D);  // Single line!

		if (!mesh || mesh->vertexCount == 0)
		{
			syserr("vertex count is 0 for mesh Index %d, group %d", i, group->groupType);
			continue;
		}

		// Capture buffer offsets
		mesh->vertexOffset = Mesh3DGLBuffer_GetBufferVertexOffset(pDebugRenderer->pDynamicGeometryBuffer);
		mesh->indexOffset = Mesh3DGLBuffer_GetBufferIndexOffset(pDebugRenderer->pDynamicGeometryBuffer);
		mesh->meshMatrixIndex = pDebugRenderer->meshCounter;

		// Upload mesh data (advances buffer offsets)
		Mesh3DGLBuffer_UploadData(pDebugRenderer->pDynamicGeometryBuffer, mesh);

		// Store Model Matrix in the CPU Array
		gpuMatrices[mesh->meshMatrixIndex] = TransformGetMatrix(&mesh->transform);

		// Add indirect command with correct offsets
		IndirectBufferObject_AddCommand(
			group->pIndirectBuffer,
			(GLuint)mesh->indexCount,
			1,
			(GLuint)mesh->indexOffset,   // Correct offset!
			(GLuint)mesh->vertexOffset,  // Correct offset!
			mesh->meshMatrixIndex                   // baseInstance = index into SSBO!
		);

		pDebugRenderer->meshCounter++;
	}

	// Upload all model matrices to SSBO, Upload only meshes we have made, at offset 0 without need to reallocate
	// ShaderStorageBufferObject_Update(pDebugRenderer->pRendererSSBO, pDebugRenderer->modelsMetrices, pDebugRenderer->meshCounter * sizeof(Matrix4), 0, false);

	// Upload indirect commands
	IndirectBufferObject_Upload(group->pIndirectBuffer);

	syslog("Initialized %d debug meshes", (GLint)group->pIndirectBuffer->commands->count);

	// reset back to white
	SetRenderColor(pDebugRenderer, Vector4D(1.0f, 1.0f, 1.0f, 1.0f));
}

void RenderDebugRenderer(DebugRenderer pDebugRenderer)
{
	PushState(GetStateManager());

	BindBufferVAO(GetStateManager(), pDebugRenderer->pDynamicGeometryBuffer);
	BindShader(GetStateManager(), pDebugRenderer->pShader);
	SetCapability(GetStateManager(), CAP_DEPTH_TEST, true);
	SetCapability(GetStateManager(), CAP_CULL_FACE, true);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	SetCapability(GetStateManager(), CAP_BLEND, false);
	SetBlendFunc(GetStateManager(), GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 1) First update transforms on CPU that depend on time / logic
	if (IsGLVersionHigher(4, 5))
	{
		// Option A: only update sun transform here
		SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[DEBUG_TRIANGLES];
		Mesh3D sunSphere = VECTOR_GET(group->meshes, 0, Mesh3D);
		if (sunSphere)
		{
			DebugRenderer_UpdateSunPosition(pDebugRenderer, sunSphere);
		}
	}

	// 2) Then upload all dirty matrices to SSBO
	DebugRenderer_UpdateDirtyMeshes(pDebugRenderer);


	if (IsGLVersionHigher(4, 5))
	{
		RenderDebugRendererIndirect(pDebugRenderer, DEBUG_TRIANGLES);
		RenderDebugRendererIndirect(pDebugRenderer, DEBUG_LINES);
	}
	else
	{
		RenderDebugRendererLegacy(pDebugRenderer, DEBUG_TRIANGLES);
		RenderDebugRendererLegacy(pDebugRenderer, DEBUG_LINES);
	}

	PopState(GetStateManager());
}

void RenderDebugRendererIndirect(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	if (type == DEBUG_TRIANGLES)
	{
		Mesh3D sunSphere = VECTOR_GET(group->meshes, 0, Mesh3D);  // Sun Sphere
		if (sunSphere)
		{
			Vector3 lightColor = Vector3D(sunSphere->meshColor.x, sunSphere->meshColor.y, sunSphere->meshColor.z);
			Shader_SetVec3(pDebugRenderer->pShader, "u_lightPos", sunSphere->transform.v3Position);
			Shader_SetVec3(pDebugRenderer->pShader, "u_lightColor", lightColor);
		}
	}
	// Execute all commands in one GPU call
	IndirectBufferObject_Draw(group->pIndirectBuffer, group->primitiveType);
}

void RenderDebugRendererLegacy(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	for (int i = 0; i < group->meshes->count; i++)
	{
		Mesh3D* pMeshPtr = Vector_Get(group->meshes, i);  // Mesh3D* from vector

		if (!pMeshPtr || !*pMeshPtr)
		{  // Check pointer valid + points to struct
			continue;
		}

		Mesh3D mesh = *pMeshPtr;  // Dereference: Mesh3D* -> actual struct ptr

		if (!mesh || mesh->vertexCount == 0)
		{
			continue;
		}

		// Set model matrix as uniform (instead of SSBO)
		Matrix4 modelMatrix = pDebugRenderer->modelsMetrices[i];
		Shader_SetMat4(pDebugRenderer->pShader, "u_matModel", modelMatrix);

		// Draw this mesh
		glDrawElementsBaseVertex(
			group->primitiveType,
			(GLsizei)mesh->indexCount,
			GL_UNSIGNED_INT,
			(void*)(mesh->indexOffset * sizeof(GLuint)),  // Index offset
			(GLint)mesh->vertexOffset                      // Vertex offset
		);
	}
}

void DebugRenderer_SetLineMeshPosition(DebugRenderer pDebugRenderer, int meshIndex, Vector3 v3NewPos)
{
	if (meshIndex >= MAX_DEBUG_LINE_MESHES || meshIndex < 0)
	{
		return;
	}

	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[DEBUG_LINES];
	Mesh3D lineMesh = Vector_Get(group->meshes, meshIndex);
	TransformSetPositionV(&lineMesh->transform, v3NewPos);
}

void DebugRenderer_UpdateSunPosition(DebugRenderer pDebugRenderer, Mesh3D sunSphere)
{
	static float angle = 0.0f;
	float orbitRadius = 15.0f;
	float orbitSpeed = 0.5f; // Radians per second

	angle += orbitSpeed * GetEngine()->deltaTime;

	// Calculate new X and Z positions
	float x = cosf(angle) * orbitRadius;
	float z = sinf(angle) * orbitRadius;
	float y = sunSphere->transform.v3Position.y; // Keep it at a constant height

	// Update the transform
	TransformSetPosition(&sunSphere->transform, x, y, z);

	sunSphere->bDirty = true;
}

void DebugRenderer_UpdateDirtyMeshes(DebugRenderer pDebugRenderer)
{
	Matrix4* gpuMatrices = NULL;

	if (pDebugRenderer->pRendererSSBO->isPersistent)
	{
		gpuMatrices = (Matrix4*)pDebugRenderer->pRendererSSBO->pBufferData;

		if (gpuMatrices == NULL)
		{
			syslog("Persistent map failed!");
			return;
		}
	}

	for (int32_t i = 0; i < pDebugRenderer->groups[DEBUG_TRIANGLES].meshes->count; i++)
	{
		Mesh3D mesh = VECTOR_GET(pDebugRenderer->groups[DEBUG_TRIANGLES].meshes, i, Mesh3D);  // Sun Sphere
		if (mesh->bDirty)
		{
			int32_t iMatIndex = mesh->meshMatrixIndex;
			// Update just our sun Matrix

			Matrix4 newMatrix = TransformGetMatrix(&mesh->transform);
			if (pDebugRenderer->pRendererSSBO->isPersistent)
			{
				gpuMatrices[mesh->meshMatrixIndex] = newMatrix;
			}
			else
			{
				ShaderStorageBufferObject_Update(pDebugRenderer->pRendererSSBO, &newMatrix, sizeof(Matrix4), iMatIndex * sizeof(Matrix4), false);
			}

			pDebugRenderer->modelsMetrices[iMatIndex] = newMatrix;
			mesh->bDirty = false;
		}
	}

	for (int32_t i = 0; i < pDebugRenderer->groups[DEBUG_LINES].meshes->count; i++)
	{
		Mesh3D mesh = VECTOR_GET(pDebugRenderer->groups[DEBUG_LINES].meshes, i, Mesh3D);  // Sun Sphere
		if (mesh->bDirty)
		{
			int32_t iMatIndex = mesh->meshMatrixIndex;
			// Update just our sun Matrix

			Matrix4 newMatrix = TransformGetMatrix(&mesh->transform);
			if (pDebugRenderer->pRendererSSBO->isPersistent)
			{
				gpuMatrices[mesh->meshMatrixIndex] = newMatrix;
			}
			else
			{
				ShaderStorageBufferObject_Update(pDebugRenderer->pRendererSSBO, &newMatrix, sizeof(Matrix4), iMatIndex * sizeof(Matrix4), false);
			}

			pDebugRenderer->modelsMetrices[iMatIndex] = newMatrix;
			mesh->bDirty = false;
		}
	}

	ShaderStorageBufferObject_Bind(pDebugRenderer->pRendererSSBO);
}

void SetRenderColor(DebugRenderer pDebugRenderer, Vector4 v4Color)
{
	pDebugRenderer->v4DiffuseColor = v4Color;
}
