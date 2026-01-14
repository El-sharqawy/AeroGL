#ifndef __TERRAIN_MESH__
#define __TERRAIN_MESH__

#include "Mesh3D.h"

typedef struct STerrainMesh
{
	Mesh3D terrainMesh;

	// Patch dimensions
	int32_t patchWidth; // Number of squares in the X direction
	int32_t patchDepth; // Number of squares in the Z direction
	int32_t patchIndex; // Unique index for this patch

	// Terrain-specific data
	Vector3 worldPosition;   // Position in world space (X, Z)
	float cellSize;          // Size of each quad cell
	float minHeight;         // For culling optimization
	float maxHeight;
} STerrainMesh;

typedef struct STerrainMesh* TerrainMesh;

TerrainMesh CreateTerrainPatch(int32_t index, int32_t width, int32_t depth, Vector3 worldPos, float cellSize);
void GenerateTerrainPatchGeometry(TerrainMesh patch, Vector3 worldPos, float cellSize);
void DestroyTerrainMesh(TerrainMesh* ppTerrainMesh);

#endif // __TERRAIN_MESH__