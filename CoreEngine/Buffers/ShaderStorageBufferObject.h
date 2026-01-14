#ifndef __SHADER_STORAGE_BUFFER_OBJECT__
#define __SHADER_STORAGE_BUFFER_OBJECT__

#include <glad/glad.h>
#include <stdbool.h>

#define SSBO_MAX_OBJECTS_COUNT 4096

enum
{
	SSBO_BP_DEBUG_RENDERER,		// Binding point 0 reserved for Debug Renderer
	SSBO_BP_MAX_NUM,
};

typedef struct SShaderStorageBufferObject
{
	GLuint bufferID;
	GLuint bindingPoint; // EUBOBindingPoint
	GLsizeiptr bufferSize;
	GLsizeiptr writeOffset; // offset of last written byte
	char* szBufferName;
} SShaderStorageBufferObject;

typedef struct SShaderStorageBufferObject* ShaderStorageBufferObject;

bool InitializeShaderStorageBufferObject(ShaderStorageBufferObject* ppSSBO, GLsizeiptr bufferSize, GLuint bindingPt, const char* name);
void DestroyShaderStorageBufferObject(ShaderStorageBufferObject* ppSSBO);

void ShaderStorageBufferObject_Update(ShaderStorageBufferObject pSSBO, const void* pData, GLsizeiptr size, GLuint offset, bool bReallocation);
void ShaderStorageBufferObject_Reallocate(ShaderStorageBufferObject pSSBO, GLsizeiptr size, bool copyOldData);

void ShaderStorageBufferObject_Bind(ShaderStorageBufferObject pSSBO);
void ShaderStorageBufferObject_UnBind(ShaderStorageBufferObject pSSBO);

#endif // __SHADER_STORAGE_BUFFER_OBJECT__