#include "Mesh3D.h"
#include "../Math/EngineMath.h"

Mesh3D Mesh3D_Create(GLenum primitiveType)
{
	// 1. Allocate and zero-initialize the struct
	Mesh3D mesh = (Mesh3D)tracked_calloc(1, sizeof(SMesh3D));
	if (!mesh)
	{
		syserr("Failed to allocate Mesh3D");
		return NULL;
	}

	// 2. Initialize dynamic arrays (Vector)
	if (!Vector_Init(&mesh->pVertices, sizeof(SVertex3D)))
	{
		tracked_free(mesh);
		return NULL;
	}

	if (!Vector_Init(&mesh->pIndices, sizeof(GLuint)))
	{
		Vector_Destroy(&mesh->pVertices);
		tracked_free(mesh);
		return NULL;
	}

	// 3. Initialize transform to identity
	mesh->transform = TransformInit();  // Position (0,0,0), Scale (1,1,1), No rotation

	// 4. Set render settings
	mesh->primitiveType = primitiveType;
	mesh->vertexCount = 0;
	mesh->indexCount = 0;
	mesh->vertexOffset = 0;
	mesh->indexOffset = 0;
	mesh->bDirty = false;

	return mesh;
}

/**
 * @brief Creates a mesh with custom initial capacity hint.
 *
 * Use this when you know the approximate size in advance
 * to avoid multiple reallocations.
 */
Mesh3D Mesh3D_CreateWithCapacity(GLenum primitiveType, GLsizeiptr vertexHint, GLsizeiptr indexHint)
{
	// 1. Allocate and zero-initialize the struct
	Mesh3D mesh = (Mesh3D)tracked_calloc(1, sizeof(SMesh3D));
	if (!mesh)
	{
		syserr("Failed to allocate Mesh3D");
		return NULL;
	}

	// 2. Initialize dynamic arrays (Vector)
	Vector_InitCapacity(&mesh->pVertices, sizeof(SVertex3D), vertexHint);  // Initial capacity
	Vector_InitCapacity(&mesh->pIndices, sizeof(GLuint), indexHint);    // Initial capacity

	if (!mesh->pVertices || !mesh->pIndices)
	{
		syserr("Failed to create vertex/index vectors");
		Mesh3D_Destroy(&mesh);  // Cleanup on failure
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
void Mesh3D_AddLine3D(Mesh3D pMesh, Vector3 start, Vector3 end, Vector4 color)
{
	if (!pMesh)
	{
		syserr("Cannot create line in NULL mesh");
		return;
	}

	// Check if vectors are initialized
	if (!pMesh->pVertices)
	{
		syserr("Mesh has NULL vertices vector!");
		return;
	}

	if (!pMesh->pIndices)
	{
		syserr("Mesh has NULL indices vector!");
		return;
	}

	if (pMesh->primitiveType != GL_LINES)
	{
		return;
	}

	GLuint offset = (GLuint)pMesh->pVertices->count;

	SVertex3D startVertex = { .m_v3Position = start, .m_v4Color = color  };
	SVertex3D endVertex = { .m_v3Position = end, .m_v4Color = color };

	// Initialize Vertices
	if (!Vector_PushBack(&pMesh->pVertices, &startVertex))
	{
		syserr("Failed to push start vertex");
		return;
	}

	if (!Vector_PushBack(&pMesh->pVertices, &endVertex))
	{
		syserr("Failed to push end vertex");
		return;
	}

	// Initialize Indices
	GLuint index1 = offset;
	GLuint index2 = offset + 1;

	if (!Vector_PushBack(&pMesh->pIndices, &index1))
	{
		syserr("Failed to push index 1");
		return;
	}

	if (!Vector_PushBack(&pMesh->pIndices, &index2))
	{
		syserr("Failed to push index 2");
		return;
	}

	// Update metadata
	pMesh->vertexCount += 2;
	pMesh->indexCount += 2;
	pMesh->bDirty = true;				// Needs GPU upload
}

void Mesh3D_MakeAxis(Mesh3D pMesh, Vector3 position, float length)
{
	if (!pMesh)
	{
		syserr("Cannot create line in NULL mesh");
		return;
	}

	if (pMesh->primitiveType != GL_LINES)
	{
		return;
	}

	Mesh3D_AddLine3D(pMesh, Vector3F(0.0f), Vector3D(length, 0.0f, 0.0f), Vector4D(1.0f, 0.0f, 0.0f, 0.0f)); // X - Axis
	Mesh3D_AddLine3D(pMesh, Vector3F(0.0f), Vector3D(0.0f, length, 0.0f), Vector4D(0.0f, 1.0f, 0.0f, 0.0f)); // Y - Axis
	Mesh3D_AddLine3D(pMesh, Vector3F(0.0f), Vector3D(0.0f, 0.0f, length), Vector4D(0.0f, 0.0f, 1.0f, 0.0f)); // Z - Axis

	// Update metadata
	TransformSetPositionV(&pMesh->transform, position); // Update Transformation Matrix
	pMesh->vertexCount = pMesh->pVertices->count;
	pMesh->indexCount = pMesh->pIndices->count;
	pMesh->bDirty = true;				// Needs GPU upload
}

void Mesh3D_MakeCircle2D(Mesh3D pMesh, Vector3 center, float radius, int step, Vector4 color, bool bHorizonal)
{
	if (!pMesh)
	{
		return;
	}

	if (pMesh->primitiveType != GL_LINES)
	{
		return;
	}

	float Theta = 0.0f;
	float Delta = 2.0f * (float)M_PI / (float)step;

	float startX = center.x + radius * cosf(Theta);
	float startY = center.y;
	float startZ = center.z + radius * sinf(Theta); // sinf(0) = 0

	Vector3 startPoint;
	if (bHorizonal)
	{
		startPoint = Vector3D(startX, startY, startZ);
	}
	else
	{
		startPoint = Vector3D(startX, startZ, startY);
	}
	Vector3 currentPoint;

	for (int i = 1; i <= step; i++)
	{
		Theta = i * Delta;

		float x = center.x + radius * cosf(Theta);
		float y = center.y;
		float z = center.z + radius * sinf(Theta);

		if (bHorizonal)
		{
			currentPoint = Vector3D(x, y, z);
		}
		else
		{
			currentPoint = Vector3D(x, z, y);
		}

		Mesh3D_AddLine3D(pMesh, startPoint, currentPoint, color);
		startPoint = currentPoint;
	}
}

void Mesh3D_MakeWireSphere3D(Mesh3D pMesh, Vector3 center, float radius, int segments, int slices, Vector4 color, bool drawHorizontal)
{
	// 1. Generate Vertices (Poles and Rings)
	if (drawHorizontal)
	{
		for (int i = 0; i <= segments; ++i) // Iterate through stacks (latitude) - iSegments
		{
			float phi = (GLfloat)(i) / (GLfloat)(segments) * (GLfloat)(M_PI); // angle phi from 0 to PI
			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			Vector3 prevPos = { 0 }; // Store the previous position to draw lines

			for (GLint j = 0; j <= slices; ++j) // Iterate through slices (longitude)
			{
				float theta = (GLfloat)(j) / (GLfloat)(slices) * (2.0f * (GLfloat)(M_PI)); // angle theta from 0 to 2*PI
				float sinTheta = sinf(theta);
				float cosTheta = cosf(theta);

				// Calculate 3D coordinates
				float x = radius * cosTheta * sinPhi;
				float y = radius * sinTheta * sinPhi;
				float z = radius * cosPhi;

				Vector3 currentPos = Vector3D(center.x + x, center.y + y, center.y + z);

				if (j > 0)
				{
					// Draw longitude line segments (around the stack/ring)
					Mesh3D_AddLine3D(pMesh, prevPos, currentPos, color);
				}
				prevPos = currentPos;
			}
		}
	}
	else
	{
		// Generate Vertices (Swapped: phi=longitude -> vertical, theta=latitude -> horizontal slices)
		for (int i = 0; i <= segments; ++i) // Iterate through stacks (latitude) - iSegments
		{
			float theta = (GLfloat)(i) / (GLfloat)(segments) * (GLfloat)(M_PI); // angle phi from 0 to PI
			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			Vector3 prevPos = { 0 }; // Store the previous position to draw lines

			for (GLint j = 0; j <= slices; ++j) // Iterate through slices (longitude)
			{
				float phi = (GLfloat)(j) / (GLfloat)(slices) * (2.0f * (GLfloat)(M_PI)); // angle theta from 0 to 2*PI
				float sinPhi = sinf(phi);
				float cosPhi = cosf(phi);

				// Calculate 3D coordinates
				float x = radius * cosPhi * sinTheta;
				float y = radius * cosTheta;                    // Was Z
				float z = radius * sinPhi * sinTheta;           // Was -Y (vertical now)

				Vector3 currentPos = Vector3D(center.x + x, center.y + y, center.y + z);

				if (j > 0)
				{
					// Draw longitude line segments (around the stack/ring)
					Mesh3D_AddLine3D(pMesh, prevPos, currentPos, color);
				}
				prevPos = currentPos;
			}
		}
	}
}

void Mesh3D_MakeTriangle3D(Mesh3D pMesh, Vector3 p1, Vector3 p2, Vector3 p3, Vector4 color)
{
	GLuint baseOffset = (GLuint)pMesh->pVertices->count;
	GLuint baseOffset1 = (GLuint)pMesh->pVertices->count + 1;
	GLuint baseOffset2 = (GLuint)pMesh->pVertices->count + 2;

	SVertex3D vertex1 = { .m_v3Position = p1, .m_v4Color = color };
	SVertex3D vertex2 = { .m_v3Position = p2, .m_v4Color = color };
	SVertex3D vertex3 = { .m_v3Position = p3, .m_v4Color = color };

	Vector_PushBack(&pMesh->pVertices, &vertex1);
	Vector_PushBack(&pMesh->pVertices, &vertex2);
	Vector_PushBack(&pMesh->pVertices, &vertex3);

	pMesh->vertexCount += 3;

	Vector_PushBack(&pMesh->pIndices, &baseOffset);
	Vector_PushBack(&pMesh->pIndices, &baseOffset1);
	Vector_PushBack(&pMesh->pIndices, &baseOffset2); // Triangle Face

	pMesh->indexCount += 3;
}

void Mesh3D_MakeSphere3D(Mesh3D pMesh, Vector3 center, float radius, int segments, int slices, Vector4 color)
{
	// Generate Vertices (Poles and Rings)
	for (int i = 0; i < segments; ++i) // Iterate through stacks (latitude) - iSegments
	{
		float phi1 = (float)(i) / (float)(segments) * (float)(M_PI); // angle phi from 0 to PI
		float phi2 = (float)(i + 1) / (float)(segments) * (float)(M_PI); // angle phi from 0 to PI
		
		Vector3 prevPos = { 0 }; // Store the previous position to draw lines
		for (int j = 0; j < slices; ++j) // Iterate through slices (longitude)
		{
			float theta1 = (float)(j) / (float)(slices) * (2.0f * (float)(M_PI)); // angle theta from 0 to 2*PI
			float theta2 = (float)(j + 1) / (float)(slices) * (2.0f * (float)(M_PI)); // angle theta from 0 to 2*PI

			// Calculate the 4 corners of the "quad" patch
			Vector3 v1 = GetSpherePos(center.x, center.y, center.z, radius, phi1, theta1);
			Vector3 v2 = GetSpherePos(center.x, center.y, center.z, radius, phi1, theta2);
			Vector3 v3 = GetSpherePos(center.x, center.y, center.z, radius, phi2, theta1);
			Vector3 v4 = GetSpherePos(center.x, center.y, center.z, radius, phi2, theta2);

			// Draw two triangles to form the solid face
			Mesh3D_MakeTriangle3D(pMesh, v1, v2, v3, color);
			Mesh3D_MakeTriangle3D(pMesh, v2, v4, v3, color);
		}
	}

	syslog("MakeSphere: NumVertices: %d NumIndices: %d", (GLuint)pMesh->pVertices->count, (GLuint)pMesh->pIndices->count)
}

void Mesh3D_MakePyramid(Mesh3D pMesh, float baseSize, float height, Vector4 baseColor, Vector4 sideColor)
{
	float hs = baseSize * 0.5f;
	SVertex3D base_sw;
	base_sw.m_v3Position = Vector3D(-hs, -hs, 0);
	base_sw.m_v4Color = baseColor;

	SVertex3D base_se;
	base_se.m_v3Position = Vector3D(hs, -hs, 0);
	base_se.m_v4Color = baseColor;

	SVertex3D base_ne;
	base_ne.m_v3Position = Vector3D(hs, hs, 0);
	base_ne.m_v4Color = baseColor;

	SVertex3D base_nw;
	base_nw.m_v3Position = Vector3D(-hs, hs, 0);
	base_nw.m_v4Color = baseColor;

	SVertex3D apex;
	apex.m_v3Position = Vector3D(0, 0, height);
	apex.m_v4Color = sideColor;

	// Vertex 0: base_sw
	Vector_PushBack(&pMesh->pVertices, &base_sw);
	// Vertex 1: base_se  
	Vector_PushBack(&pMesh->pVertices, &base_se);
	// Vertex 2: base_ne
	Vector_PushBack(&pMesh->pVertices, &base_ne);
	// Vertex 3: base_nw
	Vector_PushBack(&pMesh->pVertices, &base_nw);
	// Vertex 4: apex
	Vector_PushBack(&pMesh->pVertices, &apex);

	// 4 side faces (CCW triangles, front-facing)
	// Front: base_sw(0), base_se(1), apex(4)
	VECTOR_PUSH(pMesh->pIndices, 0);
	VECTOR_PUSH(pMesh->pIndices, 1);
	VECTOR_PUSH(pMesh->pIndices, 4);
	// Right: base_se(1), base_ne(2), apex(4)
	VECTOR_PUSH(pMesh->pIndices, 1);
	VECTOR_PUSH(pMesh->pIndices, 2);
	VECTOR_PUSH(pMesh->pIndices, 4);
	// Back: base_ne(2), base_nw(3), apex(4) 
	VECTOR_PUSH(pMesh->pIndices, 2);
	VECTOR_PUSH(pMesh->pIndices, 3);
	VECTOR_PUSH(pMesh->pIndices, 4);
	// Left: base_nw(3), base_sw(0), apex(4)
	VECTOR_PUSH(pMesh->pIndices, 3);
	VECTOR_PUSH(pMesh->pIndices, 0);
	VECTOR_PUSH(pMesh->pIndices, 4);
}

void Mesh3D_MakeQuad3D(Mesh3D pMesh, Vector3 topLeft, Vector3 topRight, Vector3 bottomLeft, Vector3 bottomRight, Vector4 color)
{
	GLuint baseOffset = (GLuint)pMesh->pVertices->count;

	// Add 4 vertices
	SVertex3D v0 = { .m_v3Position = topLeft, .m_v4Color = color };
	SVertex3D v1 = { .m_v3Position = topRight, .m_v4Color = color };
	SVertex3D v2 = { .m_v3Position = bottomLeft, .m_v4Color = color };
	SVertex3D v3 = { .m_v3Position = bottomRight, .m_v4Color = color };

	// Calculate normal for the quad
	Vector3 edge1 = Vector3_Sub(topRight, topLeft);
	Vector3 edge2 = Vector3_Sub(bottomLeft, topLeft);
	Vector3 normal = Vector3_Normalized(Vector3_Cross(edge1, edge2));

	// v0.m_v3Normal = normal;
	// v1.m_v3Normal = normal;
	// v2.m_v3Normal = normal;
	// v3.m_v3Normal = normal;

	Vector_PushBack(&pMesh->pVertices, &v0);
	Vector_PushBack(&pMesh->pVertices, &v1);
	Vector_PushBack(&pMesh->pVertices, &v2);
	Vector_PushBack(&pMesh->pVertices, &v3);

	pMesh->vertexCount += 4;

	// Add 6 indices (2 triangles)
	GLuint idx0 = baseOffset;
	GLuint idx1 = baseOffset + 1;
	GLuint idx2 = baseOffset + 2;
	GLuint idx3 = baseOffset + 3;

	// First triangle (counter-clockwise)
	Vector_PushBack(&pMesh->pIndices, &idx0);  // topLeft
	Vector_PushBack(&pMesh->pIndices, &idx2);  // bottomLeft
	Vector_PushBack(&pMesh->pIndices, &idx1);  // topRight

	// Second triangle (counter-clockwise)
	Vector_PushBack(&pMesh->pIndices, &idx1);  // topRight
	Vector_PushBack(&pMesh->pIndices, &idx2);  // bottomLeft
	Vector_PushBack(&pMesh->pIndices, &idx3);  // bottomRight

	pMesh->indexCount += 6;
}

/**
 * @brief Destroys a mesh and frees all resources.
 *
 * Cleans up dynamic arrays, GPU buffers, and the mesh structure itself.
 *
 * @param ppMesh [in/out] Pointer to mesh pointer (set to NULL after destruction).
 */
void Mesh3D_Destroy(Mesh3D* ppMesh)
{
	if (!ppMesh || !*ppMesh)
	{
		return;
	}

	Mesh3D mesh = *ppMesh;

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

/**
 * @brief Destroys a mesh and frees all resources.
 *
 * Cleans up dynamic arrays, GPU buffers, and the mesh structure itself.
 *
 * @param ppMesh [in/out] Pointer to mesh pointer (set to NULL after destruction).
 */
void Mesh3D_Free(Mesh3D pMesh)
{
	if (!pMesh)
	{
		return;
	}

	// 1. Free dynamic arrays
	if (pMesh->pVertices)
	{
		Vector_Destroy(&pMesh->pVertices);
	}

	if (pMesh->pIndices)
	{
		Vector_Destroy(&pMesh->pIndices);
	}

	// 2. Free the struct itself
	tracked_free(pMesh);
}

void Mesh3D_PtrDestroy(void** elem)
{
	Mesh3D* pMesh = *(Mesh3D**)elem;  // Deref void* -> Mesh3D*
	if (pMesh)
	{
		Mesh3D_Destroy(pMesh);  // Pass address of local pointer
	}
}
