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
    Vector_InitCapacity(&mesh->pVertices, sizeof(STerrainVertex), vertexHint);  // Initial capacity
    Vector_InitCapacity(&mesh->pIndices, sizeof(GLuint), indexHint);    // Initial capacity

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

void TerrainMesh_MakeQuad3D(TerrainMesh TerrainMesh, Vector3 topLeft, Vector3 topRight, Vector3 bottomLeft, Vector3 bottomRight, Vector4 color)
{
    GLuint baseOffset = (GLuint)TerrainMesh->pVertices->count;

    // Add 4 vertices
    STerrainVertex v0 = { .v3Position = topLeft, .v4Color = color };
    STerrainVertex v1 = { .v3Position = topRight, .v4Color = color };
    STerrainVertex v2 = { .v3Position = bottomLeft, .v4Color = color };
    STerrainVertex v3 = { .v3Position = bottomRight, .v4Color = color };

    // Calculate normal for the quad
    Vector3 edge1 = Vector3_Sub(topRight, topLeft);
    Vector3 edge2 = Vector3_Sub(bottomLeft, topLeft);
    Vector3 normal = Vector3_Normalized(Vector3_Cross(edge1, edge2));

    v0.v3Normals = normal;
    v1.v3Normals = normal;
    v2.v3Normals = normal;
    v3.v3Normals = normal;

    Vector_PushBack(&TerrainMesh->pVertices, &v0);
    Vector_PushBack(&TerrainMesh->pVertices, &v1);
    Vector_PushBack(&TerrainMesh->pVertices, &v2);
    Vector_PushBack(&TerrainMesh->pVertices, &v3);

    TerrainMesh->vertexCount += 4;

    // Add 6 indices (2 triangles)
    GLuint idx0 = baseOffset;
    GLuint idx1 = baseOffset + 1;
    GLuint idx2 = baseOffset + 2;
    GLuint idx3 = baseOffset + 3;

    // First triangle (counter-clockwise)
    Vector_PushBack(&TerrainMesh->pIndices, &idx0);  // topLeft
    Vector_PushBack(&TerrainMesh->pIndices, &idx2);  // bottomLeft
    Vector_PushBack(&TerrainMesh->pIndices, &idx1);  // topRight

    // Second triangle (counter-clockwise)
    Vector_PushBack(&TerrainMesh->pIndices, &idx1);  // topRight
    Vector_PushBack(&TerrainMesh->pIndices, &idx2);  // bottomLeft
    Vector_PushBack(&TerrainMesh->pIndices, &idx3);  // bottomRight

    TerrainMesh->indexCount += 6;
}

void TerrainMesh_AddVertex(TerrainMesh TerrainMesh, const STerrainVertex vertex)
{
    TerrainMesh->vertexCount += 1;

    Vector_PushBack(&TerrainMesh->pVertices, &vertex);
}

void TerrainMesh_AddIndex(TerrainMesh TerrainMesh, const GLuint index)
{
    TerrainMesh->indexCount += 1;

    Vector_PushBack(&TerrainMesh->pIndices, &index);
}