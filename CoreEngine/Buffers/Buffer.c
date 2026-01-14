#include "Buffer.h"
#include "../Core/CoreUtils.h"
#include <memory.h>
#include <stddef.h> // Required for offsetof
#include "../Renderer/StateManager.h"
#include "GLBuffer.h"

typedef struct SGLBuffer
{
	GLuint uiVAO;			// The Vertex Array Object (The "Boss" handle)
	GLuint uiVBO;			// The Vertex Buffer Object (The actual data)
	GLuint uiEBO;			// The Element Buffer (Optional, for optimized drawing)


	GLuint vertexCount;		// glDrawArrays needs it
	GLuint indexCount;		// glDrawElements needs it

	GLsizeiptr vboCapacity;
	GLsizeiptr eboCapacity;

	GLsizeiptr vertexOffset;
	GLsizeiptr indexOffset;

	bool bIsInitialized;
} SGLBuffer;

void DeleteGLBuffer(GLBuffer buffer)
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

bool CreateGLBuffer(GLBuffer buffer)
{
	if (!buffer)
	{
		syslog("Attemp to Create NULL buffer");
		return (false);
	}

	// Prevent Leaks: Clean up if this group was already used
	DeleteGLBuffer(buffer);

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
		DeleteGLBuffer(buffer); // Clean up if one failed
		syserr("Failed to generate Buffer!");
		return (false);
	}

	return (true);
}

void ResetBuffer(GLBuffer buffer)
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
void ClearBuffer(GLBuffer buffer)
{
	if (!buffer)
	{
		syserr("ClearBuffer: NULL buffer");
		return;
	}

	// Reset cursors
	ResetBuffer(buffer);

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

void DestroyBuffer(GLBuffer* ppBuffer)
{
	if (!ppBuffer || !*ppBuffer)
	{
		return;
	}

	GLBuffer pBuffer = *ppBuffer;

	DeleteGLBuffer(pBuffer);
	tracked_free(pBuffer);

	*ppBuffer = NULL;
}

bool AllocateVertexBuffersStorage(GLBuffer buffer)
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

void LinkVertexBuffers(GLBuffer buffer)
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
		SetupVertexBufferAttributesVertex(buffer);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->bIsInitialized = true;
}

void SetupVertexBufferAttributesVertex(GLBuffer buffer)
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

void UpdateBufferVertexData(GLBuffer buffer, const SVertex* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount)
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

		ReallocateVertexBuffer(buffer, newVboCap, newEboCap, false);
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

bool InitializeVertexGLBuffer(GLBuffer* ppBuffer)
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
	if (CreateGLBuffer(pGLBuffer) == false)
	{
		DeleteGLBuffer(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return false;
	}

	// 3. Allocate the actual VRAM
	// This uses the metadata we just set to call glNamedBufferStorage or glBufferData
	if (AllocateVertexBuffersStorage(pGLBuffer) == false)
	{
		DeleteGLBuffer(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return false;
	}

	// 4. Define the Vertex Layout (What does a vertex look like?)
	// This tells the VAO how to interpret the data (Position, Color, etc.)
	SetupVertexBufferAttributesVertex(pGLBuffer);

	// 5. Physical Link (Which buffers belong to this VAO?)
	// This tells the VAO which specific VBO/EBO IDs to pull from
	LinkVertexBuffers(pGLBuffer);

	// 5.1 Log ..
	syslog("Successfully Created and Linked Buffer (%d, %d, %d)", pGLBuffer->uiVAO, pGLBuffer->uiVBO, pGLBuffer->uiEBO);

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
void ReallocateVertexBuffer(GLBuffer buffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData)
{
	// 1. Store old state
	GLuint oldVBO = buffer->uiVBO;
	GLuint oldEBO = buffer->uiEBO;
	GLsizeiptr oldVboByteSize = buffer->vboCapacity * sizeof(SVertex);
	GLsizeiptr oldEboByteSize = buffer->eboCapacity * sizeof(GLuint);

	// 2. Generate New Handles
	GLuint newVBO, newEBO;
	CreateBuffer(&newVBO);
	CreateBuffer(&newEBO);

	// 3. Allocate Storage, with Empty buffers
	GLsizeiptr newVboByteSize = newVboCapacity * sizeof(SVertex);
	GLsizeiptr newEboByteSize = newEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Modern: Immutable Storage
		glNamedBufferStorage(newVBO, newVboByteSize, NULL, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(newEBO, newEboByteSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		// Legacy: Mutable Storage (using GL_STATIC_DRAW or GL_DYNAMIC_DRAW)
		glBindBuffer(GL_ARRAY_BUFFER, newVBO);
		glBufferData(GL_ARRAY_BUFFER, newVboByteSize, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, newEboByteSize, NULL, GL_STATIC_DRAW);
	}

	if (copyOldData)
	{
		// 4. The Copy (GPU to GPU) - VBO
		if (oldVBO != 0 && oldVboByteSize > 0)
		{
			GLsizeiptr bytesToCopyV = (newVboByteSize < oldVboByteSize) ? newVboByteSize : oldVboByteSize;
			if (IsGLVersionHigher(4, 5))
			{
				// 0 represent offsets of read / write - from / to buffers
				glCopyNamedBufferSubData(oldVBO, newVBO, 0, 0, bytesToCopyV);
			}
			else
			{
				// Legacy Copy: Use temporary read/write targets to avoid messing with VAO bindings
				glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, bytesToCopyV);
			}
		}

		// 4.1. The Copy (GPU to GPU) - EBO
		if (oldEBO != 0 && oldEboByteSize > 0)
		{
			GLsizeiptr bytesToCopyE = (newEboByteSize < oldEboByteSize) ? newEboByteSize : oldEboByteSize;

			if (IsGLVersionHigher(4, 5))
			{
				// 0 represent offsets of read / write - from / to buffers
				glCopyNamedBufferSubData(oldEBO, newEBO, 0, 0, bytesToCopyE);
			}
			else
			{
				// Legacy Copy: Use temporary read/write targets to avoid messing with VAO bindings
				glBindBuffer(GL_COPY_READ_BUFFER, oldEBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newEBO);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, bytesToCopyE);
			}
		}
	}

	// 5. Cleanup
	if (oldVBO != 0)
	{
		glDeleteBuffers(1, &oldVBO);
	}
	if (oldEBO != 0)
	{
		glDeleteBuffers(1, &oldEBO);
	}

	// 6. Update Buffer
	buffer->uiVBO = newVBO;
	buffer->uiEBO = newEBO;
	buffer->vboCapacity = newVboCapacity;
	buffer->eboCapacity = newEboCapacity;

	syslog("Buffer Expanded to V:%zu E:%zu", newVboCapacity, newEboCapacity);

	LinkVertexBuffers(buffer);
}

bool AllocateMesh3DBuffersStorage(GLBuffer buffer)
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

void LinkMesh3DBuffers(GLBuffer buffer)
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
		SetupVertexBufferAttributesMesh3D(buffer);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->bIsInitialized = true;
}

void SetupVertexBufferAttributesMesh3D(GLBuffer buffer)
{
	if (!buffer)
	{
		return;
	}

	GLint iPosition = 0;
	GLint iTexCoords = 1;
	GLint iColors = 2;

	GLuint numFloats = 0;

	if (IsGLVersionHigher(4, 5))
	{
		// Enable Attributes in Vertex Array object (VAO)
		glEnableVertexArrayAttrib(buffer->uiVAO, iPosition);
		glEnableVertexArrayAttrib(buffer->uiVAO, iTexCoords);
		glEnableVertexArrayAttrib(buffer->uiVAO, iColors);

		// We will Apply 16 Bytes on all components as (4 floats) since we are using SIMD .. 
		glVertexArrayAttribFormat(buffer->uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		glVertexArrayAttribFormat(buffer->uiVAO, iTexCoords, 2, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 2 Floats (U, V)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		glVertexArrayAttribFormat(buffer->uiVAO, iColors, 4, GL_FLOAT, GL_FALSE, numFloats * sizeof(GLfloat)); // 4 flaots (r,g,b,a)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)

		// Bind Attributes to VAO
		glVertexArrayAttribBinding(buffer->uiVAO, iPosition, 0);
		glVertexArrayAttribBinding(buffer->uiVAO, iTexCoords, 0);
		glVertexArrayAttribBinding(buffer->uiVAO, iColors, 0);
	}
	else
	{
		glEnableVertexAttribArray(iPosition);

		// We will Apply 16 Bytes on all components as (4 floats) since we are using SIMD .. 
		glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 3 floats (x,y,w)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		
		glEnableVertexAttribArray(iTexCoords);
		glVertexAttribPointer(iTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 2 Floats (U, V)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
		
		glEnableVertexAttribArray(iColors);
		glVertexAttribPointer(iColors, 4, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(numFloats * sizeof(GLfloat))); // 4 flaots (r,g,b,a)
		numFloats += 4; // we add 4 floats for alignment (x,y,z,w)
	}
}

void UpdateBufferMesh3DVertexData(GLBuffer buffer, const SVertex3D* pVertices, GLsizeiptr vertexCount, const GLuint* pIndices, GLsizeiptr indexCount)
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

		ReallocateMesh3DBuffer(buffer, newVboCap, newEboCap, true);
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

void UpdateBufferMesh3DData(GLBuffer buffer, Mesh3D mesh)
{
	if (!buffer || !mesh || !mesh->pVertices || !mesh->pVertices->pData ||
		!mesh->pIndices || !mesh->pIndices->pData || mesh->vertexCount == 0)
	{
		syslog("UpdateBufferMesh3DData: Invalid mesh! vCount=%zu pDataV=%p pDataI=%p",
			mesh->vertexCount, mesh->pVertices ? mesh->pVertices->pData : NULL,
			mesh->pIndices ? mesh->pIndices->pData : NULL);
		return;
	}

	mesh->indexOffset = GetBufferIndexOffset(buffer);
	mesh->vertexOffset = GetBufferVertexOffset(buffer);

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

		ReallocateMesh3DBuffer(buffer, newVboCap, newEboCap, true); // Copy old data
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

bool InitializeMesh3DGLBuffer(GLBuffer* ppBuffer)
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
	if (CreateGLBuffer(pGLBuffer) == false)
	{
		DeleteGLBuffer(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return (false);
	}

	// 3. Allocate the actual VRAM
	// This uses the metadata we just set to call glNamedBufferStorage or glBufferData
	if (AllocateMesh3DBuffersStorage(pGLBuffer) == false)
	{
		DeleteGLBuffer(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return (false);
	}

	// X. Define the Vertex Layout (What does a vertex look like?)
	// This tells the VAO how to interpret the data (Position, Color, etc.)
	SetupVertexBufferAttributesMesh3D(pGLBuffer); // Already Called in the next step 

	// 4. Physical Link (Which buffers belong to this VAO?)
	LinkMesh3DBuffers(pGLBuffer);
	
	// 5.1 Log ..
	syslog("Successfully Created and Linked Buffer (%d, %d, %d)", pGLBuffer->uiVAO, pGLBuffer->uiVBO, pGLBuffer->uiEBO);
	return (true);
}

void ReallocateMesh3DBuffer(GLBuffer buffer, GLsizeiptr newVboCapacity, GLsizeiptr newEboCapacity, bool copyOldData)
{
	if (!buffer)
	{
		return;
	}

	// Validate: Only error if trying to shrink WITH data copy
	if (copyOldData)
	{
		if (newVboCapacity < buffer->vboCapacity || newEboCapacity < buffer->eboCapacity)
		{
			syserr("Cannot shrink buffer while copying data (would lose data)");
			return;
		}

		if (newVboCapacity == buffer->vboCapacity && newEboCapacity == buffer->eboCapacity)
		{
			syslog("Buffer already has requested capacity, skipping reallocation");
			return;  // Early exit - no work needed
		}
	}

	GLuint oldVBO = buffer->uiVBO;
	GLuint oldEBO = buffer->uiEBO;
	GLsizeiptr oldVBOCapacity = buffer->vboCapacity * sizeof(SVertex3D);
	GLsizeiptr oldEBOCapacity = buffer->eboCapacity * sizeof(GLuint);

	GLuint newVBO, newEBO;
	CreateBuffer(&newVBO);
	CreateBuffer(&newEBO);

	GLsizeiptr newVBOSize = newVboCapacity * sizeof(SVertex3D);
	GLsizeiptr newEBOSize = newEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferStorage(newVBO, newVBOSize, NULL, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(newEBO, newEBOSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, newVBO);
		glBufferData(GL_ARRAY_BUFFER, newVBOSize, NULL, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, newEBOSize, NULL, GL_DYNAMIC_DRAW);
	}

	if (copyOldData)
	{
		if (IsGLVersionHigher(4, 5))
		{
			if (oldVBO != 0)
			{
				glCopyNamedBufferSubData(oldVBO, newVBO, 0, 0, oldVBOCapacity);
			}
			if (oldEBO != 0)
			{
				glCopyNamedBufferSubData(oldEBO, newEBO, 0, 0, oldEBOCapacity);
			}
		}
		else
		{
			// Copy Old Vertex Buffer
			if (oldVBO != 0)
			{
				glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, oldVBOCapacity);
			}

			// Copy Old Index Buffer
			if (oldEBO != 0)
			{
				glBindBuffer(GL_COPY_READ_BUFFER, oldEBO);
				glBindBuffer(GL_COPY_WRITE_BUFFER, newEBO);
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, oldEBOCapacity);
			}
		}
	}

	// Delete Old Buffers
	if (oldVBO != 0)
	{
		DeleteBuffer(&oldVBO);
	}
	if (oldEBO != 0)
	{
		DeleteBuffer(&oldEBO);
	}

	// Assign New Buffers and their size
	buffer->uiVBO = newVBO;
	buffer->uiEBO = newEBO;
	buffer->vboCapacity = newVboCapacity;
	buffer->eboCapacity = newEboCapacity;

	// Physical Link (Which buffers belong to this VAO?)
	LinkMesh3DBuffers(buffer);

	syslog("Reallocated buffer: VBO %zu -> %zu vertices, EBO %zu -> %zu indices", oldVBOCapacity / sizeof(SVertex3D), newVboCapacity, oldEBOCapacity / sizeof(GLuint), newEboCapacity);
}

GLuint GetVertexArray(GLBuffer buffer)
{
	return (buffer->uiVAO);
}

GLsizeiptr GetBufferVertexOffset(GLBuffer buffer)
{
	return (buffer->vertexOffset);
}

GLsizeiptr GetBufferIndexOffset(GLBuffer buffer)
{
	return (buffer->indexOffset);
}

void RenderBuffer(GLBuffer buffer, GLenum renderMode)
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
	BindBufferVAO(GetStateManager(), buffer);

	// Perform the draw
	// indexCount was set to indexCount in our Update function
	glDrawElements(renderMode, buffer->indexCount, GL_UNSIGNED_INT, NULL);
}
