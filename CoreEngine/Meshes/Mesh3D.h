#ifndef __MESH_3D_H__
#define __MESH_3D_H__

#include "../Lib/Vector.h"
#include "../Math/Vectors/Vector2.h"
#include "../Math/Vectors/Vector3.h"
#include "../Math/Vectors/Vector4.h"
#include "../Math/Transform.h"
#include <stdint.h>

typedef struct SVertex3D
{
	Vector3 m_v3Position;		// World position
	Vector3 m_v3Normals;		// Surface Normals
	Vector2 m_v2TexCoords;		// UVs (For Texturing)
	Vector4 m_v4Color;			// Color
} SVertex3D;

typedef struct SMesh3D
{
	// Shape Data
	Vector pVertices;
	Vector pIndices;
	
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

	char* szMeshName;
	Vector4 meshColor;
	int32_t meshMatrixIndex;	// Mesh Index in the renderer (matrix array)
} SMesh3D;

typedef struct SMesh3D* Mesh3D;

/**
 * @brief Creates a 3D mesh with reasonable initial capacity.
 *
 * Starts with small capacity (2 vertices, 2 indices) which is
 * sufficient for simple shapes (quads, triangles). Automatically
 * grows as needed for more complex geometry.
 */
Mesh3D Mesh3D_Create(GLenum primitiveType);

/**
 * @brief Creates a mesh with custom initial capacity hint.
 *
 * Use this when you know the approximate size in advance
 * to avoid multiple reallocations.
 */
Mesh3D Mesh3D_CreateWithCapacity(GLenum primitiveType, GLsizeiptr vertexHint, GLsizeiptr indexHint);

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
 * @note For consistent line width across all platforms, use Mesh3D_MakeLineQuad instead.
 */
void Mesh3D_AddLine3D(Mesh3D pMesh, Vector3 start, Vector3 end, Vector4 color);
void Mesh3D_MakeAxis(Mesh3D pMesh, Vector3 position, float length);
void Mesh3D_MakeCircle2D(Mesh3D pMesh, Vector3 center, float radius, int step, Vector4 color, bool bHorizonal);
void Mesh3D_MakeWireSphere3D(Mesh3D pMesh, Vector3 center, float radius, int segments, int slices, Vector4 color, bool drawHorizontal);
void Mesh3D_MakeTriangle3D(Mesh3D pMesh, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 center, Vector4 color);
void Mesh3D_MakeSphere3D(Mesh3D pMesh, Vector3 center, float radius, int segments, int slices, Vector4 color);
void Mesh3D_MakeQuad3D(Mesh3D pMesh, Vector3 topLeft, Vector3 topRight, Vector3 bottomLeft, Vector3 bottomRight, Vector4 color);

void Mesh3D_SetName(Mesh3D pMesh, const char* szName);

/**
 * @brief Destroys a mesh and frees all resources.
 *
 * Cleans up dynamic arrays, GPU buffers, and the mesh structure itself.
 *
 * @param ppMesh [in/out] Pointer to mesh pointer (set to NULL after destruction).
 */
void Mesh3D_Destroy(Mesh3D* ppMesh);
void Mesh3D_Free(Mesh3D pMesh);

#endif // __MESH_3D_H__