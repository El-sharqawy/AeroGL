#include "UniformBufferObject.h"
#include "../Stdafx.h"
#include "../PipeLine/Utils.h"
#include <memory.h>

static GLint siMaxUBOSize = 0;

bool InitializeUniformBufferObject(UniformBufferObject* ppUniBufObj, GLsizeiptr bufferSize, GLuint bindingPt, const char* szBufferName)
{
	if (ppUniBufObj == NULL)
	{
		syserr("ppUniBufObj is NULL (invalid address)");
		return false;
	}

	if (siMaxUBOSize == 0)
	{
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &siMaxUBOSize);
	}

	if (bufferSize > (GLsizeiptr)siMaxUBOSize)
	{
		syserr("UBO: Size %d exceeds hardware limit of %d", (GLint)bufferSize, siMaxUBOSize);
		return (false);
	}

	// make sure it's all elements set to zero bytes
	*ppUniBufObj = engine_new_zero(SUniformBufferObject, 1, MEM_TAG_GPU_BUFFER);

	UniformBufferObject buffer = *ppUniBufObj;

	buffer->szBufferName = engine_strdup(szBufferName, MEM_TAG_STRINGS);

	if (!GL_CreateBuffer(&buffer->bufferID))
	{
		DestroyUniformBufferObject(&buffer);
		syserr("Failed to Create GPU Uniform Buffers!");
		return (false);
	}

	buffer->bufferFlags = GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT;  // CPU writes immediately visible to GPU



	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferStorage(buffer->bufferID, bufferSize, NULL, GL_DYNAMIC_STORAGE_BIT | buffer->bufferFlags);

		// Map ONCE and keep pointer forever
		buffer->pBufferData = glMapNamedBufferRange(buffer->bufferID, 0, bufferSize, buffer->bufferFlags);
		buffer->isPersistent = true;
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, buffer->bufferID);

		if (IsGLVersionHigher(4, 4))
		{
			glBufferStorage(GL_UNIFORM_BUFFER, bufferSize, NULL, GL_DYNAMIC_STORAGE_BIT | buffer->bufferFlags); // Need to check if it's compitable with +4.4
			buffer->pBufferData = glMapBufferRange(GL_UNIFORM_BUFFER, 0, bufferSize, buffer->bufferFlags);
			buffer->isPersistent = true;
		}
		else
		{
			glBufferData(GL_UNIFORM_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
		}
	}

	if (buffer->isPersistent && buffer->pBufferData == NULL)
	{
		syslog("UBO Persistent map failed: 0x%x", glGetError());
		return false;
	}

	buffer->bindingPoint = bindingPt;
	buffer->bufferSize = bufferSize;
	buffer->writeOffset = 0;

	UniformBufferObject_Bind(buffer);

	syslog("Created UBO '%s': buffer=%u, size=%lld, binding=%u",
		szBufferName,
		buffer->bufferID,
		(long long)bufferSize,
		bindingPt);

	return (true);
}

void DestroyUniformBufferObject(UniformBufferObject* ppUniBufObj)
{
	if (!ppUniBufObj || !*ppUniBufObj)
	{
		return;
	}

	UniformBufferObject buffer = *ppUniBufObj;

	if (buffer->szBufferName)
	{
		engine_delete(buffer->szBufferName);
	}

	GL_DeleteBuffer(&buffer->bufferID); // Clear UP GPU Resources

	engine_delete(buffer);

	*ppUniBufObj = NULL;
}

void UniformBufferObject_Update(UniformBufferObject pUniBufObj, const void* pData, GLsizeiptr size, GLuint offset, bool bReallocation)
{
	if (!pUniBufObj || !pData || size <= 0)
	{
		return;
	}

	if (size > siMaxUBOSize)
	{
		syserr("Size %zu is not allowed, Max: %d", size, siMaxUBOSize);
		return;
	}

	if (size + offset > pUniBufObj->bufferSize)
	{
		if (bReallocation)
		{
			GLsizeiptr newSize = size + offset;
			UniformBufferObject_Reallocate(pUniBufObj, newSize, true);
		}
		else
		{
			syserr("UBO '%s': Write at offset %lld + size %lld = %lld exceeds capacity %lld (reallocation disabled)",
				pUniBufObj->szBufferName,
				(long long)offset,
				(long long)size,
				(long long)(offset + size),
				(long long)pUniBufObj->bufferSize);  // Actual capacity
			return;
		}
	}

	// Data Update
	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(pUniBufObj->bufferID, offset, size, pData);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, pUniBufObj->bufferID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, pData);
	}

	pUniBufObj->writeOffset = offset + size;  // Update offset after write
}

void UniformBufferObject_Bind(UniformBufferObject pUniBufObj)
{
	if (!pUniBufObj)
	{
		return;
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, pUniBufObj->bindingPoint, pUniBufObj->bufferID);
}

void UniformBufferObject_UnBind(UniformBufferObject pUniBufObj)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, pUniBufObj->bindingPoint, 0);
}

void UniformBufferObject_Reallocate(UniformBufferObject pUniBufObj, GLsizeiptr newSize, bool copyOldData)
{
	if (!pUniBufObj)
	{
		syserr("UBO_Reallocate: NULL buffer");
		return;
	}

	if (newSize <= 0)
	{
		syserr("UBO_Reallocate: Invalid size %lld", (long long)newSize);
		return;
	}

	if (newSize > siMaxUBOSize)
	{
		syserr("UBO_Reallocate '%s': Size %lld exceeds limit %d",
			pUniBufObj->szBufferName, (long long)newSize, siMaxUBOSize);
		return;
	}

	GLuint oldUBO = pUniBufObj->bufferID;
	GLsizeiptr oldUBOSize = pUniBufObj->bufferSize;

	GLuint newUBO;
	GL_CreateBuffer(&newUBO);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferStorage(newUBO, newSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, newUBO);
		glBufferData(GL_UNIFORM_BUFFER, newSize, NULL, GL_DYNAMIC_DRAW);
	}

	if (copyOldData && oldUBOSize > 0)
	{
		GLsizeiptr copySize = (oldUBOSize < newSize) ? oldUBOSize : newSize;
		// Copy min(oldSize, newSize) to avoid overflow

		if (IsGLVersionHigher(4, 5))
		{
			glCopyNamedBufferSubData(oldUBO, newUBO, 0, 0, copySize);
		}
		else
		{
			glBindBuffer(GL_COPY_READ_BUFFER, oldUBO);
			glBindBuffer(GL_COPY_WRITE_BUFFER, newUBO);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);
		}

		pUniBufObj->writeOffset = copySize;  // Continue after copied data
	}
	else
	{
		pUniBufObj->writeOffset = 0;  // Start from beginning
	}

	// Delete old buffer
	GL_DeleteBuffer(&oldUBO);

	// Assign new data
	pUniBufObj->bufferID = newUBO;
	pUniBufObj->bufferSize = newSize;

	UniformBufferObject_Bind(pUniBufObj);
}
