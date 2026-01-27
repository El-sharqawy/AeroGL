#include "TerrainBuffer.h"
#include "../Core/CoreUtils.h"
#include "GLBuffer.h"
#include "../Terrain/TerrainData.h"

bool TerrainBuffer_Create(TerrainGLBuffer buffer)
{
	if (!buffer)
	{
		syserr("Terrain Buffer is NULL");
		return (false);
	}

	TerrainBuffer_Delete(buffer); // Prevent Duplication and Leaks

	if (!CreateVertexArray(&buffer->uiVAO))
	{
		syserr("Failed to Create Terrain Vertex Arrays");
		return (false);
	}

	GLuint buffers[2] = { 0, 0 };
	if (!CreateBuffers(buffers, 2))
	{
		syserr("Failed to Create Terrain Buffers");
		return (false);
	}

	buffer->uiVBO = buffers[0];
	buffer->uiEBO = buffers[1];

	// If the first one is 0, the driver failed to provide IDs
	if (buffer->uiVBO == 0 || buffer->uiEBO == 0)
	{
		TerrainBuffer_Delete(buffer); // Clean up if one failed
		syserr("Failed to Generate Terrain Buffer!");
		return (false);
	}

	return (true);
}

void TerrainBuffer_Delete(TerrainGLBuffer buffer)
{
	if (!buffer)
	{
		syserr("Terrain Buffer is NULL");
		return;
	}

	DeleteVertexArray(&buffer->uiVAO);
	DeleteBuffer(&buffer->uiVBO);
	DeleteBuffer(&buffer->uiEBO);

	buffer->bIsInitialized = false; // Reset the engine state flag
}

void TerrainBuffer_Reset(TerrainGLBuffer buffer)
{
	if (!buffer)
	{
		return;
	}

	// Reset write positions to beginning
	buffer->vertexOffset = 0;
	buffer->indexOffset = 0;
	buffer->vertexCount = 0;
	buffer->indexCount = 0;

	// GPU data is NOT touched - just reuse the space!
}

void TerrainBuffer_Clear(TerrainGLBuffer buffer)
{
	if (!buffer)
	{
		return;
	}

	// Reset CPU values
	TerrainBuffer_Reset(buffer);

	// Zero GPU memory (expensive!)
	if (IsGLVersionHigher(4, 3))
	{
		// Use R8 to clear byte-by-byte (works for any data structure)
		glClearNamedBufferSubData(buffer->uiVBO, GL_R8, 0, buffer->vboSize, GL_RED, GL_UNSIGNED_BYTE, NULL); // buffer->vboSize is Total Size (capacity * sizeof(element))
		glClearNamedBufferSubData(buffer->uiEBO, GL_R32UI, 0, buffer->eboSize, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	}
	else
	{
		// Use R8 to clear byte-by-byte (works for any data structure)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glClearBufferSubData(GL_ARRAY_BUFFER, GL_R8, 0, buffer->vboSize, GL_RED, GL_UNSIGNED_BYTE, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glClearBufferSubData(GL_ELEMENT_ARRAY_BUFFER, GL_R32UI, 0, buffer->eboSize, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	}

#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("TerrainBuffer_Clear failed with GL error: 0x%X", error);
	}
#endif
}

bool TerrainBuffer_Initialize(TerrainGLBuffer* ppTerrainBuffer)
{
	*ppTerrainBuffer = (TerrainGLBuffer)tracked_calloc(1, sizeof(STerrainGLBuffer));

	TerrainGLBuffer buffer = *ppTerrainBuffer;
	if (!buffer)
	{
		syserr("Failed to Allocate Memory for Terrain Buffer");
		return (false);
	}

	if (!TerrainBuffer_Create(buffer))
	{
		TerrainBuffer_Destroy(&buffer);
		return (false);
	}

	buffer->vboCapacity = TERRAIN_PATCH_COUNT * 1024;
	buffer->eboCapacity = TERRAIN_PATCH_COUNT * 1536;

	buffer->vboSize = buffer->vboCapacity * sizeof(STerrainVertex);
	buffer->eboSize = buffer->eboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		buffer->bufferStorageType = GL_DYNAMIC_STORAGE_BIT;
	}
	else
	{
		buffer->bufferStorageType = GL_STATIC_DRAW;
	}

	if (!TerrainBuffer_AllocateGPUStorage(buffer))
	{
		TerrainBuffer_Destroy(&buffer);
		return (false);
	}

	TerrainBuffer_LinkBuffers(buffer);
	return (true);
}

void TerrainBuffer_Destroy(TerrainGLBuffer* ppTerrainBuffer)
{
	if (!ppTerrainBuffer || !*ppTerrainBuffer)
	{
		return;
	}

	// Free GPU Resources
	TerrainBuffer_Delete(*ppTerrainBuffer);

	tracked_free(*ppTerrainBuffer);
	*ppTerrainBuffer = NULL;
}

bool TerrainBuffer_AllocateGPUStorage(TerrainGLBuffer pTerrainBuffer)
{
	if (!pTerrainBuffer)
	{
		syserr("Terrain Buffer is NULL");
		return (false);
	}

	if (IsGLVersionHigher(4, 5))
	{
		// Immutable Storage !
		glNamedBufferStorage(pTerrainBuffer->uiVBO, pTerrainBuffer->vboSize, NULL, pTerrainBuffer->bufferStorageType);
		glNamedBufferStorage(pTerrainBuffer->uiEBO, pTerrainBuffer->eboSize, NULL, pTerrainBuffer->bufferStorageType);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, pTerrainBuffer->uiVBO);
		glBufferData(GL_ARRAY_BUFFER, pTerrainBuffer->vboSize, NULL, pTerrainBuffer->bufferStorageType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->uiEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->eboSize, NULL, pTerrainBuffer->bufferStorageType);
	}
	
#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("TerrainBuffer_AllocateGPUStorage failed with GL error: 0x%X", error);
		return (false);
	}
#endif

	return (true);
}

bool TerrainBuffer_AllocateVertexBuffer(TerrainGLBuffer pTerrainBuffer)
{
	if (!pTerrainBuffer)
	{
		return (false);
	}

	if (pTerrainBuffer->uiVAO == 0)
	{
		syserr("did you forget to create vertex arrays?!");
		return (false);
	}

	const GLint iPosition = 0;
	const GLint iNormals = 1;
	const GLint iTexCoord = 2;
	const GLint iColors = 3;

	GLuint byteOffset = 0;

	if (IsGLVersionHigher(4, 5))
	{
		glEnableVertexArrayAttrib(pTerrainBuffer->uiVAO, iPosition);
		glVertexArrayAttribFormat(pTerrainBuffer->uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexArrayAttribBinding(pTerrainBuffer->uiVAO, iPosition, 0);

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexArrayAttrib(pTerrainBuffer->uiVAO, iNormals);
		glVertexArrayAttribFormat(pTerrainBuffer->uiVAO, iNormals, 3, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexArrayAttribBinding(pTerrainBuffer->uiVAO, iNormals, 0);

		byteOffset += 4 * sizeof(GLfloat);

		glEnableVertexArrayAttrib(pTerrainBuffer->uiVAO, iTexCoord);
		glVertexArrayAttribFormat(pTerrainBuffer->uiVAO, iTexCoord, 2, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexArrayAttribBinding(pTerrainBuffer->uiVAO, iTexCoord, 0);

		byteOffset += 4 * sizeof(GLfloat);

		glEnableVertexArrayAttrib(pTerrainBuffer->uiVAO, iColors);
		glVertexArrayAttribFormat(pTerrainBuffer->uiVAO, iColors, 4, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexArrayAttribBinding(pTerrainBuffer->uiVAO, iColors, 0);

		byteOffset += 4 * sizeof(GLfloat);
	}
	else if (IsGLVersionHigher(4, 3))
	{
		glEnableVertexAttribArray(iPosition);
		glVertexAttribFormat(iPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexAttribBinding(iPosition, 0);

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iNormals);
		glVertexAttribFormat(iNormals, 3, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexAttribBinding(iNormals, 0);

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iTexCoord);
		glVertexAttribFormat(iTexCoord, 2, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexAttribBinding(iTexCoord, 0);

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iColors);
		glVertexAttribFormat(iColors, 4, GL_FLOAT, GL_FALSE, byteOffset);
		glVertexAttribBinding(iColors, 0);

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)
	}
	else
	{
		glEnableVertexAttribArray(iPosition);
		glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(STerrainVertex), (const void*)((GLsizeiptr)byteOffset));

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iNormals);
		glVertexAttribPointer(iNormals, 3, GL_FLOAT, GL_FALSE, sizeof(STerrainVertex), (const void*)((GLsizeiptr)byteOffset));

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iTexCoord);
		glVertexAttribPointer(iTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(STerrainVertex), (const void*)((GLsizeiptr)byteOffset));

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)

		glEnableVertexAttribArray(iColors);
		glVertexAttribPointer(iColors, 4, GL_FLOAT, GL_FALSE, sizeof(STerrainVertex), (const void*)((GLsizeiptr)byteOffset));

		byteOffset += 4 * sizeof(GLfloat); // Move by 16 BYTES for alignment (SIMD)
	}

	return (true);
}

bool TerrainBuffer_LinkBuffers(TerrainGLBuffer pTerrainBuffer)
{
	if (!pTerrainBuffer)
	{
		return (false);
	}

	if (IsGLVersionHigher(4, 5))
	{
		glVertexArrayVertexBuffer(pTerrainBuffer->uiVAO, 0, pTerrainBuffer->uiVBO, 0, sizeof(STerrainVertex));
		glVertexArrayElementBuffer(pTerrainBuffer->uiVAO, pTerrainBuffer->uiEBO);
	}
	else if (IsGLVersionHigher(4, 3))
	{
		glBindVertexArray(pTerrainBuffer->uiVAO);
		glBindVertexBuffer(0, pTerrainBuffer->uiVBO, 0, sizeof(STerrainVertex));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->uiEBO);
	}
	else
	{
		// Define Attributes (VBO must be bound to GL_ARRAY_BUFFER)
		glBindVertexArray(pTerrainBuffer->uiVAO);
		glBindBuffer(GL_ARRAY_BUFFER, pTerrainBuffer->uiVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->uiEBO);
	}
	TerrainBuffer_AllocateVertexBuffer(pTerrainBuffer);

#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("TerrainBuffer_LinkBuffers failed: 0x%X", error);
		return false;
	}
#endif

	return (true);
}

bool TerrainBuffer_Reallocate(TerrainGLBuffer pTerrainBuffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData)
{
	if (!pTerrainBuffer)
	{
		return (false);
	}

	if (newVboCapacity <= pTerrainBuffer->vboCapacity && newEboCapacity <= pTerrainBuffer->eboCapacity)
	{
		return (true); // ????? same size!
	}

	// Store old GPU Buffers Data
	GLuint oldVBO = pTerrainBuffer->uiVBO;
	GLuint oldEBO = pTerrainBuffer->uiEBO;

	// New GPU Buffers
	GLuint newBuffers[2] = { 0, 0 }; // 0 -> VBO , 1 -> EBO
	if (!CreateBuffers(newBuffers, 2))
	{
		return (false); // Failed to Create GPU Buffers
	}

	// Update Buffer Bytes Size
	pTerrainBuffer->vboSize = newVboCapacity * sizeof(STerrainVertex);
	pTerrainBuffer->eboSize = newEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Immutable Storage !
		glNamedBufferStorage(newBuffers[0], pTerrainBuffer->vboSize, NULL, pTerrainBuffer->bufferStorageType);
		glNamedBufferStorage(newBuffers[1], pTerrainBuffer->eboSize, NULL, pTerrainBuffer->bufferStorageType);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, newBuffers[0]);
		glBufferData(GL_ARRAY_BUFFER, pTerrainBuffer->vboSize, NULL, pTerrainBuffer->bufferStorageType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newBuffers[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->eboSize, NULL, pTerrainBuffer->bufferStorageType);
	}

#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("TerrainBuffer_Reallocate failed with GL error: 0x%X", error);
		return (false);
	}
#endif

	// 1. Keep track of how much valid data we actually have right now
	GLsizeiptr currentVertexCount = pTerrainBuffer->vertexOffset;
	GLsizeiptr currentIndexCount = pTerrainBuffer->indexOffset;

	// Copy Data
	if (copyOldData)
	{
		GLsizeiptr bytesToCopyVBO = currentVertexCount * sizeof(STerrainVertex);
		if (bytesToCopyVBO > 0)
		{
			if (IsGLVersionHigher(4, 5))
			{
				glCopyNamedBufferSubData(oldVBO, newBuffers[0], 0, 0, bytesToCopyVBO);
			}
			else
			{
				glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newBuffers[0]);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, bytesToCopyVBO);
			}
		}

		GLsizeiptr bytesToCopyEBO = currentIndexCount * sizeof(GLuint);
		if (bytesToCopyEBO > 0)
		{
			if (IsGLVersionHigher(4, 5))
			{
				glCopyNamedBufferSubData(oldEBO, newBuffers[1], 0, 0, bytesToCopyEBO);
			}
			else
			{
				glBindBuffer(GL_COPY_READ_BUFFER, oldEBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newBuffers[1]);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, bytesToCopyEBO);
			}
		}
	}

	// Clear Up Old Buffer
	DeleteBuffer(&oldVBO);
	DeleteBuffer(&oldEBO);

	// assign new Data
	pTerrainBuffer->uiVBO = newBuffers[0];
	pTerrainBuffer->uiEBO = newBuffers[1];

	pTerrainBuffer->vboCapacity = newVboCapacity;
	pTerrainBuffer->eboCapacity = newEboCapacity;

	if (!copyOldData)
	{
		// Clear all counters if not copying data
		pTerrainBuffer->vertexCount = 0;
		pTerrainBuffer->indexCount = 0;
		pTerrainBuffer->vertexOffset = 0;
		pTerrainBuffer->indexOffset = 0;
	}

	TerrainBuffer_LinkBuffers(pTerrainBuffer);
	return (true);
}

bool TerrainBuffer_UploadData(TerrainGLBuffer pTerrainBuffer, TerrainMesh pTerrainMesh)
{
	if (!pTerrainBuffer || !pTerrainMesh || !pTerrainMesh->pVertices || !pTerrainMesh->pIndices)
	{
		syserr("Terrain Buffer or Data is NULL");
		return false;
	}

	// Update Counts and Offsets
	pTerrainMesh->indexOffset = GetTerrainBufferIndexOffset(pTerrainBuffer);
	pTerrainMesh->vertexOffset = GetTerrainBufferVertexOffset(pTerrainBuffer);

	const STerrainVertex* pVertices = (STerrainVertex*)pTerrainMesh->pVertices->pData;
	const GLuint* pIndices = (GLuint*)pTerrainMesh->pIndices->pData;
	GLsizeiptr vertexCount = pTerrainMesh->vertexCount;
	GLsizeiptr indexCount = pTerrainMesh->indexCount;

	// Check Capacity
	GLsizeiptr requiredVboCapacity = pTerrainBuffer->vertexOffset + vertexCount; // total VBO size after upload
	GLsizeiptr requiredEboCapacity = pTerrainBuffer->indexOffset + indexCount;   // total EBO size after upload

	if (pTerrainBuffer->vboCapacity < requiredVboCapacity || pTerrainBuffer->eboCapacity < requiredEboCapacity)
	{
		// Reallocate with extra space to avoid frequent reallocations
		GLsizeiptr newVboCapacity = (requiredVboCapacity > pTerrainBuffer->vboCapacity) ? requiredVboCapacity * 2 : pTerrainBuffer->vboCapacity;
		GLsizeiptr newEboCapacity = (requiredEboCapacity > pTerrainBuffer->eboCapacity) ? requiredEboCapacity * 2 : pTerrainBuffer->eboCapacity;
		if (!TerrainBuffer_Reallocate(pTerrainBuffer, newVboCapacity, newEboCapacity, true))
		{
			syserr("Failed to Reallocate Terrain Buffer");
			return (true);
		}
		else
		{
			syslog("Attemp to reallocate buffer .. new VBO size: %zu - new VEO size: %zu", newVboCapacity, newEboCapacity);
		}
	}

	// Calculate byte offsets for glBufferSubData
	GLsizeiptr vertexByteOffset = pTerrainBuffer->vertexOffset * sizeof(STerrainVertex);
	GLsizeiptr indexByteOffset = pTerrainBuffer->indexOffset * sizeof(GLuint);

	GLsizeiptr vertexByteSize = vertexCount * sizeof(STerrainVertex);
	GLsizeiptr indexByteSize = indexCount * sizeof(GLuint);

	// Upload Vertex Data
	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(pTerrainBuffer->uiVBO, vertexByteOffset, vertexByteSize, pVertices);
		glNamedBufferSubData(pTerrainBuffer->uiEBO, indexByteOffset, indexByteSize, pIndices);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, pTerrainBuffer->uiVBO);
		glBufferSubData(GL_ARRAY_BUFFER, vertexByteOffset, vertexByteSize, pVertices);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainBuffer->uiEBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexByteOffset, indexByteSize, pIndices);
	}

	// Update Offsets
	pTerrainBuffer->vertexOffset += vertexCount;
	pTerrainBuffer->indexOffset += indexCount;

	// Update total counts
	pTerrainBuffer->vertexCount = (GLuint)pTerrainBuffer->vertexOffset;
	pTerrainBuffer->indexCount = (GLuint)pTerrainBuffer->indexOffset;

	return (true);
}

GLsizeiptr GetTerrainBufferVertexOffset(TerrainGLBuffer pTerrainBuffer)
{
	return (pTerrainBuffer->vertexOffset);
}

GLsizeiptr GetTerrainBufferIndexOffset(TerrainGLBuffer pTerrainBuffer)
{
	return (pTerrainBuffer->indexOffset);
}

GLuint TerrainBuffer_GetVertexArray(TerrainGLBuffer pTerrainBuffer)
{
	return pTerrainBuffer->uiVAO;
}