#ifndef __TERRAIN_BUFFER__
#define __TERRAIN_BUFFER__

#include <glad/glad.h>
#include <stdbool.h>
#include "../Meshes/TerrainMesh.h"

typedef struct STerrainGLBuffer
{
	GLuint uiVAO;			// The Vertex Array Object (The "Boss" handle)
	GLuint uiVBO;			// The Vertex Buffer Object (The actual data)
	GLuint uiEBO;			// The Element Buffer (Optional, for optimized drawing)

	GLuint vertexCount;		// glDrawArrays needs it
	GLuint indexCount;		// glDrawElements needs it

	GLsizeiptr vboCapacity;
	GLsizeiptr eboCapacity;

	GLsizeiptr vertexOffset;
	GLsizeiptr indexOffset;

	GLsizeiptr vboSize; // Total Size (capacity * sizeof(element))
	GLsizeiptr eboSize; // Total Size (capacity * sizeof(GLuint))

	GLenum bufferStorageType;

	bool bIsInitialized;
} STerrainGLBuffer;

typedef struct STerrainGLBuffer* TerrainGLBuffer;

bool TerrainBuffer_Create(TerrainGLBuffer buffer);
void TerrainBuffer_Delete(TerrainGLBuffer buffer);

void TerrainBuffer_Reset(TerrainGLBuffer buffer);
void TerrainBuffer_Clear(TerrainGLBuffer buffer);

bool TerrainBuffer_Initialize(TerrainGLBuffer* ppTerrainBuffer);
void TerrainBuffer_Destroy(TerrainGLBuffer* ppTerrainBuffer);

bool TerrainBuffer_AllocateGPUStorage(TerrainGLBuffer pTerrainBuffer);
bool TerrainBuffer_AllocateVertexBuffer(TerrainGLBuffer pTerrainBuffer);
bool TerrainBuffer_LinkBuffers(TerrainGLBuffer pTerrainBuffer);

bool TerrainBuffer_Reallocate(TerrainGLBuffer pTerrainBuffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData);
bool TerrainBuffer_UploadData(TerrainGLBuffer pTerrainBuffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);

#endif // __TERRAIN_BUFFER__