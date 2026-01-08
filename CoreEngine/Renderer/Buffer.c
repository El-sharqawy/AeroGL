#include "Buffer.h"
#include "../Core/CoreUtils.h"
#include <memory.h>
#include <stddef.h> // Required for offsetof

typedef struct SGLBuffer
{
	GLuint uiVAO;			// The Vertex Array Object (The "Boss" handle)
	GLuint uiVBO;			// The Vertex Buffer Object (The actual data)
	GLuint uiEBO;			// The Element Buffer (Optional, for optimized drawing)


	GLuint vertexCount;		// glDrawArrays needs it
	GLuint indexCount;		// glDrawElements needs it

	GLint iVboCapacity;
	GLint iEboCapacity;
	bool IsInitialized;
} SGLBuffer;

void DeleteBuffer(GLuint* puiBufferID)
{
	if (puiBufferID && *puiBufferID != 0)
	{
		glDeleteBuffers(1, puiBufferID);
		*puiBufferID = 0;
	}
}

void DeleteVertexArray(GLuint* puiVertexArrayID)
{
	if (puiVertexArrayID && *puiVertexArrayID != 0)
	{
		glDeleteVertexArrays(1, puiVertexArrayID);
		*puiVertexArrayID = 0;
	}
}

void DeleteBuffers(GLuint* puiBufferIDs, GLsizei uiCount)
{
	if (!puiBufferIDs || uiCount <= 0)
	{
		return;
	}

	// OpenGL spec: "Unused names in buffers are silently ignored, as is the value 0."
	glDeleteBuffers(uiCount, puiBufferIDs);

	// Always reset to 0 so the rest of our engine knows they are gone
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		puiBufferIDs[i] = 0;
	}
}

void DeleteVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount)
{
	if (!puiVertexArrayIDs || uiCount <= 0)
	{
		return;
	}

	// OpenGL spec: "Unused names in vertex arrays are silently ignored, as is the value 0."
	glDeleteVertexArrays(uiCount, puiVertexArrayIDs);

	// Always reset to 0 so the rest of our engine knows they are gone
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		puiVertexArrayIDs[i] = 0;
	}
}

bool CreateBuffer(GLuint* puiBufferID)
{
	if (!puiBufferID)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	DeleteBuffer(puiBufferID);

	// Create new ID
	if (IsGLVersionHigher(4, 5))
	{
		glCreateBuffers(1, puiBufferID);
	}
	else
	{
		glGenBuffers(1, puiBufferID);
	}
	return (*puiBufferID != 0);
}

bool CreateVertexArray(GLuint* puiVertexArrayID)
{
	if (!puiVertexArrayID)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	DeleteVertexArray(puiVertexArrayID);

	// Create new ID
	if (IsGLVersionHigher(4, 5))
	{
		glCreateVertexArrays(1, puiVertexArrayID);
	}
	else
	{
		glGenVertexArrays(1, puiVertexArrayID);
	}
	return (*puiVertexArrayID != 0);
}

bool CreateBuffers(GLuint* puiBuffersIDs, GLsizei uiCount)
{
	if (!puiBuffersIDs || uiCount <= 0)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	DeleteBuffers(puiBuffersIDs, uiCount);

	// Create new ID
	if (IsGLVersionHigher(4, 5))
	{
		glCreateBuffers(uiCount, puiBuffersIDs);
	}
	else
	{
		glGenBuffers(uiCount, puiBuffersIDs);
	}

	// Make sure they are all have been Initialized
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		if (puiBuffersIDs[i] == 0)
		{
			DeleteBuffers(puiBuffersIDs, uiCount);
			syserr("Failed to Create Buffer at Index: %d", i);
			return (false);
		}
	}

	return (true);
}

bool CreateVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount)
{
	if (!puiVertexArrayIDs || uiCount <= 0)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	DeleteVertexArrays(puiVertexArrayIDs, uiCount);

	// Create new ID
	if (IsGLVersionHigher(4, 5))
	{
		glCreateVertexArrays(uiCount, puiVertexArrayIDs);
	}
	else
	{
		glGenVertexArrays(uiCount, puiVertexArrayIDs);
	}

	// Make sure they are all have been Initialized
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		if (puiVertexArrayIDs[i] == 0)
		{
			DeleteVertexArrays(puiVertexArrayIDs, uiCount);
			syserr("Failed to Create Vertex Array at Index: %d", i);
			return (false);
		}
	}

	return (true);
}

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

	buffer->IsInitialized = false; // Reset the engine state flag
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

bool AllocateBuffersStorage(GLBuffer buffer)
{
	if (buffer->uiVBO == 0 || buffer->uiEBO == 0)
	{
		syserr("Called with invalid VBO (%u) or EBO (%u)!", buffer->uiVBO, buffer->uiEBO);
		return (false);
	}

	GLsizeiptr vboBytes = buffer->iVboCapacity * sizeof(SVertex);
	GLsizeiptr eboBytes = buffer->iEboCapacity * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// IMPORTANT: glNamedBufferStorage is IMMUTABLE. 
		// If the buffer was already allocated, we MUST delete and recreate the ID.
		// Allocation (Immutable) - Ensure ID is fresh or deleted/recreated before calling
		glNamedBufferStorage(buffer->uiVBO, vboBytes, nullptr, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(buffer->uiEBO, eboBytes, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		// Allocation (Mutable)
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);
		glBufferData(GL_ARRAY_BUFFER, vboBytes, nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboBytes, nullptr, GL_DYNAMIC_DRAW);
	}

	return (true);
}

void LinkBuffers(GLBuffer buffer)
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

		SetupVertexBufferAttributesVertex(buffer);
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

	buffer->IsInitialized = true;
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

	if (vertexCount > buffer->iVboCapacity || indexCount > buffer->iEboCapacity)
	{
		// Grow by 50% or the required amount, whichever is larger
		GLsizeiptr newVboCap = buffer->iVboCapacity + (buffer->iVboCapacity / 2);
		if (newVboCap < vertexCount)
		{
			newVboCap = vertexCount;
		}

		GLsizeiptr newEboCap = buffer->iEboCapacity + (buffer->iEboCapacity / 2);
		if (newEboCap < indexCount)
		{
			newEboCap = indexCount;
		}

		ReallocateBuffer(buffer, newVboCap, newEboCap, false);
		syslog("Attemp to Reallocate Buffer...");
	}

	// Calculate byte sizes automatically based on the template type T
	GLsizeiptr vboByteSize = vertexCount * sizeof(SVertex);
	GLsizeiptr eboByteSize = indexCount * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// DSA Path: Modern Invalidation
		glInvalidateBufferData(buffer->uiVBO);
		glInvalidateBufferData(buffer->uiEBO);

		glNamedBufferSubData(buffer->uiVBO, 0, vboByteSize, pVertices);
		glNamedBufferSubData(buffer->uiEBO, 0, eboByteSize, pIndices);
	}
	else
	{
		// Legacy Path: Manual Orphaning
		glBindBuffer(GL_ARRAY_BUFFER, buffer->uiVBO);

		// Legacy "Orphaning": Re-specifying the buffer with NULL tells the driver
		// to give us a fresh block of memory. This prevents pipeline stalls.
		glBufferData(GL_ARRAY_BUFFER, buffer->iVboCapacity * sizeof(SVertex), nullptr, GL_DYNAMIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, vboByteSize, pVertices);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->uiEBO);

		// Legacy "Orphaning": Re-specifying the buffer with NULL tells the driver
		// to give us a fresh block of memory. This prevents pipeline stalls.
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->iEboCapacity * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboByteSize, pIndices);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	buffer->vertexCount = (GLuint)vertexCount;
	buffer->indexCount = (GLuint)indexCount;
}

bool InitializeGLBuffer(GLBuffer* ppBuffer)
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
	pGLBuffer->iVboCapacity = INITIAL_VERTEX_CAPACITY;
	pGLBuffer->iEboCapacity = INITIAL_INDEX_CAPACITY;
	pGLBuffer->IsInitialized = false;

	// 2. Create the GPU "Names" (IDs)
	if (CreateGLBuffer(pGLBuffer) == false)
	{
		DeleteGLBuffer(pGLBuffer); // Clean up GPU side
		tracked_free(pGLBuffer);   // Clean up CPU side
		return false;
	}

	// 3. Allocate the actual VRAM
	// This uses the metadata we just set to call glNamedBufferStorage or glBufferData
	if (AllocateBuffersStorage(pGLBuffer) == false)
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
	LinkBuffers(pGLBuffer);

	// 5.1 Log ..
	syslog("Successfully Created and Linked Buffer (%d, %d, %d)", pGLBuffer->uiVAO, pGLBuffer->uiVBO, pGLBuffer->uiEBO);

	// 6. Return the Created Buffer
	return (true);
}

void RenderBuffer(GLBuffer buffer, GLenum renderMode)
{
	// Safety: Don't draw if not initialized or if we have no data, no need to check vertexCount since we're using glDrawElements but we will keep it ..
	if (!buffer || buffer->IsInitialized == false || buffer->vertexCount == 0 || buffer->indexCount == 0)
	{
		return;
	}

	// Bind the 'Boss' (The VAO)
	// In OpenGL, the VAO already knows about the VBO and EBO because of our LinkBuffers call
	BindBufferVAO(GetStateManager(), buffer);

	// Perform the draw
	// indexCount was set to indexCount in our Update function
	glDrawElements(renderMode, buffer->indexCount, GL_UNSIGNED_INT, NULL);
}

void ReallocateBuffer(GLBuffer buffer, size_t vNewSize, size_t iNewSize, bool copyOldData)
{
	// 1. Store old state
	GLuint oldVBO = buffer->uiVBO;
	GLuint oldEBO = buffer->uiEBO;
	GLsizeiptr oldVboByteSize = buffer->iVboCapacity * sizeof(SVertex);
	GLsizeiptr oldEboByteSize = buffer->iEboCapacity * sizeof(GLuint);

	// 2. Generate New Handles
	GLuint newVBO, newEBO;
	CreateBuffer(&newVBO);
	CreateBuffer(&newEBO);

	// 3. Allocate Storage, with Empty buffers
	GLsizeiptr newVboByteSize = vNewSize * sizeof(SVertex);
	GLsizeiptr newEboByteSize = iNewSize * sizeof(GLuint);

	if (IsGLVersionHigher(4, 5))
	{
		// Modern: Immutable Storage
		glNamedBufferStorage(newVBO, newVboByteSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(newEBO, newEboByteSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}
	else
	{
		// Legacy: Mutable Storage (using GL_STATIC_DRAW or GL_DYNAMIC_DRAW)
		glBindBuffer(GL_ARRAY_BUFFER, newVBO);
		glBufferData(GL_ARRAY_BUFFER, newVboByteSize, nullptr, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, newEboByteSize, nullptr, GL_STATIC_DRAW);
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
	buffer->iVboCapacity = (GLuint)vNewSize;
	buffer->iEboCapacity = (GLuint)iNewSize;

	syslog("Buffer Expanded to V:%zu E:%zu", vNewSize, iNewSize);

	LinkBuffers(buffer);
}

GLuint GetVertexArray(GLBuffer buffer)
{
	return (buffer->uiVAO);
}
