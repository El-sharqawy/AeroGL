#include "CoreUtils.h"
#include "../Stdafx.h"

GLint glMajorVersion = 0;
GLint glMinorVersion = 0;

// #define ENABLE_MEMORY_LOGS

void* cjson_tracked_malloc(size_t size)
{
	// We manually pass "cJSON" as the file so you know where the leak originated
	return tracked_malloc_internal(size, "cJSON_Internal", 0, "cJSON", MEM_TAG_RESOURCES);
}

void cjson_tracked_free(void* ptr)
{
	tracked_free_internal(ptr, "cJSON_Internal", 0);
}

const char* get_filename_ext(const char* filename)
{
	const char* dot = strrchr(filename, '.');
	if (dot)
	{
		return (dot + 1);
	}
	return NULL;
}

const char* get_filename(const char* filepath)
{
	// Find last backslash (Windows) or forward slash (Unix)
	// 1. Find the last backslash
	const char* last_backslash = strrchr(filepath, '\\');
	// 2. Find the last forward slash
	const char* last_forward = strrchr(filepath, '/');

	// Determine which one is later in the string
	// 3. We want the one that appears LATEST in the string
	const char* last_slash = (last_backslash > last_forward) ? last_backslash : last_forward;

	// 4. If we found any slash, return the char after it. Otherwise, return the original.
	return (last_slash) ? (last_slash + 1) : filepath;
}

bool IsGLVersionHigher(GLint MajorVer, GLint MinorVer)
{
	// Only query once for performance
	if (glMajorVersion == 0)
	{
		glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
	}

	// Check major version first
	if (glMajorVersion > MajorVer)
	{
		return true;
	}

	// Same major version, check minor
	if (glMajorVersion == MajorVer && glMinorVersion >= MinorVer)
	{
		return true;
	}

	return false;
}

bool MakeDirectory(const char* fullPath)
{
	errno_t result = 0;

	if (!IsDirectoryExists(fullPath))
	{
		// Create the directory
		result = MKDIR(fullPath);
	}

	if (result != 0)
	{
		char buffer[100];
		// Use strerror to get a human-readable message from the error code
		if (strerror_s(buffer, sizeof(buffer), result) == 0)
		{
			syserr("Error Creating Map Folder: %s (Code: %d)", buffer, result);
		}
		else
		{
			syserr("Failed to retrieve error message (Code: %d)", result);
		}
		return (false);
	}

	return (true);
}

bool IsDirectoryExists(const char* path)
{
	if (!path)
	{
		return (false);
	}

	struct stat stats;
	// Get file/directory information
	if (stat(path, &stats) == 0 && S_ISDIR(stats.st_mode)) 
	{
		return (true); // Directory exists
	}
	return (false); // Directory does not exist
}

bool File_IsFileExists(const char* filePath)
{
	if (access(filePath, F_OK) != -1)
	{
		return (true); // Don't overwrite without asking!
	}

	return (false);
}

bool File_GetInfo(const char* szPath, size_t* pOutSize)
{
	struct stat buffer;
	if (stat(szPath, &buffer) == 0)
	{
		if (pOutSize)
		{
			*pOutSize = buffer.st_size;
		}
		return true;
	}
	return false;
}

const char* File_GetExtension(const char* szPath)
{
	if (szPath == NULL) return NULL;

	// Find the last occurrence of '.'
	const char* dot = strrchr(szPath, '.');

	// If no dot is found, or if the dot is the very last character, return an empty string
	if (!dot || dot == szPath + strlen(szPath) - 1)
	{
		return "";
	}

	// Return the string starting one character after the dot
	return dot + 1;
}

const char* File_GetFileName(const char* szPath)
{
	if (!szPath) return NULL;

	// Find the last occurrence of both types of slashes
	const char* lastSlash = strrchr(szPath, '/');
	const char* lastBackslash = strrchr(szPath, '\\');

	// Determine which one appears later in the string
	const char* lastSeparator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

	// If no separator is found, the path is already just the filename
	if (!lastSeparator)
	{
		return szPath;
	}

	// Return the string starting one character after the slash
	return lastSeparator + 1;
}

void File_GetFileNameNoExtension(const char* szPath, char* pOutBuffer, size_t bufferSize)
{
	const char* filename = File_GetFileName(szPath);
	if (!filename)
	{
		return;
	}

	// Copy filename to buffer
#if defined(_WIN32) || defined(_WIN64)
	strncpy_s(pOutBuffer, bufferSize, filename, bufferSize - 1);
#else
	strncpy(pOutBuffer, filename, bufferSize - 1);
#endif

	pOutBuffer[bufferSize - 1] = '\0';

	// Find the last dot in the result and null-terminate there
	char* dot = strrchr(pOutBuffer, '.');
	if (dot)
	{
		*dot = '\0';
	}
}