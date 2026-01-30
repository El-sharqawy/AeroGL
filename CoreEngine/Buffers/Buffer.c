#include "Buffer.h"
#include "../Core/CoreUtils.h"
#include <memory.h>
#include <stddef.h> // Required for offsetof
#include "../PipeLine/StateManager.h"
#include "GLBuffer.h"

typedef struct SGLBuffer
{
	GLuint uiVAO;			// The Vertex Array Object (The "Boss" handle)
	GLuint uiVBO;			// The Vertex Buffer Object (The actual data)
	GLuint uiEBO;			// The Element Buffer (Optional, for optimized drawing)

	GLuint vertexCount;		// glDrawArrays needs it
	GLuint indexCount;		// glDrawElements needs it

	GLsizeiptr vboCapacity;	// VBO size (capacity)
	GLsizeiptr eboCapacity;	// EBO size (capacity)

	GLsizeiptr vertexOffset; // total written vertices offset
	GLsizeiptr indexOffset;	// total written indices offset

	GLsizeiptr vboSize; // Total Size (capacity * sizeof(element))
	GLsizeiptr eboSize; // Total Size (capacity * sizeof(GLuint))

	GLenum bufferStorageType;

	bool bIsInitialized;
} SGLBuffer;

void GLBuffer_Delete(GLBuffer buffer)
{
	if (!buffer)
	{
		return;
	}

	DeleteVertexArray(&buffer->uiVAO);

	// Explicitly delete both to be safe against future struct changes
	DeleteBuffer(&buffer->uiVBO);
	DeleteBuffer(&buffer->uiEBO);

	buffer->bIsInitialized = false; // Reset the engine state flag
}

bool GLBuffer_Create(GLBuffer buffer)
{
	if (!buffer)
	{
		syslog("Attemp to Create NULL buffer");
		return (false);
	}

	// Prevent Leaks: Clean up if this group was already used
	GLBuffer_Delete(buffer);

	// 1. VAO is a separate type - Create it first
	if (!CreateVertexArray(&buffer->uiVAO))
	{
		syserr("Failed to Create Buffer Vertex Array");
		return (false);
	}

	// 2. VBO and EBO are Buffers - We can batch these safely
	GLuint ids[2] = { 0, 0 };

	// Create VBO and EBO
	CreateBuffers(ids, 2);

	// 3. Assign them back to the struct
	buffer->uiVBO = ids[0];
	buffer->uiEBO = ids[1];

	// If the first one is 0, the driver failed to provide IDs
	if (buffer->uiVBO == 0 || buffer->uiEBO == 0)
	{
		GLBuffer_Delete(buffer); // Clean up if one failed
		syserr("Failed to generate Buffer!");
		return (false);
	}

	return (true);
}

void GLBuffer_ResetBuffer(GLBuffer buffer)
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

/**
 * @brief Resets buffer AND zeroes GPU memory.
 * WARNING: Slow! Only use for debugging or security.
 */
void GLBuffer_ClearBuffer(GLBuffer buffer)
{
	if (!buffer)
	{
		syserr("ClearBuffer: NULL buffer");
		return;
	}

	// Reset cursors
	GLBuffer_ResetBuffer(buffer);

	// Zero GPU memory (expensive!)
	if (IsGLVersionHigher(4, 3))
	{
		// Use glClearBufferData (OpenGL 4.3+)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glClearBufferData(GL_ARRAY_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glClearBufferData(GL_ELEMENT_ARRAY_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, NULL);
	}
	else
	{
		// Fallback: Upload zeros (very slow!)
		size_t vertexBytes = buffer->vboCapacity * sizeof(SVertex3D);
		size_t indexBytes = buffer->eboCapacity * sizeof(GLuint);

		void* zeroVerts = calloc(buffer->vboCapacity, sizeof(SVertex3D));
		void* zeroIndices = calloc(buffer->eboCapacity, sizeof(GLuint));

		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBytes, zeroVerts);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBytes, zeroIndices);

		free(zeroVerts);
		free(zeroIndices);
	}

	syslog("Buffer cleared (GPU memory zeroed)");
}

void GLBuffer_DestroyBuffer(GLBuffer* ppBuffer)
{
	if (!ppBuffer || !*ppBuffer)
	{
		return;
	}

	GLBuffer pBuffer = *ppBuffer;

	GLBuffer_Delete(pBuffer);
	tracked_free(pBuffer);

	*ppBuffer = NULL;
}

bool GLBuffer_AllocateStorage(GLBuffer buffer)
{
	if (buffer->uiVBO == 0 || buffer->uiEBO == 0)
	{
		syserr("Called with invalid VBO (%u) or EBO (%u)!", buffer->uiVBO, buffer->uiEBO);
		return (false);
	}

	GLsizeiptr vboBytes = buffer->vboCapacity * sizeof(SVertex);
	GLsizeiptr eboBytes = buffer->eboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// IMPORTANT: glNamedBufferStorage is IMMUTABLE. 
		// If the buffer was already allocated, we MUST delete and recreate the ID.
		// Allocation (Immutable) - Ensure ID is fresh or deleted/recreated before calling
		glNamedBufferStorage(buffer->uiVBO, vboBytes, NULL, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(buffer->uiEBO, eboBytes, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		// Allocation (Mutable)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferData(GL_ARRAY_BUFFER, vboBytes, NULL, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboBytes, NULL, GL_DYNAMIC_DRAW);
	}

	return (true);
}

void GLBuffer_LinkBuffers(GLBuffer buffer)
{
	if (!buffer)
	{
		syslog("Attemp to Link NULL buffer");
		return;
	}

	// DSA Linking
	if (IsGLVersionHigher(4, 5))
	{
		glVertexArrayVertexBuffer(buffer->uiVAO, 0, buffer->uiVBO, 0, sizeof(SVertex));
		glVertexArrayElementBuffer(buffer->uiVAO, buffer->uiEBO);
	}
	else
	{
		glBindVertexArray(buffer->uiVAO);
		// Bind and Allocate VBO and EBO (This also links it to the VAO permanently)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);

		// If we changed the VBO, we must re-inform the VAO where the pointers are.
		GLBuffer_AllocateVertexBuffer(buffer);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->bIsInitialized = true;
}

void GLBuffer_AllocateVertexBuffer(GLBuffer buffer)
{
	if (!buffer)
	{
		syslog("Attemp to Setup NULL buffer");
		return;
	}

	GLint iPosition = 0;
	GLint iNormals = 1;
	GLint iTexCoord = 2;
	GLint iColors = 3;

	GLuint sNumFloats = 0;

	// Attributes, we won't use offsetof for now
	if (IsGLVersionHigher(4, 5))
	{
		// Position
		glEnableVertexArrayAttrib(buffer->uiVAO, iPosition);
		glVertexArrayAttribFormat(buffer->uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, sNumFloats * sizeof(GLfloat));
		glVertexArrayAttribBinding(buffer->uiVAO, iPosition, 0);

		sNumFloats += 4; // Skip the 3 floats + 1 padding float

		// Normals
		glEnableVertexArrayAttrib(buffer->uiVAO, iNormals);
		glVertexArrayAttribFormat(buffer->uiVAO, iNormals, 3, GL_FLOAT, GL_FALSE, sNumFloats * sizeof(GLfloat));
		glVertexArrayAttribBinding(buffer->uiVAO, iNormals, 0);

		sNumFloats += 4; // Skip the 3 floats + 1 padding float

		// Tex Coords
		glEnableVertexArrayAttrib(buffer->uiVAO, iTexCoord);
		glVertexArrayAttribFormat(buffer->uiVAO, iTexCoord, 2, GL_FLOAT, GL_FALSE, sNumFloats * sizeof(GLfloat));
		glVertexArrayAttribBinding(buffer->uiVAO, iTexCoord, 0);

		sNumFloats += 4; // We skip 4 floats here because Color is aligned to 16 bytes

		// Colors
		glEnableVertexArrayAttrib(buffer->uiVAO, iColors);
		glVertexArrayAttribFormat(buffer->uiVAO, iColors, 4, GL_FLOAT, GL_FALSE, sNumFloats * sizeof(GLfloat));
		glVertexArrayAttribBinding(buffer->uiVAO, iColors, 0);

		sNumFloats += 4; // Assign 4 floats since colors type of Vec4;
	}
	else
	{
		// Define Attributes (VBO must be bound to GL_ARRAY_BUFFER)
		glEnableVertexAttribArray(iPosition);
		glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), (const void*)(sNumFloats * sizeof(GLfloat)));

		sNumFloats += 4; // Skip the 3 floats + 1 padding float

		glEnableVertexAttribArray(iNormals);
		glVertexAttribPointer(iNormals, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), (const void*)(sNumFloats * sizeof(GLfloat)));

		sNumFloats += 4; // Skip the 3 floats + 1 padding float

		glEnableVertexAttribArray(iTexCoord);
		glVertexAttribPointer(iTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), (const void*)(sNumFloats * sizeof(GLfloat)));

		sNumFloats += 4; // We skip 4 floats here because Color is aligned to 16 bytes

		glEnableVertexAttribArray(iColors);
		glVertexAttribPointer(iColors, 4, GL_FLOAT, GL_FALSE, sizeof(SVertex), (const void*)(sNumFloats * sizeof(GLfloat)));

		sNumFloats += 4; // Assign 4 floats since colors type of Vec4;
	}
}

void GLBuffer_UploadDataPtr(GLBuffer buffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount)
{
	if (!pVertices || !pIndices || vertexCount == 0)
	{
		syserr("Called with null vertex or index data!");
		return;
	}

	if (vertexCount > buffer->vboCapacity || indexCount > buffer->eboCapacity)
	{
		// Grow by 50% or the required amount, whichever is larger
		GLsizeiptr newVboCap = buffer->vboCapacity + (buffer->vboCapacity / 2);
		if (newVboCap < vertexCount)
		{
			newVboCap = vertexCount;
		}

		GLsizeiptr newEboCap = buffer->eboCapacity + (buffer->eboCapacity / 2);
		if (newEboCap < indexCount)
		{
			newEboCap = indexCount;
		}

		GLBuffer_Reallocate(buffer, newVboCap, newEboCap, false);
		syslog("Attemp to Reallocate Buffer...");
	}

	// Calculate byte sizes automatically based on the template type T
	GLsizeiptr vboByteSize = vertexCount * sizeof(SVertex);
	GLsizeiptr eboByteSize = indexCount * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// DSA Path: Modern Invalidation
		glNamedBufferSubData(buffer->uiVBO, 0, vboByteSize, pVertices);
		glNamedBufferSubData(buffer->uiEBO, 0, eboByteSize, pIndices);
	}
	else
	{
		// Legacy Path: Manual Orphaning
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vboByteSize, pVertices);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboByteSize, pIndices);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->vertexCount = (GLuint)vertexCount;
	buffer->indexCount = (GLuint)indexCount;
}

bool GLBuffer_Initialize(GLBuffer* ppBuffer)
{
	*ppBuffer = (GLBuffer)tracked_malloc(sizeof(SGLBuffer));

	GLBuffer pGLBuffer = *ppBuffer;
	if (!pGLBuffer)
	{
		syserr("Failed to Allocate Space for GLBuffer");
		return (false);
	}

	// make sure it's all elements set to zero bytes
	memset(pGLBuffer, 0, sizeof(SGLBuffer));

	// 1. Setup the Metadata inside the struct
	pGLBuffer->vboCapacity = INITIAL_VERTEX_CAPACITY;
	pGLBuffer->eboCapacity = INITIAL_INDEX_CAPACITY;
	pGLBuffer->bIsInitialized = false;

	// 2. Create the GPU "Names" (IDs)
	if (GLBuffer_Create(pGLBuffer) == false)
	{
		GLBuffer_Delete(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return false;
	}

	if (IsGLVersionHigher(4, 5))
	{
		pGLBuffer->bufferStorageType = GL_DYNAMIC_STORAGE_BIT;
	}
	else
	{
		pGLBuffer->bufferStorageType = GL_STATIC_DRAW;
	}

	// 3. Allocate the actual VRAM
	// This uses the metadata we just set to call glNamedBufferStorage or glBufferData
	if (GLBuffer_AllocateStorage(pGLBuffer) == false)
	{
		GLBuffer_Delete(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return false;
	}

	// 4. Define the Vertex Layout (What does a vertex look like?)
	// This tells the VAO how to interpret the data (Position, Color, etc.)
	GLBuffer_AllocateVertexBuffer(pGLBuffer);

	// 5. Physical Link (Which buffers belong to this VAO?)
	// This tells the VAO which specific VBO/EBO IDs to pull from
	GLBuffer_LinkBuffers(pGLBuffer);

	// 6. Return the Created Buffer
	return (true);
}

/**
 * @brief Reallocates buffer storage with optional data preservation.
 *
 * Creates new VBO/EBO with larger capacity and optionally copies old data.
 * Uses GPU-to-GPU copy to avoid CPU round-trip.
 *
 * @param buffer [in/out] The buffer to resize.
 * @param newVboCapacity [in] New vertex capacity.
 * @param newEboCapacity [in] New index capacity.
 * @param copyOldData [in] If true, preserves existing data; if false, starts fresh.
 */
bool GLBuffer_Reallocate(GLBuffer pBuffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData)
{
	if (!pBuffer)
	{
		return (false);
	}

	if (newVboCapacity <= pBuffer->vboCapacity && newEboCapacity <= pBuffer->eboCapacity)
	{
		return (true); // ????? same size!
	}

	// Store old GPU Buffers Data
	GLuint oldVBO = pBuffer->uiVBO;
	GLuint oldEBO = pBuffer->uiEBO;

	// New GPU Buffers
	GLuint newBuffers[2] = { 0, 0 }; // 0 -> VBO , 1 -> EBO
	if (!CreateBuffers(newBuffers, 2))
	{
		return (false); // Failed to Create GPU Buffers
	}

	// Update Buffer Bytes Size
	pBuffer->vboSize = newVboCapacity * sizeof(SVertex3D);
	pBuffer->eboSize = newEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Immutable Storage !
		glNamedBufferStorage(newBuffers[0], pBuffer->vboSize, NULL, pBuffer->bufferStorageType);
		glNamedBufferStorage(newBuffers[1], pBuffer->eboSize, NULL, pBuffer->bufferStorageType);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, newBuffers[0]);
		glBufferData(GL_ARRAY_BUFFER, pBuffer->vboSize, NULL, pBuffer->bufferStorageType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newBuffers[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pBuffer->eboSize, NULL, pBuffer->bufferStorageType);
	}

#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("GLBuffer_Reallocate failed with GL error: 0x%X", error);
		return (false);
	}
#endif

	// 1. Keep track of how much valid data we actually have right now
	GLsizeiptr currentVertexCount = pBuffer->vertexOffset;
	GLsizeiptr currentIndexCount = pBuffer->indexOffset;

	// Copy Data
	if (copyOldData)
	{
		GLsizeiptr bytesToCopyVBO = currentVertexCount * sizeof(SVertex3D);
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
	pBuffer->uiVBO = newBuffers[0];
	pBuffer->uiEBO = newBuffers[1];

	pBuffer->vboCapacity = newVboCapacity;
	pBuffer->eboCapacity = newEboCapacity;

	if (!copyOldData)
	{
		// Clear all counters if not copying data
		pBuffer->vertexCount = 0;
		pBuffer->indexCount = 0;
		pBuffer->vertexOffset = 0;
		pBuffer->indexOffset = 0;
	}

	GLBuffer_LinkBuffers(pBuffer);
	return (true);
}

/////////////// Mesh3D GL Buffer //////////////////////////
bool Mesh3DGLBuffer_AllocateStorage(GLBuffer buffer)
{
	if (buffer->uiVBO == 0 || buffer->uiEBO == 0)
	{
		syserr("Called with invalid VBO (%u) or EBO (%u)!", buffer->uiVBO, buffer->uiEBO);
		return (false);
	}

	GLsizeiptr vboSize = buffer->vboCapacity * sizeof(SVertex3D);
	GLsizeiptr eboSize = buffer->eboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Immutable Data, can't resize .. MUST DELETE (Re-Allocate)
		glNamedBufferStorage(buffer->uiVBO, vboSize, NULL, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(buffer->uiEBO, eboSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		// Allocation (Mutable)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferData(GL_ARRAY_BUFFER, vboSize, NULL, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboSize, NULL, GL_DYNAMIC_DRAW);
	}

	return (true);
}

void Mesh3DGLBuffer_LinkBuffers(GLBuffer buffer)
{
	if (!buffer)
	{
		syslog("Attemp to Link NULL buffer");
		return;
	}

	// DSA Linking
	if (IsGLVersionHigher(4, 5))
	{
		glVertexArrayVertexBuffer(buffer->uiVAO, 0, buffer->uiVBO, 0, sizeof(SVertex3D));
		glVertexArrayElementBuffer(buffer->uiVAO, buffer->uiEBO);
	}
	else
	{
		glBindVertexArray(buffer->uiVAO);
		// Bind and Allocate VBO and EBO (This also links it to the VAO permanently)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);

		// If we changed the VBO, we must re-inform the VAO where the pointers are.
		Mesh3DGLBuffer_AllocateVertexBuffer(buffer);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->bIsInitialized = true;
}

void Mesh3DGLBuffer_AllocateVertexBuffer(GLBuffer buffer)
{
	if (!buffer)
	{
		return;
	}

	GLint iPosition = 0;
	GLint iNormals = 1;
	GLint iTexCoords = 2;
	GLint iColors = 3;

	GLuint numFloats = 0;

	if (IsGLVersionHigher(4, 5))
	{
		// Enable Attributes in Vertex Array object (VAO)
		glEnableVertexArrayAttrib(buffer->uiVAO, iPosition);
		glEnableVertexArrayAttrib(buffer->uiVAO, iNormals);
		glEnableVertexArrayAttrib(buffer->uiVAO, iTexCoords);
		glEnableVertexArrayAttrib(buffer->uiVAO, iColors);

		// We will Apply 16 Bytes on all components as (4 floats) since we are using SIMD .. 
		glVertexArrayAttribFormat(buffer->uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		glVertexArrayAttribFormat(buffer->uiVAO, iNormals, 3, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		glVertexArrayAttribFormat(buffer->uiVAO, iTexCoords, 2, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 2 Floats (U, V)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		glVertexArrayAttribFormat(buffer->uiVAO, iColors, 4, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 4 flaots (r,g,b,a)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)

		// Bind Attributes to VAO
		glVertexArrayAttribBinding(buffer->uiVAO, iPosition, 0);
		glVertexArrayAttribBinding(buffer->uiVAO, iNormals, 0);
		glVertexArrayAttribBinding(buffer->uiVAO, iTexCoords, 0);
		glVertexArrayAttribBinding(buffer->uiVAO, iColors, 0);
	}
	else
	{
		glEnableVertexAttribArray(iPosition);

		// We will Apply 16 Bytes on all components as (4 floats) since we are using SIMD .. 
		glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		
		// We will Apply 16 Bytes on all components as (4 floats) since we are using SIMD .. 
		glEnableVertexAttribArray(iNormals);
		glVertexAttribPointer(iNormals, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)

		glEnableVertexAttribArray(iTexCoords);
		glVertexAttribPointer(iTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 2 Floats (U, V)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		
		glEnableVertexAttribArray(iColors);
		glVertexAttribPointer(iColors, 4, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 4 flaots (r,g,b,a)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
	}
}

void Mesh3DGLBuffer_UploadDataPtr(GLBuffer buffer, const SVertex3D* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount)
{
	if (!buffer)
	{
		syserr("Buffer Is Null!");
		return;
	}

	if (!pVertices || !pIndices)
	{
		syserr("Called with null Mesh3D vertex or index data!");
		return;
	}

	// Check if we need to reallocate
	GLsizeiptr newVertexCount = buffer->vertexOffset + vertexCount;
	GLsizeiptr newIndexCount = buffer->indexOffset + indexCount;

	if (newVertexCount > buffer->vboCapacity || newIndexCount > buffer->eboCapacity)
	{
		// Grow by 50% or the required amount, whichever is larger
		GLsizeiptr newVboCap = buffer->vboCapacity + (buffer->vboCapacity / 2);
		if (newVboCap < newVertexCount)
		{
			newVboCap = newVertexCount;
		}

		GLsizeiptr newEboCap = buffer->eboCapacity + (buffer->eboCapacity / 2);
		if (newEboCap < newIndexCount)
		{
			newEboCap = newIndexCount;
		}

		Mesh3DGLBuffer_Reallocate(buffer, newVboCap, newEboCap, true);
		syslog("Attemp to Reallocate Buffer...");
	}

	// Calculate byte offsets for glBufferSubData
	GLsizeiptr vertexByteOffset = buffer->vertexOffset * sizeof(SVertex3D);
	GLsizeiptr indexByteOffset = buffer->indexOffset * sizeof(GLuint);

	GLsizeiptr vertexByteSize = vertexCount * sizeof(SVertex3D);
	GLsizeiptr indexByteSize = indexCount * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// DSA Path: Modern Invalidation
		glNamedBufferSubData(buffer->uiVBO, vertexByteOffset, vertexByteSize, pVertices);
		glNamedBufferSubData(buffer->uiEBO, indexByteOffset, indexByteSize, pIndices);
	}
	else
	{
		// Legacy Path: Manual Orphaning
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferSubData(GL_ARRAY_BUFFER, vertexByteOffset, vertexByteSize, pVertices);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexByteOffset, indexByteSize, pIndices);
	}

	buffer->vertexOffset += vertexCount;
	buffer->indexOffset += indexCount;

	// Update total counts
	buffer->vertexCount = (GLuint)buffer->vertexOffset;
	buffer->indexCount = (GLuint)buffer->indexOffset;

	syslog("buffer VertexOffset: %zu IndexOffset: %zu, VertexCount: %u, IndexCount: %u", buffer->vertexOffset, buffer->indexOffset, buffer->vertexCount, buffer->indexCount);
}

void Mesh3DGLBuffer_UploadData(GLBuffer buffer, Mesh3D mesh)
{
	if (!buffer || !mesh || !mesh->pVertices || !mesh->pVertices->pData ||
		!mesh->pIndices || !mesh->pIndices->pData || mesh->vertexCount == 0)
	{
		syslog("UpdateBufferMesh3DData: Invalid mesh! vCount=%zu pDataV=%p pDataI=%p",
			mesh->vertexCount, mesh->pVertices ? mesh->pVertices->pData : NULL,
			mesh->pIndices ? mesh->pIndices->pData : NULL);
		return;
	}

	mesh->indexOffset = Mesh3DGLBuffer_GetBufferIndexOffset(buffer);
	mesh->vertexOffset = Mesh3DGLBuffer_GetBufferVertexOffset(buffer);

	const SVertex3D* pVertices = (SVertex3D*)mesh->pVertices->pData;
	const GLuint* pIndices = (GLuint*)mesh->pIndices->pData;
	GLsizeiptr vertexCount = mesh->vertexCount;
	GLsizeiptr indexCount = mesh->indexCount;

	// Check if we need to reallocate
	GLsizeiptr requiredVertexCount = buffer->vertexOffset + vertexCount;
	GLsizeiptr requiredIndexCount = buffer->indexOffset + indexCount;

	if (requiredVertexCount > buffer->vboCapacity || requiredIndexCount > buffer->eboCapacity)
	{
		// Grow by 50% or the required amount, whichever is larger
		GLsizeiptr newVboCap = buffer->vboCapacity + (buffer->vboCapacity / 2);
		if (newVboCap < requiredVertexCount)
		{
			newVboCap = requiredVertexCount;
		}

		GLsizeiptr newEboCap = buffer->eboCapacity + (buffer->eboCapacity / 2);
		if (newEboCap < requiredIndexCount)
		{
			newEboCap = requiredIndexCount;
		}

		if (newVboCap > 1024 * 1024 || newEboCap > 1024 * 1024)
		{
			syserr("VBO(%zu) or EBO(%zu) out of bounds!", newVboCap, newEboCap);
			return;
		}

		Mesh3DGLBuffer_Reallocate(buffer, newVboCap, newEboCap, true); // Copy old data
		syslog("Attemp to Reallocate Buffer...");
	}

	// Calculate byte offsets for glBufferSubData
	GLsizeiptr vertexByteOffset = buffer->vertexOffset * sizeof(SVertex3D);
	GLsizeiptr indexByteOffset = buffer->indexOffset * sizeof(GLuint);

	GLsizeiptr vertexByteSize = vertexCount * sizeof(SVertex3D);
	GLsizeiptr indexByteSize = indexCount * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// DSA Path: Modern Invalidation
		glNamedBufferSubData(buffer->uiVBO, vertexByteOffset, vertexByteSize, pVertices);
		glNamedBufferSubData(buffer->uiEBO, indexByteOffset, indexByteSize, pIndices);
	}
	else
	{
		// Legacy Path: Manual Orphaning
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferSubData(GL_ARRAY_BUFFER, vertexByteOffset, vertexByteSize, pVertices);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexByteOffset, indexByteSize, pIndices);
	}

	buffer->vertexOffset += vertexCount;
	buffer->indexOffset += indexCount;

	// Update total counts
	buffer->vertexCount = (GLuint)buffer->vertexOffset;
	buffer->indexCount = (GLuint)buffer->indexOffset;
}

bool Mesh3DGLBuffer_Initialize(GLBuffer* ppBuffer)
{
	*ppBuffer = (GLBuffer)tracked_malloc(sizeof(SGLBuffer));
	GLBuffer pGLBuffer = *ppBuffer;

	if (!pGLBuffer)
	{
		syslog("Failed to Allocate Space for GLBuffer");
		return (false);
	}

	// make sure it's all elements set to zero bytes
	memset(pGLBuffer, 0, sizeof(SGLBuffer));

	// 1. Setup the Metadata inside the struct
	pGLBuffer->vboCapacity = INITIAL_VERTEX_CAPACITY;
	pGLBuffer->eboCapacity = INITIAL_INDEX_CAPACITY;
	pGLBuffer->bIsInitialized = false;

	// 2. Create the GPU "Names" (IDs)
	if (GLBuffer_Create(pGLBuffer) == false)
	{
		GLBuffer_Delete(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return (false);
	}

	if (IsGLVersionHigher(4, 5))
	{
		pGLBuffer->bufferStorageType = GL_DYNAMIC_STORAGE_BIT;
	}
	else
	{
		pGLBuffer->bufferStorageType = GL_STATIC_DRAW;
	}

	// 3. Allocate the actual VRAM
	// This uses the metadata we just set to call glNamedBufferStorage or glBufferData
	if (Mesh3DGLBuffer_AllocateStorage(pGLBuffer) == false)
	{
		GLBuffer_Delete(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return (false);
	}

	// X. Define the Vertex Layout (What does a vertex look like?)
	// This tells the VAO how to interpret the data (Position, Color, etc.)
	Mesh3DGLBuffer_AllocateVertexBuffer(pGLBuffer); // Already Called in the next step 

	// 4. Physical Link (Which buffers belong to this VAO?)
	Mesh3DGLBuffer_LinkBuffers(pGLBuffer);
	
	// 5.1 Log ..
	syslog("Successfully Created and Linked Buffer (%d, %d, %d)", pGLBuffer->uiVAO, pGLBuffer->uiVBO, pGLBuffer->uiEBO);
	return (true);
}

bool Mesh3DGLBuffer_Reallocate(GLBuffer pBuffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData)
{
#ifdef _DEBUG
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		syserr("Mesh3DGLBuffer_Reallocate failed with GL error: 0x%X", error);
		// return (false);
	}
#endif

	if (!pBuffer)
	{
		return (false);
	}

	if (newVboCapacity <= pBuffer->vboCapacity && newEboCapacity <= pBuffer->eboCapacity)
	{
		return (true); // ????? same size!
	}

	// Store old GPU Buffers Data
	GLuint oldVBO = pBuffer->uiVBO;
	GLuint oldEBO = pBuffer->uiEBO;

	// New GPU Buffers
	GLuint newBuffers[2] = { 0, 0 }; // 0 -> VBO , 1 -> EBO
	if (!CreateBuffers(newBuffers, 2))
	{
		return (false); // Failed to Create GPU Buffers
	}

	// Update Buffer Bytes Size
	pBuffer->vboSize = newVboCapacity * sizeof(SMesh3D);
	pBuffer->eboSize = newEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Immutable Storage !
		glNamedBufferStorage(newBuffers[0], pBuffer->vboSize, NULL, pBuffer->bufferStorageType);
		glNamedBufferStorage(newBuffers[1], pBuffer->eboSize, NULL, pBuffer->bufferStorageType);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, newBuffers[0]);
		glBufferData(GL_ARRAY_BUFFER, pBuffer->vboSize, NULL, pBuffer->bufferStorageType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newBuffers[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pBuffer->eboSize, NULL, pBuffer->bufferStorageType);
	}

	// 1. Keep track of how much valid data we actually have right now
	GLsizeiptr currentVertexCount = pBuffer->vertexOffset;
	GLsizeiptr currentIndexCount = pBuffer->indexOffset;

	// Copy Data
	if (copyOldData)
	{
		GLsizeiptr bytesToCopyVBO = currentVertexCount * sizeof(SMesh3D);
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
	pBuffer->uiVBO = newBuffers[0];
	pBuffer->uiEBO = newBuffers[1];

	pBuffer->vboCapacity = newVboCapacity;
	pBuffer->eboCapacity = newEboCapacity;

	if (!copyOldData)
	{
		// Clear all counters if not copying data
		pBuffer->vertexCount = 0;
		pBuffer->indexCount = 0;
		pBuffer->vertexOffset = 0;
		pBuffer->indexOffset = 0;
	}

	Mesh3DGLBuffer_LinkBuffers(pBuffer);
	return (true);
}

GLuint Mesh3DGLBuffer_GetVertexArray(GLBuffer buffer)
{
	return (buffer->uiVAO);
}

GLsizeiptr Mesh3DGLBuffer_GetBufferVertexOffset(GLBuffer buffer)
{
	return (buffer->vertexOffset);
}

GLsizeiptr Mesh3DGLBuffer_GetBufferIndexOffset(GLBuffer buffer)
{
	return (buffer->indexOffset);
}

void Mesh3DGLBuffer_RenderBuffer(GLBuffer buffer, GLenum renderMode)
{
	if (!buffer)
	{
		syserr("Buffer is NULL!!");
		return;
	}

	// Safety: Don't draw if not initialized or if we have no data, no need to check vertexCount since we're using glDrawElements but we will keep it ..
	if (buffer->bIsInitialized == false || buffer->vertexCount == 0 || buffer->indexCount == 0)
	{
		syserr("Cannot render buffer! (%u-%u)", buffer->vertexCount, buffer->indexCount);
		return;
	}

	// Bind the 'Boss' (The VAO)
	// In OpenGL, the VAO already knows about the VBO and EBO because of our LinkBuffers call
	StateManager_BindBufferVAO(GetStateManager(), buffer);

	// Perform the draw
	// indexCount was set to indexCount in our Update function
	glDrawElements(renderMode, buffer->indexCount, GL_UNSIGNED_INT, NULL);
}
