#ifndef __DEBUG_RENDERER_H__
#define __DEBUG_RENDERER_H__

#include "../PipeLine/Shader.h"
#include "../Buffers/Buffer.h"
#include "../Buffers/IndirectBufferObject.h"
#include "../Buffers/ShaderStorageBufferObject.h"
#include "../Lib/Vector.h"
#include "../Core/Camera.h"
#include "../Meshes/TerrainMesh.h"

#define MAX_DEBUG_LINE_MESHES 10
#define MAX_DEBUG_MESHES 10

typedef enum EDebugPrimitiveType
{
    DEBUG_LINES,
    DEBUG_TRIANGLES,
    DEBUG_MAX_TYPES
} EDebugPrimitiveType;

typedef struct SDebugRendererPrimitiveGroup
{
    // Specific for Lines
    IndirectBufferObject pIndirectBuffer;

    // Specific for Lines
    Vector meshes;
    GLenum primitiveType; // GL_LINES or GL_TRIANGLES
    EDebugPrimitiveType groupType;
} SDebugRendererPrimitiveGroup;

typedef struct SDebugRenderer
{
    // GPU Resources
    GLShader pShader;
    GLBuffer pDynamicGeometryBuffer;
    ShaderStorageBufferObject pRendererSSBO; // for Metrices
    Matrix4 modelsMetrices[MAX_DEBUG_MESHES];    // CPU-side storage

    // Typed primitive groups (dynamic)
    SDebugRendererPrimitiveGroup groups[DEBUG_MAX_TYPES];

    // Renderer Data
    char* szRendererName;
    Vector4 v4DiffuseColor;
    GLCamera pCamera;
    Vector3 v3PickingPoint;
    int32_t meshCounter;
} SDebugRenderer;

typedef struct SDebugRenderer* DebugRenderer;

bool CreateDebugRenderer(DebugRenderer* ppDebugRenderer, GLCamera pCamera, const char* szRendererName);
bool DebugRenderer_InitGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type, GLenum glType, GLsizeiptr capacity);
void DebugRenderer_DestroyGroup(DebugRenderer pDebugRenderer, EDebugPrimitiveType type);
void DestroyDebugRenderer(DebugRenderer* ppDebugRenderer);

void InitializeDebuggingMeshes(DebugRenderer pDebugRenderer, EDebugPrimitiveType type);
void RenderDebugRenderer(DebugRenderer pDebugRenderer); // Main Renderer

void RenderDebugRenderer(DebugRenderer pDebugRenderer); // Lines Renderer
void RenderDebugRendererIndirect(DebugRenderer pDebugRenderer, EDebugPrimitiveType type);
void RenderDebugRendererLegacy(DebugRenderer pDebugRenderer, EDebugPrimitiveType type);

void DebugRenderer_SetLineMeshPosition(DebugRenderer pDebugRenderer, int meshIndex, Vector3 v3NewPos);

void DebugRenderer_UpdateSunPosition(DebugRenderer pDebugRenderer);
void DebugRenderer_UpdateDirtyMeshes(DebugRenderer pDebugRenderer);

void SetRenderColor(DebugRenderer pDebugRenderer, Vector4 v4Color);

#endif // __DEBUG_RENDERER_H__