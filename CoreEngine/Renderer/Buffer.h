#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <glad/glad.h>
#include <stdbool.h>
#include "../Math/Vertex/Vertex.h"
#include "../Meshes/Mesh3D.h"
#include "StateManager.h"

#define INITIAL_VERTEX_CAPACITY 8190	// Start with 8k vertices
#define INITIAL_INDEX_CAPACITY 16384	// Start with 16k indices

typedef struct SGLBuffer* GLBuffer;

/* Global Buffer Functions*/
void DeleteBuffer(GLuint* puiBufferID);
void DeleteVertexArray(GLuint* puiVertexArrayID);
void DeleteBuffers(GLuint* puiBufferIDs, GLsizei uiCount);
void DeleteVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

bool CreateBuffer(GLuint* puiBufferID);
bool CreateVertexArray(GLuint* puiVertexArrayID);
bool CreateBuffers(GLuint* puiBuffersIDs, GLsizei uiCount);
bool CreateVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

void DeleteGLBuffer(GLBuffer buffer);
bool CreateGLBuffer(GLBuffer buffer);

void DestroyBuffer(GLBuffer* buffer);

/* SVertex Struct */
bool AllocateVertexBuffersStorage(GLBuffer buffer);
void LinkVertexBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void SetupVertexBufferAttributesVertex(GLBuffer buffer);
void UpdateBufferVertexData(GLBuffer buffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);

// Combine the previous Functions Into One with Malloc
bool InitializeVertexGLBuffer(GLBuffer* ppBuffer);
void ReallocateVertexBuffer(GLBuffer buffer, size_t vNewSize, size_t iNewSize, bool copyOldData);


/* SMesh3D Struct */
bool AllocateMesh3DBuffersStorage(GLBuffer buffer);
void LinkMesh3DBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void SetupVertexBufferAttributesMesh3D(GLBuffer buffer);
void UpdateBufferMesh3DVertexData(GLBuffer buffer, const SMesh3D* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);

void RenderBuffer(GLBuffer buffer, GLenum renderMode);

GLuint GetVertexArray(GLBuffer buffer);

#endif // __BUFFER_H__