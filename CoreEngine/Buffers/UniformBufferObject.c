#include "UniformBufferObject.h"
#include "../Core/CoreUtils.h"
#include "Buffer.h"
#include <memory.h>

static GLint siMaxUBOSize = 0;

bool InitializeUniformBufferObject(UniformBufferObject* ppUniBufObj, GLsizeiptr bufferSize, GLuint bindingPt, const char* szBufferName)
{
	if (siMaxUBOSize == 0)
	{
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &siMaxUBOSize);
	}

	if (bufferSize > (GLsizeiptr)siMaxUBOSize)
	{
		syserr("UBO: Size %d exceeds hardware limit of %d", (GLint)bufferSize, siMaxUBOSize);
		return (false);
	}

	*ppUniBufObj = (UniformBufferObject)tracked_malloc(sizeof(SUniformBufferObject));
	UniformBufferObject buffer = *ppUniBufObj;

	// Init to Zeros
	memset(buffer, 0, sizeof(SUniformBufferObject));

	buffer->szBufferName = tracked_strdup(szBufferName);

	CreateBuffer(&buffer->bufferID);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferStorage(buffer->bufferID, bufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, buffer->bufferID);
		glBufferData(GL_UNIFORM_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
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
		tracked_free(buffer->szBufferName);
	}

	DeleteBuffer(&buffer->bufferID); // Clear UP GPU Resources

	tracked_free(buffer);

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
	CreateBuffer(&newUBO);

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
	DeleteBuffer(&oldUBO);

	// Assign new data
	pUniBufObj->bufferID = newUBO;
	pUniBufObj->bufferSize = newSize;

	UniformBufferObject_Bind(pUniBufObj);
}
