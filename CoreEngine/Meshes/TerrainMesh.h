#ifndef __TERRAIN_MESH__
#define __TERRAIN_MESH__

#include <glad/glad.h>
#include "../Math/Transform.h"
#include "../Math/Vectors/Vector2.h"
#include "../Math/Vectors/Vector3.h"
#include "../Math/Vectors/Vector4.h"
#include "../Lib/Vector.h"

typedef struct STerrainVertex
{
	Vector3 v3Position;		// World position
	Vector3 v3Normals;		// Normals
	Vector2 v2TexCoords;	// UVs (For Texturing)
	Vector4 v4Color;		// Color
} STerrainVertex;

typedef struct STerrainMesh
{
	// Mesh Transformation (pos, scale, orientation)
	STransform transform;

	// Render settings
	GLenum primitiveType;	// GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.

	// Cache counts for quick access
	// (avoids dereferencing Vector->count repeatedly)
	GLsizeiptr vertexCount;		// Cached pVertices->count
	GLsizeiptr indexCount;		// Cached pIndices->count

	GLsizeiptr vertexOffset;        // Where this mesh starts in shared VBO (in vertices)
	GLsizeiptr indexOffset;         // Where this mesh starts in shared EBO (in indices)

	bool bDirty;			// True when vertices/indices modified, needs GPU upload
	int32_t meshMatrixIndex;

	// Shape Data
	Vector pVertices;
	Vector pIndices;
} STerrainMesh;

typedef struct STerrainMesh* TerrainMesh;

/**
 * @brief Creates a 3D mesh with reasonable initial capacity.
 *
 * Starts with small capacity (2 vertices, 2 indices) which is
 * sufficient for simple shapes (quads, triangles). Automatically
 * grows as needed for more complex geometry.
 */
TerrainMesh TerrainMesh_Create(GLenum primitiveType);

/**
 * @brief Creates a mesh with custom initial capacity hint.
 *
 * Use this when you know the approximate size in advance
 * to avoid multiple reallocations.
 */
TerrainMesh TerrainMesh_CreateWithCapacity(GLenum primitiveType, GLsizeiptr vertexHint, GLsizeiptr indexHint);
void TerrainMesh_Destroy(TerrainMesh* ppMesh);
void TerrainMesh_PtrDestroy(TerrainMesh pTerrainMesh);

void TerrainMesh_AddVertex(TerrainMesh TerrainMesh, const STerrainVertex vertex);
void TerrainMesh_AddIndex(TerrainMesh TerrainMesh, const GLuint index);

#endif // __TERRAIN_MESH__