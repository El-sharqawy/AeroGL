#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define syserr(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr);
#define syslog(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);

// #define ENABLE_MEMORY_LOGS

static const size_t header_size = 16;

#ifdef _MSC_VER
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
typedef struct SMemoryBlockHeader
{
	size_t size;
	uint32_t magic;   // To detect corruption (e.g., 0xDEADBEEF)
	uint32_t line;
	char* file;

	struct SMemoryBlockHeader* next;
	struct SMemoryBlockHeader* prev;

	char padding[8];  // Ensure the whole struct is 32 bytes
} SMemoryBlockHeader;

#ifdef __cplusplus
static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#else
_Static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#endif

typedef struct SMemoryManager
{
	uint64_t totalAllocated; // total allocated memory in bytes
	uint64_t totalFreed;     // total freed memory in bytes
	uint64_t peakUsage;      // peak memory usage in bytes
	uint64_t currentUsage;   // current memory usage in bytes
	uint64_t allocationCount; // number of allocations

	SMemoryBlockHeader* head; // Head of the "live" allocations list
	bool isInitialized;
} SMemoryManager;

typedef struct SMemoryManager* MemoryManager;

bool MemoryManager_Initialize(MemoryManager* ppMemoryManager);
void MemoryManager_Destroy(MemoryManager* ppMemoryManager);

MemoryManager GetMemoryManager();
static MemoryManager psMemoryManager;

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

#endif // __MEMORY_MANAGER_H__