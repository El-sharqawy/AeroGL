#ifndef __UNIFORM_BUFFER_OBJECT_H__
#define __UNIFORM_BUFFER_OBJECT_H__

#include <glad/glad.h>
#include <stdbool.h>

enum EUBOBindingPoint
{
	UBO_BP_CAMERA = 0,			// Binding point 0 reserved for Camera UBO
	UBO_BP_LIGHTING = 1,
	UBO_BP_MATERIAL = 2,
	UBO_BP_TRANSFORM = 3,
	UBO_BP_ANIMATION = 4,
	UBO_BP_POSTPROCESS = 5,
	UBO_BP_MAX_NUM = 6
};

typedef struct SUniformBufferObject
{
	GLuint bufferID;
	GLuint bindingPoint; // EUBOBindingPoint
	GLsizeiptr bufferSize;
	GLsizeiptr writeOffset; // offset of last written byte
	char* szBufferName;
} SUniformBufferObject;

typedef struct SUniformBufferObject* UniformBufferObject;

bool InitializeUniformBufferObject(UniformBufferObject* ppUniBufObj, GLsizeiptr bufferSize, GLuint bindingPt, const char* szBufferName);
void DestroyUniformBufferObject(UniformBufferObject* ppUniBufObj);

void UniformBufferObject_Update(UniformBufferObject pUniBufObj, const void* pData, GLsizeiptr size, GLuint offset, bool bReallocation);
void UniformBufferObject_Bind(UniformBufferObject pUniBufObj);
void UniformBufferObject_UnBind(UniformBufferObject pUniBufObj);
void UniformBufferObject_Reallocate(UniformBufferObject pUniBufObj, GLsizeiptr newSize, bool copyOldData);

#endif // __UNIFORM_BUFFER_OBJECT_H__