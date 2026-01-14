#include "GLBuffer.h"

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
