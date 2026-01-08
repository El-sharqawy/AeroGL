#include "Renderer.h"
#include "../Core/CoreUtils.h"
#include "../Lib/Vector.h"
#include <memory.h>
#include "StateManager.h"

bool CreateRenderer(Renderer* ppRenderer, GLCamera pCamera, const char* szRendererName)
{
	*ppRenderer = (Renderer)tracked_malloc(sizeof(SRenderer));

	// 2. Now, create a local helper pointer so the rest of the code is easy to read
	Renderer pRenderer = *ppRenderer;
	if (!pRenderer)
	{
		syslog("Failed to Create Renderer");
		return (false);
	}

	// This ensures pShader, pBuffer, pVertices etc. are all NULL
	memset(pRenderer, 0, sizeof(SRenderer));

	pRenderer->pCamera = pCamera;

	// Initialize Renderer Name
	pRenderer->szRendererName = tracked_strdup(szRendererName);

	// Initialize Shaders
	 InitializeShader(&pRenderer->pShader, "NormalShader");

	// Validation Check
	if (!pRenderer->pShader)
	{
		syslog("Failed To Initialize Renderer Shaders");
		DestroyRenderer(&pRenderer);
		return (false);
	}

	AttachShader(pRenderer->pShader, "Assets/Shaders/normal_shader.vert");
	AttachShader(pRenderer->pShader, "Assets/Shaders/normal_shader.frag");
	LinkProgram(pRenderer->pShader);

	// Validation Check
	if (!InitializeGLBuffer(&pRenderer->pLinesBuffer))
	{
		syslog("Failed To Initialize Renderer Buffer");
		DestroyProgram(&pRenderer->pShader);
		DestroyRenderer(&pRenderer);
		return (false);
	}

	// Validation Check
	if (!InitializeGLBuffer(&pRenderer->pTrianglesBuffer))
	{
		syslog("Failed To Initialize Renderer Buffer");
		DestroyProgram(&pRenderer->pShader);
		DestroyRenderer(&pRenderer);
		return (false);
	}

	// Initialize Vectors
	pRenderer->pLineVertices = VectorInit(sizeof(SVertex));
	pRenderer->pLineIndices = VectorInit(sizeof(GLuint));

	pRenderer->pTriangleVertices = VectorInit(sizeof(SVertex));
	pRenderer->pTriangleIndices = VectorInit(sizeof(GLuint));

	pRenderer->v4DiffuseColor = Vector4D(1.0f, 1.0f, 1.0f, 1.0f); // Default White

	return (true);
}

void DestroyRenderer(Renderer* ppRenderer)
{
	if (!ppRenderer || !*ppRenderer)
	{
		syserr("Attemp to delete a NULL shader");
		return;
	}

	Renderer pRenderer = *ppRenderer;
	if (pRenderer->szRendererName)
	{
		tracked_free(pRenderer->szRendererName);
	}
	DestroyProgram(&pRenderer->pShader);
	DestroyBuffer(&pRenderer->pLinesBuffer);
	DestroyBuffer(&pRenderer->pTrianglesBuffer);

	VectorFree(&pRenderer->pLineVertices);
	VectorFree(&pRenderer->pLineIndices);

	VectorFree(&pRenderer->pTriangleVertices);
	VectorFree(&pRenderer->pTriangleIndices);

	tracked_free(pRenderer);
	*ppRenderer = NULL;
}

void InitializeTriangle(Renderer pRenderer)
{
	// Define 5 points for a pyramid (4 base points, 1 apex)
	// Position (x,y,z)           Normals (x,y,z)     UV (u,v)     Color (r,g,b,a)
	SVertex vertex1 = { { 0.0f, 1.0f, 0.0f  }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }; // 0: Apex (Red)
	SVertex vertex2 = { {-0.5f, 0.0f, 0.5f  }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }; // 1: Front-Left (Green)
	SVertex vertex3 = { { 0.5f, 0.0f, 0.5f  }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }; // 2: Front-Right (Blue)
	SVertex vertex4 = { { 0.5f, 0.0f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } }; // 3: Back-Right (Yellow)
	SVertex vertex5 = { {-0.5f, 0.0f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }; // 4: Back-Left (Magenta)

	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex1);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex2);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex3);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex4);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex5);

	// 18 Indices total (4 sides + 2 for the square base)
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 0); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 1); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 2); // Front face
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 0); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 2); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 3); // Right face
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 0); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 3); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 4); // Back face
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 0); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 4); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 1); // Left face
	// Base (Two triangles to make a square)
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 1); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 4); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 3); // Left face
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 1); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 3); ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, 2); // Left face
}

void RenderRenderer(Renderer pRenderer)
{
	RenderRendererLines(pRenderer);
	RenderRendererTriangles(pRenderer);
}

void RenderRendererLines(Renderer pRenderer)
{
	SetRendererDiffuseColor(pRenderer, 1.0f, 0.0f, 0.0f, 1.0f); // Red - X Axis
	RenderLine3D(pRenderer, 0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f);

	SetRendererDiffuseColor(pRenderer, 0.0f, 1.0f, 0.0f, 1.0f); // Green - Y Axis
	RenderLine3D(pRenderer, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f, 0.0f);

	SetRendererDiffuseColor(pRenderer, 0.0f, 0.0f, 1.0f, 1.0f); // Blue - Z Axis
	RenderLine3D(pRenderer, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f);

	SetRendererDiffuseColor(pRenderer, 1.0f, 0.0f, 1.0f, 1.0f); // Idk
	RenderLine2D(pRenderer, 0.0f, 0.0f, 50.0f, 50.0f, 0.0f);

	RenderCircle2D(pRenderer, 0.0f, 0.0f, 0.0f, 1.0f, 8, false);
	RenderCircle3D(pRenderer, 0.0f, 0.0f, 15.0f, 1.0f, 32);

	RenderSphere3D(pRenderer, 0.0f, 10.0f, 0.0f, 3.0f, 64, 64);

	if (pRenderer->pLineVertices->count == 0)
	{
		return;
	}

	PushState(GetStateManager());
	BindShader(GetStateManager(), pRenderer->pShader);

	Matrix4 modelMat = S_Matrix4_Identity;

	SetMat4(pRenderer->pShader, "u_matViewProjection", GetViewProjectionMatrix(pRenderer->pCamera));
	SetMat4(pRenderer->pShader, "u_matModel", modelMat);

	UpdateBufferVertexData(pRenderer->pLinesBuffer, (SVertex*)pRenderer->pLineVertices->pData, pRenderer->pLineVertices->count, (GLuint*)pRenderer->pLineIndices->pData, pRenderer->pLineIndices->count);
	
	RenderBuffer(pRenderer->pLinesBuffer, GL_LINES);
	PopState(GetStateManager());

	VectorClear(pRenderer->pLineVertices);
	VectorClear(pRenderer->pLineIndices);
}

void RenderRendererTriangles(Renderer pRenderer)
{
	// InitializeTriangle(pRenderer);
	SetRendererDiffuseColor(pRenderer, 1.0f, 1.0f, 1.0f, 1.0f); // Idk
	RenderSolidCircle2D(pRenderer, 0.0f, 0.0f, 0.0f, 1.0f, 64, false);
	RenderSolidCircle3D(pRenderer, 20.0f, 0.0f, 0.0f, 1.0f, 64);

	if (pRenderer->pTriangleVertices->count == 0)
	{
		return;
	}

	
	PushState(GetStateManager());
	BindShader(GetStateManager(), pRenderer->pShader);

	Matrix4 modelMat = S_Matrix4_Identity;

	// 1. Initialize once (happens when the program starts)
	static float fDegree = 0.0f;

	// 2. Increment every time the function is called
	fDegree += 0.01f;

	// modelMat = Matrix4_Translate(modelMat, (Vector3) { 5.0f, 0.0f, 0.0f });
	// modelMat = Matrix4_Rotate(modelMat, fDegree, (Vector3) { 0.0f, 1.0f, 0.0f });
	// modelMat = Matrix4_Scale(modelMat, (Vector3) { 1.0f, 1.0f, 1.0f }); // Scale applied first to vertices

	SetMat4(pRenderer->pShader, "u_matViewProjection", GetViewProjectionMatrix(pRenderer->pCamera));
	SetMat4(pRenderer->pShader, "u_matModel", modelMat);

	SetCapability(GetStateManager(), CAP_CULL_FACE, false);

	UpdateBufferVertexData(pRenderer->pTrianglesBuffer, (SVertex*)pRenderer->pTriangleVertices->pData, pRenderer->pTriangleVertices->count, (GLuint*)pRenderer->pTriangleIndices->pData, pRenderer->pTriangleIndices->count);
	RenderBuffer(pRenderer->pTrianglesBuffer, GL_TRIANGLES);
	PopState(GetStateManager());
	
	VectorClear(pRenderer->pTriangleVertices);
	VectorClear(pRenderer->pTriangleIndices);
}

void SetRendererDiffuseColorV(Renderer pRenderer, Vector4 v4DiffuseColor)
{
	pRenderer->v4DiffuseColor = v4DiffuseColor;
}

void SetRendererDiffuseColor(Renderer pRenderer, float r, float g, float b, float a)
{
	pRenderer->v4DiffuseColor = Vector4D(r, g, b, a);
}

void RenderLine3D(Renderer pRenderer, float sx, float sy, float sz, float ex, float ey, float ez)
{
	GLuint offset = (GLuint)pRenderer->pLineVertices->count;

	SVertex startVertex = { 0 };
	startVertex.m_v3Position = Vector3D(sx, sy, sz);
	startVertex.m_v4Color = pRenderer->v4DiffuseColor;

	SVertex endVertex = { 0 };
	endVertex.m_v3Position = Vector3D(ex, ey, ez);
	endVertex.m_v4Color = pRenderer->v4DiffuseColor;

	ANUBIS_VECTOR_PUSH(pRenderer->pLineVertices, SVertex, startVertex);
	ANUBIS_VECTOR_PUSH(pRenderer->pLineVertices, SVertex, endVertex);

	ANUBIS_VECTOR_PUSH(pRenderer->pLineIndices, GLuint, offset);
	ANUBIS_VECTOR_PUSH(pRenderer->pLineIndices, GLuint, offset + 1);
}

void RenderLine2D(Renderer pRenderer, float sx, float sz, float ex, float ez, float y)
{
	RenderLine3D(pRenderer, sx, y, sz, ex, y, ez); // Make it with specific height
}

void RenderTriangle3D(Renderer pRenderer, Vector3 p1, Vector3 p2, Vector3 p3)
{
	GLuint baseOffset = (GLuint)pRenderer->pTriangleVertices->count;

	SVertex vertex1 = { .m_v3Position = p1, .m_v4Color = pRenderer->v4DiffuseColor };
	SVertex vertex2 = { .m_v3Position = p2, .m_v4Color = pRenderer->v4DiffuseColor };
	SVertex vertex3 = { .m_v3Position = p3, .m_v4Color = pRenderer->v4DiffuseColor };

	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex1);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex2);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleVertices, SVertex, vertex3);

	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, baseOffset);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, baseOffset + 1);
	ANUBIS_VECTOR_PUSH(pRenderer->pTriangleIndices, GLuint, baseOffset + 2); // Triangle Face
}

void RenderCircle2D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step, bool bHorizonal)
{
	float Theta = 0.0f;
	float Delta = 2.0f * (float)M_PI / (float)step;

	float startX = cx + radius * cosf(Theta);
	float startY = cy;
	float startZ = cz + radius * sinf(Theta); // sinf(0) = 0

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

		float x = cx + radius * cosf(Theta);
		float y = cy;
		float z = cz + radius * sinf(Theta);

		if (bHorizonal)
		{
			currentPoint = Vector3D(x, y, z);
		}
		else
		{
			currentPoint = Vector3D(x, z, y);
		}

		RenderLine3D(pRenderer, startPoint.x, startPoint.y, startPoint.z, currentPoint.x, currentPoint.y, currentPoint.z);

		startPoint = currentPoint;
	}
}

void RenderCircle3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step)
{
	Matrix4 billboard = GetViewBillboardMatrix(pRenderer->pCamera);

	float Theta = 0.0f;
	float Delta = 2.0f * (float)M_PI / (float)step;

	// 1. Calculate the START point at (0,0,0) and rotate it
	Vector4 localStart = Vector4D(radius * cosf(0.0f), radius * sinf(0.0f), 0.0f, 1.0f);
	Vector4 rotatedStart = Matrix4_Mul_Vec4(billboard, localStart);

	// 2. ONLY NOW add the center offset (cx, cy, cz)
	Vector3 prevPoint = Vector3D(cx + rotatedStart.x, cy + rotatedStart.y, cz + rotatedStart.z);

	for (int i = 1; i <= step; i++)
	{
		Theta = i * Delta;

		// 3. Generate point in local space (centered at 0,0,0)
		Vector4 localPoint = Vector4D(radius * cosf(Theta), radius * sinf(Theta), 0.0f, 1.0f);

		// 4. Rotate to face camera
		Vector4 rotatedPoint = Matrix4_Mul_Vec4(billboard, localPoint);

		// 5. Move to world center
		Vector3 currentPoint = Vector3D(cx + rotatedPoint.x, cy + rotatedPoint.y, cz + rotatedPoint.z);

		// 6. Draw line from the correctly offset previous point
		RenderLine3D(pRenderer, prevPoint.x, prevPoint.y, prevPoint.z, currentPoint.x, currentPoint.y, currentPoint.z);

		prevPoint = currentPoint;
	}
}

void RenderSolidCircle2D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step, bool bHorizonal)
{
	float Delta = 2.0f * (float)M_PI / (float)step;

	// The shared center point for all triangles
	Vector3 centerPoint = Vector3D(cx, cy, cz);

	for (int i = 0; i < step; i++)
	{
		float theta1 = i * Delta;
		float theta2 = (i + 1) * Delta;

		// Calculate the two points on the edge
		float x1 = cx + radius * cosf(theta1);
		float z1 = cz + radius * sinf(theta1);

		float x2 = cx + radius * cosf(theta2);
		float z2 = cz + radius * sinf(theta2);

		Vector3 p1, p2;
		if (bHorizonal)
		{
			p1 = Vector3D(x1, cy, z1);
			p2 = Vector3D(x2, cy, z2);
		}
		else
		{
			p1 = Vector3D(x1, z1, cy);
			p2 = Vector3D(x2, z2, cy);
		}

		// Draw
		RenderTriangle3D(pRenderer, centerPoint, p1, p2);
	}
}

void RenderSolidCircle3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step)
{
	Matrix4 billboard = GetViewBillboardMatrix(pRenderer->pCamera);

	float Delta = 2.0f * (float)M_PI / (float)step;

	// The shared center point in world space
	Vector3 centerPoint = Vector3D(cx, cy, cz);

	for (int i = 0; i < step; i++)
	{
		float theta1 = i * Delta;
		float theta2 = (i + 1) * Delta;

		// Position in the XY plane, relative to the center
		// Note: No billboard transform, circle is fixed in World Space.
		Vector4 point_local1 = Vector4D(radius * cosf(theta1), radius * sinf(theta1), 0.0f, 1.0f);
		Vector4 point_local2 = Vector4D(radius * cosf(theta2), radius * sinf(theta2), 0.0f, 1.0f);

		// Rotate points to face camera
		Vector4 rotP1 = Matrix4_Mul_Vec4(billboard, point_local1);
		Vector4 rotP2 = Matrix4_Mul_Vec4(billboard, point_local2);

		// 4. Offset to world position (Add the center coordinates)
		float x1 = cx + radius * cosf(theta1);
		float z1 = cz + radius * sinf(theta1);

		float x2 = cx + radius * cosf(theta2);
		float z2 = cz + radius * sinf(theta2);

		Vector3 worldP1 = Vector3D(cx + rotP1.x, cy + rotP1.y, cz + rotP1.z);
		Vector3 worldP2 = Vector3D(cx + rotP2.x, cy + rotP2.y, cz + rotP2.z);

		// Draw
		RenderTriangle3D(pRenderer, centerPoint, worldP1, worldP2);
	}
}

void RenderWiredSphere3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int segments, int slices)
{
	// Generate Vertices (Poles and Rings)
	for (int i = 0; i <= segments; ++i) // Iterate through stacks (latitude) - iSegments
	{
		float phi = (float)(i) / (float)(segments) * (float)(M_PI); // angle phi from 0 to PI
		float sinPhi = sinf(phi);
		float cosPhi = cosf(phi);

		Vector3 prevPos = { 0 }; // Store the previous position to draw lines
		for (int j = 0; j <= slices; ++j) // Iterate through slices (longitude)
		{
			float theta = (float)(j) / (float)(slices) * (2.0f * (float)(M_PI)); // angle theta from 0 to 2*PI
			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			// Calculate 3D coordinates
			float x = radius * cosTheta * sinPhi;
			float y = radius * sinTheta * sinPhi;
			float z = radius * cosPhi;

			Vector3 currentPos = Vector3D(cx + x, cy + y, cz + z);

			if (j > 0)
			{
				// Draw longitude line segments (around the stack/ring)
				RenderLine3D(pRenderer, prevPos.x, prevPos.y, prevPos.z, currentPos.x, currentPos.y, currentPos.z);
			}

			prevPos = currentPos;
		}
	}
}

/**
 * @brief Renders a wireframe 3D sphere at a specified center with given radius.
 *
 * This function draws a wireframe sphere centered at (cx, cy, cz) with a specified radius.
 * The sphere is approximated using the specified number of segments and slices.
 *
 * @param cx   X coordinate of the sphere center.
 * @param cy   Y coordinate of the sphere center.
 * @param cz   Z coordinate of the sphere center.
 * @param radius    Radius of the sphere.
 * @param segments  Latitude, This determines the vertical resolution (from pole to pole). 128 is an extremely high number and provides excellent vertical smoothness.
 * @param slices    Longitude, This determines the horizontal resolution (around the equator). 128 is an extremely high number and provides excellent horizontal smoothness.
 */
void RenderSphere3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int segments, int slices)
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
			Vector3 v1 = GetSpherePos(cx, cy, cz, radius, phi1, theta1);
			Vector3 v2 = GetSpherePos(cx, cy, cz, radius, phi1, theta2);
			Vector3 v3 = GetSpherePos(cx, cy, cz, radius, phi2, theta1);
			Vector3 v4 = GetSpherePos(cx, cy, cz, radius, phi2, theta2);

			// Draw two triangles to form the solid face
			RenderTriangle3D(pRenderer, v1, v2, v3);
			RenderTriangle3D(pRenderer, v2, v4, v3);
		}
	}
}
