#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <glad/glad.h>
#include <stdbool.h>
#include "../Math/Vertex/Vertex.h"
#include "../Meshes/Mesh3D.h"

typedef struct SGLBuffer* GLBuffer;

#define INITIAL_VERTEX_CAPACITY 8190	// Start with 8k vertices
#define INITIAL_INDEX_CAPACITY 16384	// Start with 16k indices

// GL Buffer Functions
void GLBuffer_Delete(GLBuffer buffer);
bool GLBuffer_Create(GLBuffer buffer);

void GLBuffer_ResetBuffer(GLBuffer buffer);
void GLBuffer_ClearBuffer(GLBuffer buffer);
void GLBuffer_DestroyBuffer(GLBuffer* buffer);

/* SVertex Struct */
bool GLBuffer_AllocateStorage(GLBuffer buffer);
void GLBuffer_LinkBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void GLBuffer_AllocateVertexBuffer(GLBuffer buffer);
void GLBuffer_UploadDataPtr(GLBuffer buffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);

// Combine the previous Functions Into One with Malloc
bool GLBuffer_Initialize(GLBuffer* ppBuffer);
bool GLBuffer_Reallocate(GLBuffer pBuffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData);


/* SMesh3D Struct */
bool Mesh3DGLBuffer_AllocateStorage(GLBuffer buffer);
void Mesh3DGLBuffer_LinkBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void Mesh3DGLBuffer_AllocateVertexBuffer(GLBuffer buffer);
void Mesh3DGLBuffer_UploadDataPtr(GLBuffer buffer, const SVertex3D* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);
void Mesh3DGLBuffer_UploadData(GLBuffer buffer, Mesh3D mesh);
bool Mesh3DGLBuffer_Initialize(GLBuffer* ppBuffer);
bool Mesh3DGLBuffer_Reallocate(GLBuffer buffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData);
void Mesh3DGLBuffer_RenderBuffer(GLBuffer buffer, GLenum renderMode);

GLuint Mesh3DGLBuffer_GetVertexArray(GLBuffer buffer);
GLsizeiptr Mesh3DGLBuffer_GetBufferVertexOffset(GLBuffer buffer);
GLsizeiptr Mesh3DGLBuffer_GetBufferIndexOffset(GLBuffer buffer);

#endif // __BUFFER_H__