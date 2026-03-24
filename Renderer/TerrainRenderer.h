#ifndef __TERRAIN_RENDERER_H__
#define __TERRAIN_RENDERER_H__

#include "../PipeLine/Shader.h"
#include "../Buffers/TerrainBuffer.h"
#include "../Buffers/IndirectBufferObject.h"
#include "../Buffers/ShaderStorageBufferObject.h"
#include "../Core/Camera.h"
#include "../Terrain/Terrain/Terrain.h"
#include "../Terrain/TerrainMap/TerrainMap.h"

typedef enum ERendererSSBOBP // Renderer SSBO Binding Points
{
    SSBO_BP_TERRAIN_DATA,
    SSBO_BP_PATCHES_DATA,
    SSBO_BP_TERRAIN_HEIGHTMAP,
} ERendererSSBOBP;

typedef struct SPatchGPUData
{
    int32_t terrainIndex;
    int32_t localPatchID; // Used to offset UVs (0-63)
} SPatchGPUData;

typedef struct STerrainGPUData
{
    uint32_t heightOffset;          // The heightmap Offset index (offset / sizeof(float))
    uint32_t padding;           // 4 bytes padding ← ADD THIS
    int32_t terrainCoords[2];       // Maintain 16-byte alignment for GLSL
} STerrainGPUData;

typedef struct STerrainRenderer
{
    // GPU Resources
    GLShader pTerrainShader;
    TerrainGLBuffer pTerrainBuffer;
    IndirectBufferObject pIndirectBuffer;
    ShaderStorageBufferObject pTerrainRendererSSBO; // for terrains
    ShaderStorageBufferObject pPatchRendererSSBO; // for patches
    ShaderStorageBufferObject pHeightMapSSBO; // for global heightMap

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
bool TerrainRenderer_InitGLBuffers(TerrainRenderer pTerrainRenderer, GLenum glType, GLint iTerrainX, GLint iTerrainZ);
void TerrainRenderer_DestroyGLBuffers(TerrainRenderer pTerrainRenderer);

void TerrainRenderer_UploadGPUData(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_Render(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_RenderIndirect(TerrainRenderer pTerrainRenderer);
void TerrainRenderer_RenderLegacy(TerrainRenderer pTerrainRenderer);

void TerrainRenderer_Reset(TerrainRenderer pTerrainRenderer);

#endif // __TERRAIN_RENDERER_H__