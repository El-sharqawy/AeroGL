#include "TerrainPatch.h"
#include "TerrainData.h"
#include "Terrain/Terrain.h"

TerrainPatch TerrainPatch_Create(struct STerrain* pParentTerrain, int32_t index, int32_t patchX, int32_t patchZ, Vector3 worldPos, float cellSize)
{
    TerrainPatch patch = (TerrainPatch)tracked_malloc(sizeof(STerrainPatch));

    patch->patchWidth = PATCH_XSIZE;
    patch->patchDepth = PATCH_ZSIZE;
    patch->patchIndex = index;
    patch->pParentTerrain = pParentTerrain;

    int32_t vertexCapacity = (patch->patchWidth + 1) * (patch->patchDepth + 1);  // Grid vertices
    int32_t indexCapacity = patch->patchWidth * patch->patchDepth * 6;            // 6 indices per quad
    patch->terrainMesh = TerrainMesh_CreateWithCapacity(GL_TRIANGLES, vertexCapacity, indexCapacity);

    // Generate mesh geometry in LOCAL space (starting at 0,0,0)
    Vector4 color;
    color.r = (float)(patch->patchIndex % 8) / 8.0f;      // Red: 0,0.125,0.25...
    color.g = (float)(patch->patchIndex / 8 % 8) / 8.0f;  // Green: cycles every 8
    color.b = 0.5f + 0.5f * sinf(patch->patchIndex * 0.3f);  // Blue: rainbow
    color.a = 1.0f;

    TerrainPatch_GenerateGeometry(patch, patchX, patchZ, cellSize, color);

    patch->terrainMesh->transform = TransformInit();

    // Position the mesh in world space
    TransformSetPositionV(&patch->terrainMesh->transform, worldPos);

    // syslog("Generated terrain %d: %zu vertices, %zu indices", patch->patchIndex, patch->terrainMesh->vertexCount, patch->terrainMesh->indexCount);
    return patch;
}

void TerrainPatch_GenerateGeometry(TerrainPatch patch, int32_t patchX, int32_t patchZ, float cellSize, Vector4 color)
{
    TerrainMesh mesh = patch->terrainMesh;
    int32_t width = patch->patchWidth;
    int32_t depth = patch->patchDepth;

    // Clear any existing geometry
    Vector_Clear(mesh->pVertices);
    Vector_Clear(mesh->pIndices);

    Vector_Reserve(mesh->pVertices, (width + 1) * (depth + 1));
    Vector_Reserve(mesh->pIndices, (width * depth * 6));

    mesh->vertexCount = 0;
    mesh->indexCount = 0;

    // Generate Vertices
    // We go to <= width/depth because a 16x16 square grid needs 17x17 vertices
    for (int32_t iZ = 0; iZ <= patch->patchDepth; iZ++)
    {
        for (int32_t iX = 0; iX <= patch->patchWidth; iX++)
        {
            STerrainVertex v = { 0 };

            // Calculate Global coordinates for the heightmap
            int32_t gx = (patchX * patch->patchWidth) + iX;
            int32_t gz = (patchZ * patch->patchDepth) + iZ;

            // Height lookup (using your +1 padding offset)
            float height = GetHeightMapValue(patch->pParentTerrain, gx + 1, gz + 1);
            // syslog("gx: %d, gz: %d", gx, gz);

            v.v3Position = Vector3D(iX * cellSize, height, iZ * cellSize);
            v.v2TexCoords = Vector2D((float)iX / patch->patchWidth, (float)iZ / patch->patchDepth);
            v.v3Normals = Vector3D(0, 1, 0); // Placeholder until you calculate real normals
            v.v4Color = color;

            TerrainMesh_AddVertex(mesh, v);
        }
    }

    // 2. GENERATE INDICES (The "Wiring")
    int32_t verticesPerRow = patch->patchWidth + 1;
    for (int32_t z = 0; z < patch->patchDepth; z++)
    {
        for (int32_t x = 0; x < patch->patchWidth; x++)
        {
            // Calculate indices for the 4 corners of the current quad
            GLuint topLeft = (z * verticesPerRow) + x;
            GLuint topRight = topLeft + 1;
            GLuint bottomLeft = ((z + 1) * verticesPerRow) + x;
            GLuint bottomRight = bottomLeft + 1;

            // Triangle 1 (Clockwise or Counter-Clockwise depending on your GL setup)
            TerrainMesh_AddIndex(mesh, topLeft);
            TerrainMesh_AddIndex(mesh, bottomLeft);
            TerrainMesh_AddIndex(mesh, topRight);

            // Triangle 2
            TerrainMesh_AddIndex(mesh, topRight);
            TerrainMesh_AddIndex(mesh, bottomLeft);
            TerrainMesh_AddIndex(mesh, bottomRight);
        }
    }
}

void TerrainPatch_Destroy(TerrainPatch* ppTerrainPatch)
{
    if (!ppTerrainPatch || !*ppTerrainPatch)
    {
        return;
    }

    TerrainPatch patch = *ppTerrainPatch;

    TerrainMesh_Destroy(&patch->terrainMesh);

    tracked_free(patch);

    *ppTerrainPatch = NULL;
}

void TerrainPatch_DestroyPtr(TerrainPatch elem)
{
    TerrainPatch* pMesh = *(TerrainPatch**)elem;  // Deref void* -> TerrainMesh*
    if (pMesh)
    {
        TerrainPatch_Destroy(pMesh);  // Pass address of local pointer
    }
}
