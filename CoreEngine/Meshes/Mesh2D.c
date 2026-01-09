#include "Mesh2D.h"

Mesh2D Mesh2D_Create(GLenum primitiveType)
{
	// 1. Allocate and zero-initialize the struct
	Mesh2D mesh = (Mesh2D)tracked_calloc(1, sizeof(SMesh2D));
	if (!mesh)
	{
		syserr("Failed to allocate Mesh2D");
		return NULL;
	}

	// 2. Initialize dynamic arrays (Vector)
	mesh->pVertices = VectorInitSize(sizeof(SVertex), 2);  // Initial capacity: 2 vertices -- for line
	mesh->pIndices = VectorInitSize(sizeof(GLuint), 2);    // Initial capacity: 2 indices -- for line

	if (!mesh->pVertices || !mesh->pIndices)
	{
		syserr("Failed to create vertex/index vectors");
		Mesh2D_Destroy(&mesh);  // Cleanup on failure
		return NULL;
	}

	// 3. Initialize transform to identity
	mesh->transform = TransformInit();  // Position (0,0,0), Scale (1,1,1), No rotation

	// 4. Set render settings
	mesh->primitiveType = primitiveType;
	mesh->vertexCount = 0;
	mesh->indexCount = 0;
	mesh->bDirty = false;

	return mesh;
}

/**
 * @brief Creates a mesh with custom initial capacity hint.
 *
 * Use this when you know the approximate size in advance
 * to avoid multiple reallocations.
 */
Mesh2D Mesh2D_CreateWithCapacity(GLenum primitiveType, size_t vertexHint, size_t indexHint)
{
	// 1. Allocate and zero-initialize the struct
	Mesh2D mesh = (Mesh2D)tracked_calloc(1, sizeof(SMesh2D));
	if (!mesh)
	{
		syserr("Failed to allocate Mesh2D");
		return NULL;
	}

	// 2. Initialize dynamic arrays (Vector)
	mesh->pVertices = VectorInitSize(sizeof(SVertex), vertexHint);  // Initial capacity
	mesh->pIndices = VectorInitSize(sizeof(GLuint), indexHint);    // Initial capacity

	if (!mesh->pVertices || !mesh->pIndices)
	{
		syserr("Failed to create vertex/index vectors");
		Mesh2D_Destroy(&mesh);  // Cleanup on failure
		return NULL;
	}

	// 3. Initialize transform to identity
	mesh->transform = TransformInit();  // Position (0,0,0), Scale (1,1,1), No rotation

	// 4. Set render settings
	mesh->primitiveType = primitiveType;
	mesh->vertexCount = 0;
	mesh->indexCount = 0;
	mesh->bDirty = false;

	return mesh;
}

/**
 * @brief Creates a 3D line segment from start to end point.
 *
 * Generates a line using GL_LINES primitive. Note that line width
 * is driver-dependent and may be clamped to 1 pixel.
 *
 * @param pMesh [in/out] The mesh to populate with line geometry.
 * @param start [in] Starting point (world space).
 * @param end [in] Ending point (world space).
 * @param color [in] Line color (RGBA).
 *
 * @note For consistent line width across all platforms, use Mesh2D_MakeLineQuad instead.
 */
void Mesh2D_MakeLine3D(Mesh2D pMesh, Vector3 start, Vector3 end, Vector4 color)
{
	if (!pMesh)
	{
		syserr("Cannot create line in NULL mesh");
		return;
	}

	VectorClear(pMesh->pVertices);
	VectorClear(pMesh->pIndices);

	SVertex startVertex = { 0 };
	startVertex.m_v3Position = start;
	startVertex.m_v4Color = color;

	SVertex endVertex = { 0 };
	endVertex.m_v3Position = end;
	endVertex.m_v4Color = color;

	// Initialize Vertices
	ANUBIS_VECTOR_PUSH(pMesh->pVertices, SVertex, startVertex);
	ANUBIS_VECTOR_PUSH(pMesh->pVertices, SVertex, endVertex);

	// Initialize Indices
	ANUBIS_VECTOR_PUSH(pMesh->pIndices, GLuint, 0);
	ANUBIS_VECTOR_PUSH(pMesh->pIndices, GLuint, 1);

	// Update metadata
	pMesh->vertexCount = 2;
	pMesh->indexCount = 2;
	pMesh->primitiveType = GL_LINES;	// Render Type
	pMesh->bDirty = true;				// Needs GPU upload
}

/**
 * @brief Destroys a mesh and frees all resources.
 *
 * Cleans up dynamic arrays, GPU buffers, and the mesh structure itself.
 *
 * @param ppMesh [in/out] Pointer to mesh pointer (set to NULL after destruction).
 */
void Mesh2D_Destroy(Mesh2D* ppMesh)
{
	if (!ppMesh || !*ppMesh)
	{
		return;
	}

	Mesh2D mesh = *ppMesh;

	// 1. Free dynamic arrays
	if (mesh->pVertices)
	{
		VectorFree(&mesh->pVertices);
	}
	if (mesh->pIndices)
	{
		VectorFree(&mesh->pIndices);
	}

	// 2. Free the struct itself
	tracked_free(mesh);

	// 3. Set pointer to NULL (prevent double-free)
	*ppMesh = NULL;
}