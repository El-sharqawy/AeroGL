#include "CoreUtils.h"
#include "Window.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h> // SSE

size_t allocation_count = 0;
size_t bytes_allocated = 0;
GLint glMajorVersion = 0;
GLint glMinorVersion = 0;

// #define ENABLE_MEMORY_LOGS

void* tracked_malloc_internal(size_t size, const char* file, int line)
{
	// 1. Align the header size to 16 bytes. 
	// Even if size_t is 8, we reserve 16 to keep the user pointer aligned.
	size_t total_size = size + sizeof(SMemoryBlockHeader); // actual size + header_size(for size_t)

	// 2. Use _aligned_malloc or ensure malloc gives us 16-byte alignment
	// On most 64-bit systems, malloc is 16-byte aligned by default.
	// size_t* raw_ptr = (size_t*)malloc(total_size); // allocate with total size
	void* raw_ptr = _mm_malloc(total_size, 16); // Ensure we get a 16-byte aligned block from the OS

	if (!raw_ptr)
	{
		return (NULL);
	}

	// Fill the header
	SMemoryBlockHeader* header = (SMemoryBlockHeader*)raw_ptr;
	header->size = size;
	header->magic = 0xDEADBEEF;
	header->file = file;
	header->line = line;

	// Return the pointer shifted by exactly 16 bytes
	// (raw_ptr + 2) if raw_ptr is size_t* (8 bytes each)
	void* user_ptr = (void*)((char*)raw_ptr + sizeof(SMemoryBlockHeader));

#ifdef _DEBUG
	if (((uintptr_t)user_ptr % 16) != 0)
	{
		syserr("User pointer not 16-byte aligned! Header size: %zu", sizeof(SMemoryBlockHeader));
	}
#endif

	allocation_count++;
	bytes_allocated += size;

#if defined(ENABLE_MEMORY_LOGS)
	syslog("allocated: %zu bytes (%s:%d)", bytes_allocated, get_filename(file), line);
#endif

	// Return pointer after the header
	return user_ptr;
}

void* tracked_calloc_internal(size_t count, size_t size, const char* file, int line)
{
	size_t total_size = count * size; // actual size

	// We still need our header for the tracker!
	void* ptr = tracked_malloc_internal(total_size, file, line);
	if (ptr)
	{
		memset(ptr, 0, total_size);
	}

	return (ptr);
}

void* tracked_realloc_internal(void* ptr, size_t new_size, const char* file, int line)
{
	// If ptr is NULL, it's just a malloc
	if (ptr == NULL)
	{
		return tracked_malloc_internal(new_size, file, line);
	}

	// If size is 0, it's just a free
	if (new_size == 0)
	{
		tracked_free(ptr);
		return NULL;
	}

	// Recover old header
	SMemoryBlockHeader* old_header = (SMemoryBlockHeader*)((char*)ptr - sizeof(SMemoryBlockHeader));

	// Safety check: Validate the magic number before doing anything
	if (old_header->magic != 0xDEADBEEF)
	{
		fprintf(stderr, "Critical: realloc on invalid/corrupt pointer!\n");
		abort();
	}

	// Allocate the NEW block
	void* new_ptr = tracked_malloc_internal(new_size, file, line);
	if (!new_ptr)
	{
		// Recovery: If realloc fails, the old pointer is still valid
		// but our stats are now wrong. You should handle this!
		syserr("Realloc failed!");
		return NULL;
	}

	// Update header and stats for the new size
	SMemoryBlockHeader* header = (SMemoryBlockHeader*)new_ptr;
	size_t copy_size = (old_header->size < new_size) ? old_header->size : new_size;
	memcpy(new_ptr, ptr, copy_size);

#if defined(ENABLE_MEMORY_LOGS)
	syslog("Reallocated: %zu bytes (Old: %zu)", new_size, old_header->size);
#endif

	// Free the old block
	tracked_free_internal(ptr, file, line);

	return new_ptr;
}

char* tracked_strdup_internal(const char* szSource, const char* file, int line)
{
	if (!szSource)
	{
		char* emptyStr = (char*)tracked_malloc_internal(1, file, line);
		if (emptyStr)
		{
			emptyStr[0] = '\0';
		}
		return emptyStr;
	}

	size_t len = strlen(szSource) + 1; // +1 for the null terminator '\0'
	char* newStr = (char*)tracked_malloc_internal(len, file, line);

	if (newStr)
	{
		memcpy(newStr, szSource, len);
	}

	return (newStr);
}

void tracked_free_internal(void* pObject, const char* file, int line)
{
	if (pObject == NULL)
	{
		return;
	}

	// 1. Move the pointer back to find the header
	// Shift back by 16 bytes to find the real start
	SMemoryBlockHeader* raw_ptr = (SMemoryBlockHeader*)((char*)pObject - sizeof(SMemoryBlockHeader));

	// Validation Check
	if (raw_ptr->magic != 0xDEADBEEF)
	{
		fprintf(stderr, "MEMORY CORRUPTION! \n");
		fprintf(stderr, "Attempted free at: %s:%d\n", get_filename(file), line);

		if (raw_ptr->magic == 0)
		{
			fprintf(stderr, "Error: DOUBLE FREE detected! (Already freed elsewhere)\n");
		}
		else
		{
			fprintf(stderr, "Error: Pointer was never allocated or is corrupted.\n");
		}
	}

	// 2. Update stats using the "hidden" size
	allocation_count--;
	bytes_allocated -= raw_ptr->size;

#if defined(ENABLE_MEMORY_LOGS)
	syslog("Automatically detected and will free: %zu bytes (%s:%d)", raw_ptr->size, get_filename(file), line);
#endif

	// 3. Free the original starting address
	// Clear the magic before freeing to prevent "Double Free"
	raw_ptr->magic = 0;

	memset(pObject, 0xFE, raw_ptr->size); // Easy to track use-after-free bugs
	_mm_free(raw_ptr);
}

void* cjson_tracked_malloc(size_t size)
{
	// We manually pass "cJSON" as the file so you know where the leak originated
	return tracked_malloc_internal(size, "cJSON_Internal", 0);
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
