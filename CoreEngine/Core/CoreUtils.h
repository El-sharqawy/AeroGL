#ifndef __CORE_UTILS_H__
#define __CORE_UTILS_H__

#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>

#define nullptr NULL
#define syserr(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr);
#define syslog(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);

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

// "extern" tells other files: "The real variable exists somewhere else."
static const size_t header_size = 16;
extern size_t allocation_count;
extern size_t bytes_allocated;

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

const char* get_filename_ext(const char* filename);
const char* get_filename(const char* filepath);

bool IsGLVersionHigher(GLint MajorVer, GLint MinorVer);

float clampf(float val, float min, float max);

#endif // __CORE_UTILS_H__