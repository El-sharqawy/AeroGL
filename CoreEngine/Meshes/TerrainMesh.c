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

void TerrainMesh_PtrDestroy(void* elem)
{
    TerrainMesh* pMesh = *(TerrainMesh**)elem;  // Deref void* -> TerrainMesh*
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

TerrainPatch CreateTerrainPatch(int32_t index, int32_t width, int32_t depth, Vector3 worldPos, float cellSize)
{
    TerrainPatch patch = (TerrainPatch)tracked_malloc(sizeof(STerrainPatch));

    int32_t vertexCapacity = (width + 1) * (depth + 1);  // Grid vertices
    int32_t indexCapacity = width * depth * 6;            // 6 indices per quad

    patch->terrainMesh = TerrainMesh_CreateWithCapacity(GL_TRIANGLES, vertexCapacity, indexCapacity);
    patch->patchWidth = width;
    patch->patchDepth = depth;
    patch->patchIndex = index;

    // Generate mesh geometry in LOCAL space (starting at 0,0,0)
    GenerateTerrainPatchGeometry(patch, Vector3F(0.0f), cellSize);

    // Position the mesh in world space
    TransformSetPositionV(&patch->terrainMesh->transform, worldPos);
    return patch;
}

void GenerateTerrainPatchGeometry(TerrainPatch patch, Vector3 localOrigin, float cellSize)
{
    TerrainMesh mesh = patch->terrainMesh;
    int32_t width = patch->patchWidth;
    int32_t depth = patch->patchDepth;

    // Clear any existing geometry
    Vector_Clear(mesh->pVertices);
    Vector_Clear(mesh->pIndices);
    mesh->vertexCount = 0;
    mesh->indexCount = 0;

    // Generate quads
    for (int32_t z = 0; z < depth; z++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            // Calculate corner positions
            Vector3 topLeft = Vector3D(localOrigin.x + (x * cellSize), localOrigin.y, localOrigin.z + (z * cellSize));

            Vector3 topRight = Vector3D(localOrigin.x + ((x + 1) * cellSize), localOrigin.y, localOrigin.z + (z * cellSize));

            Vector3 bottomLeft = Vector3D(localOrigin.x + (x * cellSize), localOrigin.y, localOrigin.z + ((z + 1) * cellSize));

            Vector3 bottomRight = Vector3D(localOrigin.x + ((x + 1) * cellSize), localOrigin.y, localOrigin.z + ((z + 1) * cellSize));

            // Create quad
            TerrainMesh_MakeQuad3D(mesh, topLeft, topRight, bottomLeft, bottomRight, Vector4D(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    syslog("Generated terrain: %zu vertices, %zu indices", mesh->vertexCount, mesh->indexCount);
}

void DestroyTerrainPatch(TerrainPatch* ppTerrainPatch)
{
    if (!ppTerrainPatch || !*ppTerrainPatch)
    {
        return;
    }

    TerrainMesh_Destroy(&(*ppTerrainPatch)->terrainMesh);

    *ppTerrainPatch = NULL;
}
