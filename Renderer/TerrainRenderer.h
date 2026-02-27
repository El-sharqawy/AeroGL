#ifndef __TERRAIN_RENDERER_H__
#define __TERRAIN_RENDERER_H__

#include "../PipeLine/Shader.h"
#include "../Buffers/TerrainBuffer.h"
#include "../Buffers/IndirectBufferObject.h"
#include "../Buffers/ShaderStorageBufferObject.h"
#include "../Core/Camera.h"
#include "../Terrain/Terrain/Terrain.h"
#include "../Terrain/TerrainMap/TerrainMap.h"

typedef struct SPatchData
{
    Matrix4 modelMatrix;
    uint64_t heightMapHandle;       // The Bindless Texture Handle
    float heightScale;              // Specific height scale for this terrain
    int32_t terrainCoords[2];       // Maintain 16-byte alignment for GLSL
} SPatchData;

typedef struct STerrainRenderer
{
    // GPU Resources
    GLShader pTerrainShader;
    TerrainGLBuffer pTerrainBuffer;
    IndirectBufferObject pIndirectBuffer;
    ShaderStorageBufferObject pRendererSSBO; // for Metrices

    // Typed primitive groups (dynamic)
    GLenum primitiveType; // GL_LINES or GL_TRIANGLES

    // Renderer Data
    char* szRendererName;
    Vector4 v4DiffuseColor;
    GLCamera pCamera;
    Vector3 v3PickingPoint;
} STerrainRenderer;

typedef struct STerrainRenderer* TerrainRenderer;

bool TerrainRenderer_Initialize(TerrainRenderer* ppTerrainRenderer, const char* szRendererName, int32_t iTerrainX, int32_t iTerrainZ);
void TerrainRenderer_Destroy(TerrainRenderer* pTerrainRenderer);
bool TerrainRenderer_InitGLBuffers(TerrainRenderer pTerrainRenderer, GLenum glType, GLsizeiptr capacity);
void TerrainRenderer_DestroyGLBuffers(TerrainRenderer pTerrainRenderer);

void TerrainRenderer_UploadGPUData(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_Render(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_RenderIndirect(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_RenderLegacy(TerrainRenderer pTerrainRenderer);

void TerrainRenderer_Reset(TerrainRenderer pTerrainRenderer);

#endif // __TERRAIN_RENDERER_H__