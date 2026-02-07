#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "MemoryTags.h"

// #define ENABLE_MEMORY_LOGS

typedef struct SMemoryManager* MemoryManager;

bool MemoryManager_Initialize(MemoryManager* ppMemoryManager);
void MemoryManager_Destroy(MemoryManager* ppMemoryManager);

bool MemoryManager_Validate();
void MemoryManager_DumpLeaks();
void MemoryManager_PrintData();
void MemoryManager_PrintTagReport();
void LockManager(MemoryManager mgr);
void UnlockManager(MemoryManager mgr);

MemoryManager GetMemoryManager();
static MemoryManager psMemoryManager;

void* tracked_malloc_internal(size_t size, const char* file, int line, const char* typeName, EMemoryTag tag);
void* tracked_calloc_internal(size_t count, size_t size, const char* file, int line, const char* typeName, EMemoryTag tag);
void* tracked_realloc_internal(void* ptr, size_t new_size, const char* file, int line, const char* typeName);
char* tracked_strdup_internal(const char* szSource, const char* file, int line, const char* typeName, EMemoryTag tag);
void tracked_free_internal(void* pObject, const char* file, int line);

const char* FormatMemorySize(uint64_t bytes);
void FormatMemorySizeThreadSafe(uint64_t bytes, char* out_buf, size_t buf_size);

// Engine System Tagged Allocation (Explicit Tags)
#define engine_new(type, tag) (type*)tracked_malloc_internal(sizeof(type), __FILE__, __LINE__, #type, tag)
#define engine_new_zero(type, count, tag) (type*)tracked_calloc_internal(count, sizeof(type), __FILE__, __LINE__, #type, tag)
#define engine_new_count_zero(type, count, tag) (type*)tracked_calloc_internal(count, sizeof(type), __FILE__, __LINE__, #type "[]", tag)

// Arrays/Bytes
#define engine_malloc(size, tag) tracked_malloc_internal(size, __FILE__, __LINE__, "raw_bytes", tag)
#define engine_calloc(count, size, tag) tracked_calloc_internal(count, size, __FILE__, __LINE__, "raw_bytes", tag)

// Reallocation (Pass NULL for typeName to keep the old one)
#define engine_realloc(ptr, size) tracked_realloc_internal(ptr, size, __FILE__, __LINE__, NULL)
#define engine_realloc_array(ptr, type, count) (type*)tracked_realloc_internal(ptr, sizeof(type) * (count), __FILE__, __LINE__, #type "[]")

// Strings
#define engine_strdup(szSource, tag) tracked_strdup_internal(szSource, __FILE__, __LINE__, "strdup(" #szSource ")", tag)
#define tracked_strdup(szSource) engine_strdup(szSource, MEM_TAG_STRINGS)

// Cleanup
#define engine_delete(pObject) tracked_free_internal(pObject, __FILE__, __LINE__)
#define engine_free(pObject) tracked_free_internal(pObject, __FILE__, __LINE__)

// General Tracking (Defaults to ENGINE or NONE)
// Basic C-Style Tracking (Defaults to ENGINE tag)
#define tracked_malloc(size)         engine_malloc(size, MEM_TAG_ENGINE)
#define tracked_calloc(n, size)      engine_calloc(n, size, MEM_TAG_ENGINE)
#define tracked_realloc(ptr, size)   engine_realloc(ptr, size)
#define tracked_free(ptr)            tracked_free_internal(ptr, __FILE__, __LINE__)

// "New" Style Object Tracking (Captures Type Name)
#define tracked_new(type)            engine_new(type, MEM_TAG_ENGINE)
#define tracked_new_zero(type, n)    engine_new_zero(type, n, MEM_TAG_ENGINE)
#define tracked_realloc_array(p, t, n) engine_realloc_array(p, t, n)


#endif // __MEMORY_MANAGER_H__