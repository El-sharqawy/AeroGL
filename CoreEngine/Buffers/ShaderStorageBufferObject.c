#include "ShaderStorageBufferObject.h"
#include "../Core/CoreUtils.h"
#include "Buffer.h"
#include <memory.h>

static GLint siMaxSSBOSize = 0;

bool InitializeShaderStorageBufferObject(ShaderStorageBufferObject* ppSSBO, GLsizeiptr bufferSize, GLuint bindingPt, const char* name)
{
    if (siMaxSSBOSize == 0)
    {
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &siMaxSSBOSize);
    }

    if (bufferSize > (GLsizeiptr)siMaxSSBOSize)
    {
        syserr("SSBO : Size %zu exceeds hardware limit of %d", bufferSize, siMaxSSBOSize);
        return (false);
    }

    if (bindingPt > SSBO_BP_MAX_NUM)
    {
        syserr("SSBO : Binding Point %d exceeds limit of %d", bindingPt, SSBO_BP_MAX_NUM);
        return (false);
    }

    *ppSSBO = (ShaderStorageBufferObject)tracked_malloc(sizeof(SShaderStorageBufferObject));

    ShaderStorageBufferObject pSSBO = *ppSSBO;

    if (!pSSBO)
    {
        syserr("Failed to create Shader Storage Buffer Object");
        return (false);
    }

    memset(pSSBO, 0, sizeof(SShaderStorageBufferObject));

    pSSBO->szBufferName = tracked_strdup(name);

    if (!CreateBuffer(&pSSBO->bufferID))
    {
        DestroyShaderStorageBufferObject(&pSSBO);
        syserr("Failed to Create GPU Buffers!");
        return (false);
    }

    if (IsGLVersionHigher(4, 5))
    {
        glNamedBufferStorage(pSSBO->bufferID, bufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    else
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pSSBO->bufferID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
    }

    pSSBO->bufferSize = bufferSize;
    pSSBO->bindingPoint = bindingPt;
    pSSBO->writeOffset = 0;

    ShaderStorageBufferObject_Bind(pSSBO);

    syslog("Created SSBO '%s': buffer=%u, size=%lld, binding=%u",
        name,
        pSSBO->bufferID,
        (long long)bufferSize,
        bindingPt);

    return (true);
}

void DestroyShaderStorageBufferObject(ShaderStorageBufferObject* ppSSBO)
{
    if (!ppSSBO || !*ppSSBO)
    {
        return;
    }

    ShaderStorageBufferObject pSSBO = *ppSSBO;

    if (pSSBO->szBufferName)
    {
        tracked_free(pSSBO->szBufferName);
    }

    DeleteBuffer(&pSSBO->bufferID);

    tracked_free(pSSBO);

    *ppSSBO = NULL;
}

void ShaderStorageBufferObject_Update(ShaderStorageBufferObject pSSBO, const void* pData, GLsizeiptr size, GLuint offset, bool bReallocation)
{
    if (!pSSBO || !pData || size <= 0)
    {
        return;
    }

    if (size > siMaxSSBOSize)
    {
        syserr("Size %zu is not allowed, Max: %d", size, siMaxSSBOSize);
        return;
    }

    if ((size + offset) > pSSBO->bufferSize)
    {
        if (bReallocation)
        {
            GLsizeiptr newSize = size + offset;
            ShaderStorageBufferObject_Reallocate(pSSBO, newSize, true);
        }
        else
        {
            syserr("SSBO '%s': Write at offset %lld + size %lld = %lld exceeds capacity %lld (reallocation disabled)",
                pSSBO->szBufferName,
                (long long)offset,
                (long long)size,
                (long long)(offset + size),
                (long long)pSSBO->bufferSize);  // Actual capacity
            return;
        }
    }

    // Update Part
    if (IsGLVersionHigher(4, 5))
    {
        glNamedBufferSubData(pSSBO->bufferID, offset, size, pData);
    }
    else
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pSSBO->bufferID);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, pData);
    }

    pSSBO->writeOffset = offset + size;  // Update offset after write
}

void ShaderStorageBufferObject_Reallocate(ShaderStorageBufferObject pSSBO, GLsizeiptr newSize, bool copyOldData)
{
    if (!pSSBO)
    {
        return;
    }

    if (newSize > siMaxSSBOSize)
    {
        syserr("Size %zu is not allowed, Max: %d", newSize, siMaxSSBOSize);
        return;
    }

    GLuint oldSSBO = pSSBO->bufferID;
    GLsizeiptr oldSSBOSize = pSSBO->bufferSize;

    GLuint newSSBO;
    CreateBuffer(&newSSBO);

    if (IsGLVersionHigher(4, 5))
    {
        glNamedBufferStorage(newSSBO, newSize, NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    else
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, NULL, GL_DYNAMIC_DRAW);
    }

    if (copyOldData && oldSSBOSize > 0)
    {
        GLsizeiptr copySize = (oldSSBOSize < newSize) ? oldSSBOSize : newSize;
        // Copy min(oldSize, newSize) to avoid overflow

        if (IsGLVersionHigher(4, 5))
        {
            glCopyNamedBufferSubData(oldSSBO, newSSBO, 0, 0, copySize);
        }
        else
        {
            glBindBuffer(GL_COPY_READ_BUFFER, oldSSBO);
            glBindBuffer(GL_COPY_WRITE_BUFFER, newSSBO);

            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);
        }

        pSSBO->writeOffset = copySize;  // Continue after copied data
    }
    else
    {
        pSSBO->writeOffset = 0;  // Start from beginning
    }

    // Delete Old Buffer
    DeleteBuffer(&oldSSBO);

    pSSBO->bufferID = newSSBO;
    pSSBO->bufferSize = newSize;

    ShaderStorageBufferObject_Bind(pSSBO);
}

void ShaderStorageBufferObject_Bind(ShaderStorageBufferObject pSSBO)
{
    if (!pSSBO)
    {
        return;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, pSSBO->bindingPoint, pSSBO->bufferID);
}

void ShaderStorageBufferObject_UnBind(ShaderStorageBufferObject pSSBO)
{
    if (!pSSBO)
    {
        return;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, pSSBO->bindingPoint, 0);
}
