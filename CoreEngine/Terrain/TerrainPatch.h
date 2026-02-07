#ifndef __TERRAIN_PATCH__
#define __TERRAIN_PATCH__

#include "../Lib/Vector.h"
#include "../Meshes/TerrainMesh.h"

typedef struct STerrainPatch
{
	TerrainMesh terrainMesh;

	// Patch dimensions
	int32_t patchWidth; // Number of squares in the X direction
	int32_t patchDepth; // Number of squares in the Z direction
	int32_t patchIndex; // Unique index for this patch

	// Terrain-specific data
	Vector3 worldPosition;   // Position in world space (X, Z)
	float cellSize;          // Size of each quad cell
	float minHeight;         // For culling optimization
	float maxHeight;

	GLsizeiptr patchVerticesOffset;		// this patch vertices offset in OpenGL Buffer (from x to y)
	GLsizeiptr patchIndicesOffset;		// this patch indices offset in OpenGL Buffer (from x to y)

	struct STerrain* pParentTerrain;
} STerrainPatch;

typedef struct STerrainPatch* TerrainPatch;

bool TerrainPatch_Initialize(TerrainPatch* ppTerrainPatch, struct STerrain* pParentTerrain, int32_t index);
void TerrainPatch_Destroy(TerrainPatch* ppTerrainPatch);
void TerrainPatch_DestroyPtr(TerrainPatch elem);
void TerrainPatch_Clear(TerrainPatch pTerrainPatch);
bool TerrainPatch_InitializeIndices(TerrainPatch pTerrainPatch);

void TerrainPatch_GenerateGeometry(TerrainPatch patch, int32_t patchX, int32_t patchZ, float cellSize, Vector4 color);


#endif // __TERRAIN_PATCH__