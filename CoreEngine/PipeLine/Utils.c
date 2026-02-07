#include "Utils.h"
#include "../Stdafx.h"

void GL_DeleteBuffer(GLuint* puiBufferID)
{
	if (puiBufferID && *puiBufferID != 0)
	{
		glDeleteBuffers(1, puiBufferID);
		*puiBufferID = 0;
	}
}

void GL_DeleteVertexArray(GLuint* puiVertexArrayID)
{
	if (puiVertexArrayID && *puiVertexArrayID != 0)
	{
		glDeleteVertexArrays(1, puiVertexArrayID);
		*puiVertexArrayID = 0;
	}
}

void GL_DeleteBuffers(GLuint* puiBufferIDs, GLsizei uiCount)
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

void GL_DeleteVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount)
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

bool GL_CreateBuffer(GLuint* puiBufferID)
{
	if (!puiBufferID)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	GL_DeleteBuffer(puiBufferID);

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

bool GL_CreateVertexArray(GLuint* puiVertexArrayID)
{
	if (!puiVertexArrayID)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	GL_DeleteVertexArray(puiVertexArrayID);

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

bool GL_CreateBuffers(GLuint* puiBuffersIDs, GLsizei uiCount)
{
	if (!puiBuffersIDs || uiCount <= 0)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	GL_DeleteBuffers(puiBuffersIDs, uiCount);

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
			GL_DeleteBuffers(puiBuffersIDs, uiCount);
			syserr("Failed to Create Buffer at Index: %d", i);
			return (false);
		}
	}

	return (true);
}

bool GL_CreateVertexArrays(GLuint* puiVertexArrayIDs, GLsizei uiCount)
{
	if (!puiVertexArrayIDs || uiCount <= 0)
	{
		return false;
	}

	// Prevent leaks by cleaning up existing ID
	GL_DeleteVertexArrays(puiVertexArrayIDs, uiCount);

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
			GL_DeleteVertexArrays(puiVertexArrayIDs, uiCount);
			syserr("Failed to Create Vertex Array at Index: %d", i);
			return (false);
		}
	}

	return (true);
}

void GL_DeleteTexture(GLuint* puiTextureID)
{
	if (puiTextureID && *puiTextureID != 0)
	{
		glDeleteTextures(1, puiTextureID);
		*puiTextureID = 0;
	}
}

void GL_DeleteTextures(GLuint* puiTextureIDs, GLsizei uiCount)
{
	// 1. Input validation
	if (puiTextureIDs == NULL || uiCount <= 0)
	{
		return;
	}

	// 2. Delete textures
	// OpenGL spec: "Unused names in buffers are silently ignored, as is the value 0."
	glDeleteTextures(uiCount, puiTextureIDs);

	// 3. Reset IDs, Always reset to 0 so the rest of your engine knows they are gone
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		puiTextureIDs[i] = 0;
	}
}

bool GL_CreateTexture(GLuint* puiTextureID, GLenum eTextureTarget) // can just use GL_CreateTextures(puiTextureID, 1, eTextureTarget);
{
	// 1. Prevent leaks by cleaning up existing ID
	GL_DeleteTexture(puiTextureID);

	// 2. Create new ID
	if (IsGLVersionHigher(4, 5))
	{
		// DSA: No binding required, very efficient
		glCreateTextures(eTextureTarget, 1, puiTextureID);
	}
	else
	{
		// Legacy: MUST bind to "initialize" the object type in the driver
		glGenTextures(1, puiTextureID);

		// Use the passed target (eTargetTexture) instead of hardcoded GL_TEXTURE_2D
		glBindTexture(eTextureTarget, *puiTextureID);
	}

	// 3. Validation
	if (*puiTextureID == 0)
	{
		syserr("Core: Failed to generate GL Texture!");
		return (false);
	}

	return (true);
}

bool GL_CreateTextures(GLuint* puiTextureIDs, GLsizei uiCount, GLenum eTextureTarget)
{
	// 1. Input validation
	if (puiTextureIDs == NULL || uiCount <= 0)
	{
		return (false);
	}

	// 2. Prevent leaks by cleaning up existing IDs
	GL_DeleteTextures(puiTextureIDs, uiCount);

	// 3. Create new IDs
	if (IsGLVersionHigher(4, 5))
	{
		glCreateTextures(eTextureTarget, uiCount, puiTextureIDs);
	}
	else
	{
		glGenTextures(uiCount, puiTextureIDs);

		// Legacy Fix: Initialize the object type
		for (GLsizei i = 0; i < uiCount; ++i)
		{
			glBindTexture(eTextureTarget, puiTextureIDs[i]);
		}

		// Unbind the last one to stay clean
		glBindTexture(eTextureTarget, 0);
	}

	// 4. Validation
	for (GLsizei i = 0; i < uiCount; ++i)
	{
		if (puiTextureIDs[i] == 0)
		{
			syserr("Failed to generate GL Texture at index %d", i);
			return (false);
		}
	}

	return (true);
}

/**
 * @brief Maps the number of channels to OpenGL internal and pixel formats.
 *
 * @param channels Number of color channels (1-4).
 * @param internal Reference to store the internal format.
 * @param pixel Reference to store the pixel format.
 */
void GL_GetTextureFormats(GLint channels, GLenum* internal, GLenum* pixel, bool bIsFloat)
{
	switch (channels)
	{
	case 1:
		*internal = bIsFloat ? GL_R32F : GL_R8;
		*pixel = GL_RED;
		break;
	case 2:
		*internal = bIsFloat ? GL_RG32F : GL_RG8;
		*pixel = GL_RG;
		break;
	case 3:
		*internal = bIsFloat ? GL_RGB32F : GL_RGB8;
		*pixel = GL_RGB;  break;
	case 4:
		*internal = bIsFloat ? GL_RGBA32F : GL_RGBA8;
		*pixel = GL_RGBA; break;

	default:
		syslog("Format not implemented: %d channels", channels);
		*internal = bIsFloat ? GL_RGBA32F : GL_RGBA8;
		*pixel = GL_RGBA;
		break;
	}
}
