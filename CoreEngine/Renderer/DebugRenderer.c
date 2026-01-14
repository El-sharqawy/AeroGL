#include "DebugRenderer.h"
#include <memory.h>
#include <glad/glad.h>
#include "StateManager.h"
#include "../Core/Camera.h"
#include "../Math/Transform.h"

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

	if (!InitializeShader(&pDebugRenderer->pShader, "DebuggingRendererShader"))
	{
		syserr("Failed to Create DebuggingRendererShader");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	AttachShader(pDebugRenderer->pShader, "Assets/Shaders/debug_shader.vert");
	AttachShader(pDebugRenderer->pShader, "Assets/Shaders/debug_shader.frag");
	LinkProgram(pDebugRenderer->pShader);

	// Initialize Vertex & Index Buffers
	if (!InitializeMesh3DGLBuffer(&pDebugRenderer->pDynamicGeometryBuffer))
	{
		syserr("Failed to Create DebuggingRendererShader");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	DebugRenderer_InitGroup(pDebugRenderer, DEBUG_LINES, GL_LINES, MAX_DEBUG_LINE_MESHES);
	DebugRenderer_InitGroup(pDebugRenderer, DEBUG_TRIANGLES, GL_TRIANGLES, MAX_DEBUG_MESHES);

	pDebugRenderer->v4DiffuseColor = Vector4F(1.0f);
	pDebugRenderer->v3PickingPoint = Vector3F(0.0f);

	InitializeDebuggingMeshes(pDebugRenderer, DEBUG_LINES); 
	InitializeDebuggingMeshes(pDebugRenderer, DEBUG_TRIANGLES);

	return (true);
}

bool DebugRenderer_InitGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type, GLenum glType, GLsizeiptr capacity)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	// Initialize Indirect Buffer
	if (!InitializeIndirectBuffer(&group->pIndirectBuffer, capacity))
	{
		syserr("Failed to Create Indirect Buffer");
		DestroyDebugRenderer(&pDebugRenderer);
		return (false);
	}

	// Initialize Metrices SSBO
	GLsizeiptr SSBOSize = capacity * sizeof(Matrix4);
	if (!InitializeShaderStorageBufferObject(&group->pRendererSSBO, SSBOSize, type, "DebugRenderer SSBO"))
	{
		syserr("Failed to Create Shader Storage Buffer");
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
		Mesh3D mesh2 = Mesh3D_Create(glType);

		Vector_PushBack(&group->meshes, &mesh1);
		Vector_PushBack(&group->meshes, &mesh2);
	}
	else
	{
		Mesh3D mesh1 = Mesh3D_Create(glType);
		Vector_PushBack(&group->meshes, &mesh1);
	}

	// Initialize Metrices
	for (int i = 0; i < capacity; i++)
	{
		group->modelsMetrices[i] = S_Matrix4_Identity;
	}

	group->bMatricesDirty = false;
	group->primitiveType = glType;
	group->groupType = type;

	return (true);
}

void DebugRenderer_DestroyGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	Vector_Destroy(&group->meshes);
	DestroyIndirectBuffer(&group->pIndirectBuffer);
	DestroyShaderStorageBufferObject(&group->pRendererSSBO);
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

	DestroyProgram(&pDebugRenderer->pShader);
	DestroyBuffer(&pDebugRenderer->pDynamicGeometryBuffer);

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

		Mesh3D mesh2 = VECTOR_GET(group->meshes, 1, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr
		Mesh3D_MakeWireSphere3D(mesh2, pDebugRenderer->v3PickingPoint, 1.0f, 64, 64, pDebugRenderer->v4DiffuseColor, false);
	}
	else
	{
		Vector3 topLeft = Vector3D(0.0f, 0.0f, 0.0f);
		Vector3 topRight = Vector3D(1.0f, 0.0f, 0.0f);
		Vector3 bottomLeft = Vector3D(0.0f, 1.0f, 0.0f);
		Vector3 bottomRight = Vector3D(1.0f, 1.0f, 0.0f);

		Mesh3D mesh1 = VECTOR_GET(group->meshes, 0, Mesh3D);  // Dereference: Mesh3D* -> actual struct ptr
		Mesh3D_MakeQuad3D(mesh1, topLeft, topRight, bottomLeft, bottomRight, pDebugRenderer->v4DiffuseColor);
	}

	// Reset buffer and commands
	// ResetBuffer(pDebugRenderer->pDynamicGeometryBuffer);
	IndirectBufferObject_Clear(group->pIndirectBuffer);

	int meshCount = 0;
	
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
		mesh->vertexOffset = GetBufferVertexOffset(pDebugRenderer->pDynamicGeometryBuffer);
		mesh->indexOffset = GetBufferIndexOffset(pDebugRenderer->pDynamicGeometryBuffer);

		// Upload mesh data (advances buffer offsets)
		UpdateBufferMesh3DData(pDebugRenderer->pDynamicGeometryBuffer, mesh);

		// Store Model Matrix in the CPU Array
		group->modelsMetrices[meshCount] = TransformGetMatrix(&mesh->transform);

		// Add indirect command with correct offsets
		IndirectBufferObject_AddCommand(
			group->pIndirectBuffer,
			(GLuint)mesh->indexCount,
			1,
			(GLuint)mesh->indexOffset,   // Correct offset!
			(GLuint)mesh->vertexOffset,  // Correct offset!
			meshCount                   // baseInstance = index into SSBO!
		);

		meshCount++;
	}

	// Upload all model matrices to SSBO, Upload only meshes we have made, at offset 0 without need to reallocate
	ShaderStorageBufferObject_Update(group->pRendererSSBO, group->modelsMetrices, meshCount * sizeof(Matrix4), 0, false);

	// Upload indirect commands
	IndirectBufferObject_Upload(group->pIndirectBuffer);

	syslog("Initialized %d debug meshes", (GLint)group->pIndirectBuffer->commands->count);

	// reset back to white
	SetRenderColor(pDebugRenderer, Vector4D(1.0f, 1.0f, 1.0f, 1.0f));
}

void RenderDebugRenderer(DebugRenderer pDebugRenderer)
{
	if (IsGLVersionHigher(4, 5))
	{
		RenderDebugRendererIndirect(pDebugRenderer, DEBUG_LINES);
		RenderDebugRendererIndirect(pDebugRenderer, DEBUG_TRIANGLES);
	}
	else
	{
		RenderDebugRendererLegacy(pDebugRenderer, DEBUG_LINES);
		RenderDebugRendererLegacy(pDebugRenderer, DEBUG_TRIANGLES);
	}
}

void RenderDebugRendererIndirect(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	PushState(GetStateManager());

	BindBufferVAO(GetStateManager(), pDebugRenderer->pDynamicGeometryBuffer);
	BindShader(GetStateManager(), pDebugRenderer->pShader);

	// Execute all commands in one GPU call
	IndirectBufferObject_Draw(group->pIndirectBuffer, group->primitiveType);

	PopState(GetStateManager());
}

void RenderDebugRendererLegacy(DebugRenderer pDebugRenderer, EDebugPrimitiveType type)
{
	SDebugRendererPrimitiveGroup* group = &pDebugRenderer->groups[type];

	PushState(GetStateManager());

	BindBufferVAO(GetStateManager(), pDebugRenderer->pDynamicGeometryBuffer);
	BindShader(GetStateManager(), pDebugRenderer->pShader);

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
		Matrix4 modelMatrix = group->modelsMetrices[i];
		SetMat4(pDebugRenderer->pShader, "u_matModel", modelMatrix);

		// Draw this mesh
		glDrawElementsBaseVertex(
			group->primitiveType,
			(GLsizei)mesh->indexCount,
			GL_UNSIGNED_INT,
			(void*)(mesh->indexOffset * sizeof(GLuint)),  // Index offset
			(GLint)mesh->vertexOffset                      // Vertex offset
		);
	}

	PopState(GetStateManager());
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

void SetRenderColor(DebugRenderer pDebugRenderer, Vector4 v4Color)
{
	pDebugRenderer->v4DiffuseColor = v4Color;
}
