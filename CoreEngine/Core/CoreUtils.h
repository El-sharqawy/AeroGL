#ifndef __CORE_UTILS_H__
#define __CORE_UTILS_H__

#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define syserr(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr);
#define syslog(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);

#define S_ISDIR(m)	(m & _S_IFDIR)
#define MAX_STRING_LEN 256

inline GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        char error[256] = { 0 };
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            break;
        case GL_INVALID_VALUE:
            break;
        case GL_INVALID_OPERATION:
            break;
        case GL_STACK_OVERFLOW:
            break;
        case GL_STACK_UNDERFLOW:
            break;
        case GL_OUT_OF_MEMORY:
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            break;
        }
        syserr("OpenGL Error: %d | File: %s (line: %d)", errorCode, file, line);
    }
    return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__) 

#ifdef _MSC_VER
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
typedef struct SMemoryBlockHeader
{
    size_t size;
    unsigned int magic;   // To detect corruption (e.g., 0xDEADBEEF)
    int line;
    const char* file;
    char padding[4];  // Ensure the whole struct is 32 bytes
} SMemoryBlockHeader;

#ifdef __cplusplus
static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#else
_Static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#endif

// "extern" tells other files: "The real variable exists somewhere else."
static const size_t header_size = 16;
extern size_t allocation_count;
extern size_t bytes_allocated;

extern GLint glMajorVersion;
extern GLint glMinorVersion;

void* tracked_malloc_internal(size_t size, const char* file, int line);
void* tracked_calloc_internal(size_t count, size_t size, const char* file, int line);
void* tracked_realloc_internal(void* ptr, size_t new_size, const char* file, int line);
char* tracked_strdup_internal(const char* szSource, const char* file, int line);
void tracked_free_internal(void* pObject, const char* file, int line);

#define tracked_malloc(size) tracked_malloc_internal(size, __FILE__, __LINE__)
#define tracked_calloc(count, size) tracked_calloc_internal(count, size, __FILE__, __LINE__)
#define tracked_realloc(ptr, new_size) tracked_realloc_internal(ptr, new_size, __FILE__, __LINE__)
#define tracked_strdup(szSource) tracked_strdup_internal(szSource,  __FILE__, __LINE__)
#define tracked_free(pObject) tracked_free_internal(pObject, __FILE__, __LINE__)

void* cjson_tracked_malloc(size_t size);
void cjson_tracked_free(void* ptr);

const char* get_filename_ext(const char* filename);
const char* get_filename(const char* filepath);

bool IsGLVersionHigher(GLint MajorVer, GLint MinorVer);

bool MakeDirectory(const char* fullPath);
bool IsDirectoryExists(const char* path);

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>  // For _mkdir on Windows
#include <io.h>
#define MKDIR(path) _mkdir(path)
#undef access
#define access _access
#else
#include <sys/stat.h> // For mkdir on Linux/Unix
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755) // rwxr-xr-x permissions
#define _stricmp(__dst, __src) strcasecmp(__dst, __src)
#endif

#endif // __CORE_UTILS_H__