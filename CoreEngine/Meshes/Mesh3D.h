#ifndef __MESH_3D_H__
#define __MESH_3D_H__

#include "../Lib/Vector.h"
#include "../Renderer/Buffer.h"
#include "../Math/Transform.h"
#include <stdint.h>

typedef struct SVertex3D
{
	Vector3 m_v3Position;		// World position
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
	size_t vertexCount;		// Cached pVertices->count
	size_t indexCount;		// Cached pIndices->count
	bool bDirty;			// True when vertices/indices modified, needs GPU upload
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
Mesh3D Mesh3D_CreateWithCapacity(GLenum primitiveType, size_t vertexHint, size_t indexHint);

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
void Mesh3D_MakeLine3D(Mesh3D pMesh, Vector3 start, Vector3 end, Vector4 color);

/**
 * @brief Destroys a mesh and frees all resources.
 *
 * Cleans up dynamic arrays, GPU buffers, and the mesh structure itself.
 *
 * @param ppMesh [in/out] Pointer to mesh pointer (set to NULL after destruction).
 */
void Mesh3D_Destroy(Mesh3D* ppMesh);

#endif // __MESH_3D_H__