#ifndef __INDIRECT_BUFFER_OBJECT_H__
#define __INDIRECT_BUFFER_OBJECT_H__

#include <stdint.h>
#include <stdbool.h>
#include <glad/glad.h>
#include "../Lib/Vector.h"

#define MAX_INSTANCE_DATA_COUNT 8192
#define MAX_INDIRECT_DRAW_COMMAND_COUNT 4096

typedef struct SIndirectDrawCommand
{
	GLuint count;			// Number of indices to draw
	GLuint instanceCount;	// Number of instances (1 = no instancing)
	GLuint firstIndex;	// Starting index in EBO
	GLuint baseVertex;	// Added to each index value
	GLuint baseInstance;	// Starting instance ID
} SIndirectDrawCommand;

typedef struct SIndirectDrawCommand* IndirectDrawCommand;

typedef struct SIndirectBufferObject
{
	// GPU buffer handle
	GLuint bufferID;         // GPU buffer handle
	Vector commands;         // Dynamic array of commands
	bool bDirty;             // Upload/Update flag
} SIndirectBufferObject;


typedef struct SIndirectBufferObject* IndirectBufferObject;

bool IndirectBufferObject_Initialize(IndirectBufferObject* ppIndirectBuffer, GLsizeiptr initialCapacity);

void IndirectBufferObject_GenerateGL(IndirectBufferObject pIndirectBuf, GLsizeiptr bufferSize);
void IndirectBufferObject_AddCommand(IndirectBufferObject pIndirectBuf, GLuint count, GLuint instanceCount, GLuint firstIndex, GLuint baseVertex, GLuint baseInstance);
void IndirectBufferObject_Upload(IndirectBufferObject pIndirectBuf);
void IndirectBufferObject_Draw(IndirectBufferObject pIndirectBuf, GLenum primitiveType);
void IndirectBufferObject_Clear(IndirectBufferObject pIndirectBuf);
void IndirectBufferObject_Bind(IndirectBufferObject pIndirectBuf);
void IndirectBufferObject_UnBind();

void IndirectBufferObject_SetCommand(IndirectBufferObject pIndirectBuf, size_t index, const IndirectDrawCommand cmd);

void IndirectBufferObject_Destroy(IndirectBufferObject* ppIndirectBuffer);

#endif // __INDIRECT_BUFFER_OBJECT_H__