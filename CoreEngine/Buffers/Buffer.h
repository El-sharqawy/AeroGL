#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <glad/glad.h>
#include <stdbool.h>
#include "../Math/Vertex/Vertex.h"
#include "../Meshes/Mesh3D.h"

typedef struct SGLBuffer* GLBuffer;

// GL Buffer Functions
void DeleteGLBuffer(GLBuffer buffer);
bool CreateGLBuffer(GLBuffer buffer);

void ResetBuffer(GLBuffer buffer);
void ClearBuffer(GLBuffer buffer);
void DestroyBuffer(GLBuffer* buffer);

/* SVertex Struct */
bool AllocateVertexBuffersStorage(GLBuffer buffer);
void LinkVertexBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void SetupVertexBufferAttributesVertex(GLBuffer buffer);
void UpdateBufferVertexData(GLBuffer buffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);

// Combine the previous Functions Into One with Malloc
bool InitializeVertexGLBuffer(GLBuffer* ppBuffer);
void ReallocateVertexBuffer(GLBuffer buffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData);


/* SMesh3D Struct */
bool AllocateMesh3DBuffersStorage(GLBuffer buffer);
void LinkMesh3DBuffers(GLBuffer buffer); // Must be called before SetupVertexBufferAttributesVertex ! 
void SetupVertexBufferAttributesMesh3D(GLBuffer buffer);
void UpdateBufferMesh3DVertexData(GLBuffer buffer, const SVertex3D* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount);
void UpdateBufferMesh3DData(GLBuffer buffer, Mesh3D mesh);
bool InitializeMesh3DGLBuffer(GLBuffer* ppBuffer);
void ReallocateMesh3DBuffer(GLBuffer buffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData);

void RenderBuffer(GLBuffer buffer, GLenum renderMode);

GLuint GetVertexArray(GLBuffer buffer);
GLsizeiptr GetBufferVertexOffset(GLBuffer buffer);
GLsizeiptr GetBufferIndexOffset(GLBuffer buffer);

#endif // __BUFFER_H__