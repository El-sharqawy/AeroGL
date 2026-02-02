#include "TerrainMesh.h"

/**
 * @brief Creates a 3D mesh with reasonable initial capacity.
 *
 * Starts with small capacity (2 vertices, 2 indices) which is
 * sufficient for simple shapes (quads, triangles). Automatically
 * grows as needed for more complex geometry.
 */
TerrainMesh TerrainMesh_Create(GLenum primitiveType)
{
	return TerrainMesh_CreateWithCapacity(primitiveType, 4, 6); // 4 vertices, 6 indices for a quad
}

/**
 * @brief Creates a mesh with custom initial capacity hint.
 *
 * Use this when you know the approximate size in advance
 * to avoid multiple reallocations.
 */
TerrainMesh TerrainMesh_CreateWithCapacity(GLenum primitiveType, GLsizeiptr vertexHint, GLsizeiptr indexHint)
{
    // 1. Allocate and zero-initialize the struct
    TerrainMesh mesh = (TerrainMesh)tracked_calloc(1, sizeof(STerrainMesh));
    if (!mesh)
    {
        syserr("Failed to allocate TerrainMesh");
        return NULL;
    }

    // 2. Initialize dynamic arrays (Vector)
    Vector_InitCapacity(&mesh->pVertices, sizeof(STerrainVertex), vertexHint, false);  // Initial capacity
    Vector_InitCapacity(&mesh->pIndices, sizeof(GLuint), indexHint, false);    // Initial capacity

    if (!mesh->pVertices || !mesh->pIndices)
    {
        syserr("Failed to create vertex/index vectors");
        TerrainMesh_Destroy(&mesh);  // Cleanup on failure
        return NULL;
    }

    // 3. Initialize transform to identity
    mesh->transform = TransformInit();  // Position (0,0,0), Scale (1,1,1), No rotation

    // 4. Set render settings
    mesh->primitiveType = primitiveType;
    mesh->vertexCount = 0;
    mesh->indexCount = 0;
    mesh->vertexOffset = 0xFFFFFFFFFFFF;
    mesh->indexOffset = 0xFFFFFFFFFFFF;
    mesh->bDirty = false;

    return mesh;
}

void TerrainMesh_Destroy(TerrainMesh* ppMesh)
{
    if (!ppMesh || !*ppMesh)
    {
        return;
    }

    TerrainMesh mesh = *ppMesh;

    // 1. Free dynamic arrays
    if (mesh->pVertices)
    {
        Vector_Destroy(&mesh->pVertices);
    }
    if (mesh->pIndices)
    {
        Vector_Destroy(&mesh->pIndices);
    }

    // 2. Free the struct itself
    tracked_free(mesh);

    // 3. Set pointer to NULL (prevent double-free)
    *ppMesh = NULL;
}

void TerrainMesh_PtrDestroy(TerrainMesh pTerrainMesh)
{
    TerrainMesh* pMesh = *(TerrainMesh**)pTerrainMesh;  // Deref void* -> TerrainMesh*
    if (pMesh)
    {
        TerrainMesh_Destroy(pMesh);  // Pass address of local pointer
    }
}

void TerrainMesh_AddVertex(TerrainMesh TerrainMesh, const STerrainVertex vertex)
{
    TerrainMesh->vertexCount += 1;

    Vector_PushBackValue(TerrainMesh->pVertices, vertex);
}

void TerrainMesh_AddIndex(TerrainMesh TerrainMesh, const GLuint index)
{
    TerrainMesh->indexCount += 1;

    Vector_PushBackValue(TerrainMesh->pIndices, index);
}