#include "Mesh3D.h"
#include "../Math/EngineMath.h"
#include "../Math/Matrix/Matrix3.h"

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
	if (!Vector_Init(&mesh->pVertices, sizeof(SVertex3D), false))
	{
		syserr("Failed to Initialize Mesh Vector Vertices");
		tracked_free(mesh);
		return NULL;
	}

	if (!Vector_Init(&mesh->pIndices, sizeof(GLuint), false))
	{
		syserr("Failed to Initialize Mesh Vector Indices");
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
	if (!Vector_InitCapacity(&mesh->pVertices, sizeof(SVertex3D), vertexHint, false))  // Initial capacity
	{
		syserr("Failed to Initialize Mesh Vector Vertices");
		Mesh3D_Destroy(&mesh);  // Cleanup on failure
		return NULL;
	}
	if (!Vector_InitCapacity(&mesh->pIndices, sizeof(GLuint), indexHint, false))    // Initial capacity
	{
		syserr("Failed to Initialize Mesh Vector Indices");
		Mesh3D_Destroy(&mesh);  // Cleanup on failure
		return NULL;
	}

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

	SVertex3D startVertex = { .m_v3Position = start, .m_v4Color = color };
	SVertex3D endVertex = { .m_v3Position = end, .m_v4Color = color };

	// Initialize Vertices
	Vector_PushBackValue(pMesh->pVertices, startVertex);

	Vector_PushBackValue(pMesh->pVertices, endVertex);

	// Initialize Indices
	GLuint index1 = offset;
	GLuint index2 = offset + 1;

	Vector_PushBackValue(pMesh->pIndices, index1);
	Vector_PushBackValue(pMesh->pIndices, index2);

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

	TransformSetPositionV(&pMesh->transform, position); // Update Transformation Matrix

	// Update metadata
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

	pMesh->bDirty = true;				// Needs GPU upload
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

	pMesh->bDirty = true;				// Needs GPU upload
}

void Mesh3D_MakeTriangle3D(Mesh3D pMesh, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 center, Vector4 color)
{
	// 1. Get Transformation Matrices
	Matrix4 model = TransformGetMatrix(&pMesh->transform);
	Matrix3 mat3Model = Matrix3_InitMatrix4(model);
	Matrix3 invModel = Matrix3_Inverse(mat3Model);
	Matrix3 normalMatrix = Matrix3_TransposeN(invModel);

	GLuint baseOffset = (GLuint)pMesh->pVertices->count;

	// 2. Calculate the "Face Normal" (Flat Surface)
	// Edge vectors
	Vector3 e1 = Vector3_Sub(p2, p1);
	Vector3 e2 = Vector3_Sub(p3, p1);

	// Cross product gives the perpendicular direction
	Vector3 localNormal = Vector3_Normalized(Vector3_Cross(e1, e2));

	// 3. Transform Normal to World Space
	Vector3 worldNormal = Vector3_Normalized(Matrix3_Mul_Vec3(normalMatrix, localNormal));

	// 4. Generate and Push Vertices
	Vector3 positions[3] = { p1, p2, p3 };
	for (int i = 0; i < 3; i++)
	{
		SVertex3D vtx = { 0 };

		// Position: Transform by Model Matrix
		vtx.m_v3Position = Matrix4_Mul_Vec3(model, positions[i]);

		// Normal: Use the calculated flat worldNormal
		vtx.m_v3Normals = worldNormal;

		vtx.m_v2TexCoords = (Vector2){ 0.0f, 0.0f }; // Or custom UVs
		vtx.m_v4Color = color;

		Vector_PushBackValue(pMesh->pVertices, vtx);
	}

	// 5. Generate and Push Indices
	for (GLuint i = 0; i < 3; i++)
	{
		GLuint idx = baseOffset + i;
		Vector_PushBackValue(pMesh->pIndices, idx);
	}

	// Sync metadata
	pMesh->vertexCount = (GLuint)pMesh->pVertices->count;
	pMesh->indexCount = (GLuint)pMesh->pIndices->count;

	pMesh->bDirty = true;				// Needs GPU upload
}

void Mesh3D_MakeSphere3D(Mesh3D pMesh, Vector3 center, float radius, int segments, int slices, Vector4 color)
{
	// Update metadata
	TransformSetPositionV(&pMesh->transform, center); // Update Transformation Matrix

	// 1. Calculate the Normal Matrix: transpose(inverse(mat3(model)))
	Matrix4 model = TransformGetMatrix(&pMesh->transform);

	Matrix3 mat3Model = Matrix3_InitMatrix4(model);
	Matrix3 invModel = Matrix3_Inverse(mat3Model);
	Matrix3 normalMatrix = Matrix3_TransposeN(invModel);

	GLint index = 0;
	// Generate Vertices (Poles and Rings)
	for (int i = 0; i <= segments; ++i) // Iterate through stacks (latitude) - iSegments
	{
		float phi = (float)(i) / (float)(segments) * (float)(M_PI); // angle phi from 0 to PI
		float v = (float)i / (float)segments;

		for (int j = 0; j <= slices; ++j) // Iterate through slices (longitude)
		{
			float theta = (float)(j) / (float)(slices) * (2.0f * (float)(M_PI)); // angle theta from 0 to 2*PI

			float u = (float)j / (float)slices;

			// Calculate the 4 corners of the "quad" patch
			Vector3 localPos = GetSpherePos(0.0f, 0.0f, 0.0f, radius, phi, theta);

			// 2. Transform Position to World Space
			// Use the full Matrix4 for position to include translation/rotation/scale
			Vector3 worldPos = Matrix4_Mul_Vec3(model, localPos);

			// Vector3 normal = Vector3_Normalized(Vector3_Sub(center, pos));
			Vector3 localNormal = { localPos.x / radius, localPos.y / radius, localPos.z / radius };
			Vector3 worldNormal = Vector3_Normalized(Matrix3_Mul_Vec3(normalMatrix, localNormal));

			index = i * (slices + 1) + j;

			SVertex3D vtx = { 0 };
			vtx.m_v3Position = worldPos;
			vtx.m_v3Normals = worldNormal;
			vtx.m_v2TexCoords = (Vector2){ u, v };
			vtx.m_v4Color = color;

			Vector_PushBackValue(pMesh->pVertices, vtx);
		}
	}

	// Generate indices (Poles and Rings)
	for (int i = 0; i < segments; ++i) // Iterate through stacks (latitude) - iSegments
	{
		for (int j = 0; j < slices; ++j) // Iterate through slices (longitude)
		{
			GLuint i0 = i * (slices + 1) + j;	// (bottom-left)
			GLuint i1 = i0 + 1;					// (bottom-right)
			GLuint i2 = i0 + (slices + 1);		// (top-left)
			GLuint i3 = i2 + 1;					// (top-right)

			Vector_PushBackValue(pMesh->pIndices, i0);
			Vector_PushBackValue(pMesh->pIndices, i1);
			Vector_PushBackValue(pMesh->pIndices, i2);

			Vector_PushBackValue(pMesh->pIndices, i1);
			Vector_PushBackValue(pMesh->pIndices, i3);
			Vector_PushBackValue(pMesh->pIndices, i2);
		}
	}

	pMesh->vertexCount = (GLuint)pMesh->pVertices->count;
	pMesh->indexCount = (GLuint)pMesh->pIndices->count;
	pMesh->meshColor = color;
	pMesh->bDirty = true;				// Needs GPU upload
}

void Mesh3D_MakeQuad3D(Mesh3D pMesh, Vector3 topLeft, Vector3 topRight, Vector3 bottomLeft, Vector3 bottomRight, Vector4 color)
{
	GLuint baseOffset = (GLuint)pMesh->pVertices->count;

	// Add 4 vertices
	SVertex3D v0 = { .m_v3Position = topLeft, .m_v2TexCoords = Vector2D(0.0f, 1.0f), .m_v4Color = color };
	SVertex3D v1 = { .m_v3Position = topRight, .m_v2TexCoords = Vector2D(1.0f, 1.0f), .m_v4Color = color };
	SVertex3D v2 = { .m_v3Position = bottomLeft, .m_v2TexCoords = Vector2D(0.0f, 0.0f), .m_v4Color = color };
	SVertex3D v3 = { .m_v3Position = bottomRight, .m_v2TexCoords = Vector2D(1.0f, 0.0f), .m_v4Color = color };

	// Calculate normal for the quad
	Vector3 edge1 = Vector3_Sub(topRight, topLeft);
	Vector3 edge2 = Vector3_Sub(bottomLeft, topLeft);
	Vector3 normal = Vector3_Normalized(Vector3_Cross(edge1, edge2));

	v0.m_v3Normals = normal;
	v1.m_v3Normals = normal;
	v2.m_v3Normals = normal;
	v3.m_v3Normals = normal;

	Vector_PushBackValue(pMesh->pVertices, v0);
	Vector_PushBackValue(pMesh->pVertices, v1);
	Vector_PushBackValue(pMesh->pVertices, v2);
	Vector_PushBackValue(pMesh->pVertices, v3);

	pMesh->vertexCount += 4;

	// Add 6 indices (2 triangles)
	GLuint idx0 = baseOffset;
	GLuint idx1 = baseOffset + 1;
	GLuint idx2 = baseOffset + 2;
	GLuint idx3 = baseOffset + 3;

	// First triangle (counter-clockwise)
	Vector_PushBackValue(pMesh->pIndices, idx0);  // topLeft
	Vector_PushBackValue(pMesh->pIndices, idx2);  // bottomLeft
	Vector_PushBackValue(pMesh->pIndices, idx1);  // topRight

	// Second triangle (counter-clockwise)
	Vector_PushBackValue(pMesh->pIndices, idx1);  // topRight
	Vector_PushBackValue(pMesh->pIndices, idx2);  // bottomLeft
	Vector_PushBackValue(pMesh->pIndices, idx3);  // bottomRight

	pMesh->indexCount += 6;
}

void Mesh3D_SetName(Mesh3D pMesh, const char* szName)
{
	if (pMesh->szMeshName)
	{
		tracked_free(pMesh->szMeshName);
	}

	pMesh->szMeshName = tracked_strdup(szName);
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

	if (mesh->szMeshName)
	{
		tracked_free(mesh->szMeshName);
		mesh->szMeshName = NULL;
	}

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

	if (pMesh->szMeshName)
	{
		tracked_free(pMesh->szMeshName);
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

