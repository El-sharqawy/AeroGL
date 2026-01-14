#ifndef __TERRAIN_RENDERER_H__
#define __TERRAIN_RENDERER_H__

#include "Shader.h"
#include "../Buffers/Buffer.h"
#include "../Buffers/IndirectBufferObject.h"
#include "../Buffers/ShaderStorageBufferObject.h"
#include "../Core/Camera.h"

typedef enum ETerrainPrimitiveType
{
    TERRAIN_RENDERER_TRIANGLES,
    TERRAIN_RENDERER_MAX_TYPES
} ETerrainPrimitiveType;

typedef struct STerrainRendererPrimitiveGroup
{
    // Specific for Lines
    IndirectBufferObject pIndirectBuffer;
    ShaderStorageBufferObject pRendererSSBO; // for Metrices

    // Specific for Lines
    Matrix4 modelsMetrices[TERRAIN_RENDERER_MAX_TYPES];    // CPU-side storage
    Vector meshes;
    bool bMatricesDirty;
    GLenum primitiveType; // GL_LINES or GL_TRIANGLES
    ETerrainPrimitiveType groupType;
} STerrainRendererPrimitiveGroup;

typedef struct STerrainRenderer
{
    // GPU Resources
    GLShader pShader;
    GLBuffer pDynamicGeometryBuffer;

    // Typed primitive groups (dynamic)
    STerrainRendererPrimitiveGroup groups[TERRAIN_RENDERER_MAX_TYPES];

    // Renderer Data
    char* szRendererName;
    Vector4 v4DiffuseColor;
    GLCamera pCamera;
    Vector3 v3PickingPoint;
} STerrainRenderer;

typedef struct STerrainRenderer* TerrainRenderer;

bool TerrainRenderer_Initialize(TerrainRenderer* ppTerrainRenderer, GLCamera pCamera, const char* szRendererName);
void TerrainRenderer_Destroy(TerrainRenderer* pTerrainRenderer);
bool DebugRenderer_InitGroup(TerrainRenderer pTerrainRenderer, ETerrainPrimitiveType type, GLenum glType, GLsizeiptr capacity);
void DebugRenderer_DestroyGroup(TerrainRenderer pTerrainRenderer, ETerrainPrimitiveType type);

#endif // __TERRAIN_RENDERER_H__