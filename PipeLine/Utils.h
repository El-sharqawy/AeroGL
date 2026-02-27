#ifndef __GL_UTILS_H__
#define __GL_UTILS_H__

#include <glad/glad.h>
#include <stdbool.h>

/* Global Buffer Functions*/
void GL_DeleteBuffer(GLuint* puiBufferID);
void GL_DeleteVertexArray(GLuint* puiVertexArrayID);
void GL_DeleteBuffers(GLuint* puiBufferIDs, GLsizei uiCount);
void GL_DeleteVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

bool GL_CreateBuffer(GLuint* puiBufferID);
bool GL_CreateVertexArray(GLuint* puiVertexArrayID);
bool GL_CreateBuffers(GLuint* puiBuffersIDs, GLsizei uiCount);
bool GL_CreateVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount);

void GL_DeleteTexture(GLuint* puiTextureID);
void GL_DeleteTextures(GLuint* puiTextureIDs, GLsizei uiCount);

bool GL_CreateTexture(GLuint* puiTextureID, GLenum eTextureTarget);
bool GL_CreateTextures(GLuint* puiTextureIDs, GLsizei uiCount, GLenum eTextureTarget);

void GL_GetTextureFormats(GLint channels, GLenum* internal, GLenum* pixel, bool bIsFloat);

#endif // __GL_UTILS_H__