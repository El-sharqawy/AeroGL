#include "TerrainMesh.h"
#include <glad/glad.h>

TerrainMesh CreateTerrainPatch(int32_t index, int32_t width, int32_t depth, Vector3 worldPos, float cellSize)
{
	TerrainMesh patch = (TerrainMesh)tracked_malloc(sizeof(STerrainMesh));

    int32_t vertexCapacity = (width + 1) * (depth + 1);  // Grid vertices
    int32_t indexCapacity = width * depth * 6;            // 6 indices per quad

    patch->terrainMesh = Mesh3D_CreateWithCapacity(GL_TRIANGLES, vertexCapacity, indexCapacity);
    patch->patchWidth = width;
    patch->patchDepth = depth;
    patch->patchIndex = index;

    // Generate mesh geometry in LOCAL space (starting at 0,0,0)
    GenerateTerrainPatchGeometry(patch, Vector3F(0.0f), cellSize);

    // Position the mesh in world space
    TransformSetPositionV(&patch->terrainMesh->transform, worldPos);
    return patch;
}

void GenerateTerrainPatchGeometry(TerrainMesh patch, Vector3 localOrigin, float cellSize)
{
    Mesh3D mesh = patch->terrainMesh;
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
            Mesh3D_MakeQuad3D(mesh, topLeft, topRight, bottomLeft, bottomRight, Vector4D(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    syslog("Generated terrain: %zu vertices, %zu indices", mesh->vertexCount, mesh->indexCount);
}

void DestroyTerrainMesh(TerrainMesh* ppTerrainMesh)
{
    if (!ppTerrainMesh || !*ppTerrainMesh)
    {
        return;
    }

    Mesh3D_Destroy(&(*ppTerrainMesh)->terrainMesh);

    *ppTerrainMesh = NULL;
}
