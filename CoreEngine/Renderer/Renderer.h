#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "Shader.h"
#include "Buffer.h"
#include "../Lib/Vector.h"
#include "../Core/Camera.h"

typedef struct SRenderer
{
    // GPU Resources
    GLShader pShader;
    GLBuffer pStaticTrianglesBuffer;
    GLBuffer pDynamicTrianglesBuffer;

    // Static geometry (initialized once)
    Vector pStaticTriangleVertices;
    Vector pStaticTriangleIndices;

    // Dynamic geometry (changes per frame)
    Vector pDynamicTriangleVertices;
    Vector pDynamicTriangleIndices;

    // Renderer Data
    char* szRendererName;
    Vector4 v4DiffuseColor;
    GLCamera pCamera;
} SRenderer;

typedef struct SRenderer* Renderer;

bool CreateRenderer(Renderer* pRenderer, GLCamera pCamera, const char* szRendererName);
void DestroyRenderer(Renderer* pRenderer);

void InitializeStaticShapes(Renderer pRenderer);
void InitializeMesh2D(Renderer pRenderer);

void RenderRenderer(Renderer pRenderer);
void RenderRendererLines(Renderer pRenderer);
void RenderRendererStaticTriangles(Renderer pRenderer);
void RenderRendererDynamicTriangles(Renderer pRenderer);

void SetRendererDiffuseColorV(Renderer pRenderer, Vector4 v4DiffuseColor);
void SetRendererDiffuseColor(Renderer pRenderer, float r, float g, float b, float a);

void RenderLine3D(Renderer pRenderer, float sx, float sy, float sz, float ex, float ey, float ez);
void RenderLine2D(Renderer pRenderer, float sx, float sz, float ex, float ez, float y);
void RenderTriangle3D(Renderer pRenderer, Vector3 p1, Vector3 p2, Vector3 p3);

void RenderCircle2D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step, bool bHorizonal);
void RenderCircle3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step);
void RenderSolidCircle2D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step, bool bHorizonal);
void RenderSolidCircle3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int step);

void RenderWiredSphere3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int segments, int slices);
void RenderSphere3D(Renderer pRenderer, float cx, float cy, float cz, float radius, int segments, int slices);

#endif // __RENDERER_H__