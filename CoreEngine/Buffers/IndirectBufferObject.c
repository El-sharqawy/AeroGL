#include "IndirectBufferObject.h"
#include "../Core/CoreUtils.h"
#include "../PipeLine/Utils.h"

bool IndirectBufferObject_Initialize(IndirectBufferObject* ppIndirectBuffer, GLsizeiptr initialCapacity)
{
	if (ppIndirectBuffer == NULL)
	{
		syserr("ppIndirectBuffer is NULL (invalid address)");
		return false;
	}

	*ppIndirectBuffer = tracked_calloc(1, sizeof(SIndirectBufferObject));
	IndirectBufferObject pIndirectBuf = *ppIndirectBuffer;

	if (!pIndirectBuf)
	{
		syserr("Failed to Allocate Indirect Buffer");
		return (false);
	}

	if (!Vector_InitCapacity(&pIndirectBuf->commands, sizeof(SIndirectDrawCommand), initialCapacity, false))
	{
		syserr("Failed to create command vector");
		IndirectBufferObject_Destroy(&pIndirectBuf);
		return (false);
	}

	GLsizeiptr bufferSize = sizeof(SIndirectDrawCommand) * initialCapacity;
	IndirectBufferObject_GenerateGL(pIndirectBuf, bufferSize);

	syslog("Created indirect buffer: %zu commands, %lld bytes", initialCapacity, (long long)bufferSize);
	return (true);
}

void IndirectBufferObject_GenerateGL(IndirectBufferObject pIndirectBuf, GLsizeiptr bufferSize)
{
	// Delete if Already Existing
	if (pIndirectBuf->bufferID != 0)
	{
		GL_DeleteBuffer(&pIndirectBuf->bufferID);
	}

	// Create Buffer
	GL_CreateBuffer(&pIndirectBuf->bufferID);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferStorage(pIndirectBuf->bufferID, bufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // DSA: Immutable storage (faster, more stable)
	}
	else
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectBuf->bufferID);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
	}
}

void IndirectBufferObject_AddCommand(IndirectBufferObject pIndirectBuf, GLuint count, GLuint instanceCount, GLuint firstIndex, GLuint baseVertex, GLuint baseInstance)
{
	if (!pIndirectBuf)
	{
		syserr("NULL buffer passed to IndirectBuffer_AddCommand");
		return;
	}

	// Build command
	SIndirectDrawCommand cmd;
	cmd.count = count;
	cmd.instanceCount = instanceCount;
	cmd.firstIndex = firstIndex;
	cmd.baseVertex = baseVertex;
	cmd.baseInstance = baseInstance;

	// Store old capacity
	size_t oldCapacity = pIndirectBuf->commands->capacity;

	// Push to vector (auto-grows if needed)
	Vector_PushBackValue(pIndirectBuf->commands, cmd);

	// Check if vector grew
	size_t newCapacity = pIndirectBuf->commands->capacity;
	if (newCapacity > oldCapacity)
	{
		GLsizeiptr bufferSize = newCapacity * sizeof(SIndirectDrawCommand);
		IndirectBufferObject_GenerateGL(pIndirectBuf, bufferSize);
	}

	pIndirectBuf->bDirty = true; // Mark dirty to Update
}

void IndirectBufferObject_Upload(IndirectBufferObject pIndirectBuf)
{
	if (!pIndirectBuf)
	{
		return;
	}

	if (pIndirectBuf->bDirty == false)
	{
		return;
	}

	size_t count = pIndirectBuf->commands->count;
	if (count == 0)
	{
		return; // no commands to upload
	}

	// Upload only used portion
	GLsizeiptr usedSize = count * sizeof(SIndirectDrawCommand);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(pIndirectBuf->bufferID, 0, usedSize, pIndirectBuf->commands->pData);
	}
	else
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectBuf->bufferID);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, usedSize, pIndirectBuf->commands->pData); // Offset 0
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	pIndirectBuf->bDirty = false;

	syslog("Uploaded %zu commands (%lld bytes) to GPU", count, (long long)usedSize);
}

/**
 * @brief Executes all commands in one glMultiDrawElementsIndirect call.
 *
 * Automatically uploads if dirty. Requires VAO to be bound.
 */
void IndirectBufferObject_Draw(IndirectBufferObject pIndirectBuf, GLenum primitiveType)
{
	if (!pIndirectBuf)
	{
		return;
	}

	size_t count = pIndirectBuf->commands->count;
	if (count == 0)
	{
		return;  // Nothing to draw
	}

	// Upload if needed
	if (pIndirectBuf->bDirty)
	{
		IndirectBufferObject_Upload(pIndirectBuf);
	}

	// Bind and draw
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectBuf->bufferID);

	glMultiDrawElementsIndirect(
		primitiveType,      // GL_TRIANGLES, GL_LINES, etc.
		GL_UNSIGNED_INT,    // Index type
		(void*)0,           // Start at beginning of buffer
		(GLsizei)count,     // Number of draw commands
		0                   // Stride (0 = tightly packed)
	);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

}

/**
 * @brief Modifies a command at specific index.
 */
void IndirectBufferObject_SetCommand(IndirectBufferObject pIndirectBuf, size_t index, const IndirectDrawCommand cmd)
{
	if (!pIndirectBuf || !cmd || index >= pIndirectBuf->commands->count)
	{
		return;
	}

	IndirectDrawCommand commands = (IndirectDrawCommand)pIndirectBuf->commands->pData;
	commands[index] = *cmd;

	pIndirectBuf->bDirty = true;
}

/**
 * @brief Clears all commands (resets for next frame).
 */
void IndirectBufferObject_Clear(IndirectBufferObject pIndirectBuf)
{
	if (!pIndirectBuf)
	{
		return;
	}

	Vector_Clear(pIndirectBuf->commands);  // Using Vector API
	pIndirectBuf->bDirty = false;

	// GPU buffer still allocated - just reset count
}

void IndirectBufferObject_Bind(IndirectBufferObject pIndirectBuf)
{
	if (!pIndirectBuf)
	{
		return;
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectBuf->bufferID);
}

void IndirectBufferObject_UnBind()
{
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void IndirectBufferObject_Destroy(IndirectBufferObject* ppIndirectBuffer)
{
	if (!ppIndirectBuffer || !*ppIndirectBuffer)
	{
		return;
	}

	IndirectBufferObject pBuf = *ppIndirectBuffer;

	GL_DeleteBuffer(&pBuf->bufferID); // Clear GPU Resources

	Vector_Destroy(&pBuf->commands); // Clear CPU Resources

	tracked_free(pBuf);

	*ppIndirectBuffer = NULL;
}
