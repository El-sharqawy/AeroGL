#ifndef __GL_BUFFER_H__
#define __GL_BUFFER_H__

#include <glad/glad.h>
#include "Buffer.h"

#define INITIAL_VERTEX_CAPACITY 8190	// Start with 8k vertices
#define INITIAL_INDEX_CAPACITY 16384	// Start with 16k indices

/* Global Buffer Functions*/
void DeleteBuffer(GLuint* puiBufferID);
void DeleteVertexArray(GLuint* puiVertexArrayID);
void DeleteBuffers(GLuint* puiBufferIDs, GLsizei uiCount);
void DeleteVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

bool CreateBuffer(GLuint* puiBufferID);
bool CreateVertexArray(GLuint* puiVertexArrayID);
bool CreateBuffers(GLuint* puiBuffersIDs, GLsizei uiCount);
bool CreateVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

#endif // __GL_BUFFER_H__