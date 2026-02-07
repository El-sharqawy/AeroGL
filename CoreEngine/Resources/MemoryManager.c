#include "MemoryManager.h"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include "../Stdafx.h"

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
	const char* file;
	const char* typeName;

	struct SMemoryBlockHeader* next;
	struct SMemoryBlockHeader* prev;

	EMemoryTag tag;
	// Current size (x64): 8+4+4+8+8+8+8 = 48 bytes.
	// 48 is a multiple of 16, so no extra padding is strictly needed,
	// but we can keep a small buffer for future safety.
	char padding[12];
} SMemoryBlockHeader;

#ifdef __cplusplus
static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#else
_Static_assert(sizeof(SMemoryBlockHeader) % 16 == 0, "Memory header must be 16-byte aligned!");
#endif

#if defined(_WIN32) || defined(_WIN64)
typedef CRITICAL_SECTION MutexHandle;
#else
#include <pthread.h>
typedef pthread_mutex_t MutexHandle;
#endif

typedef struct SMemoryManager
{
	uint64_t totalAllocated; // total allocated memory in bytes
	uint64_t totalFreed;     // total freed memory in bytes
	uint64_t peakUsage;      // peak memory usage in bytes
	uint64_t currentUsage;   // current memory usage in bytes
	uint64_t allocationCount; // number of allocations

	size_t usageByTag[MEM_TAG_COUNT];

	SMemoryBlockHeader* head; // Head of the "live" allocations list

	// Crucial for multi-threaded operations
	MutexHandle lock;

	bool isInitialized;
	char padding[7]; // to match 16 bytes align
} SMemoryManager;

bool MemoryManager_Initialize(MemoryManager* ppMemoryManager)
{
    if (ppMemoryManager == NULL)
    {
        syserr("ppMemoryManager is NULL (invalid address)");
        return false;
	}

	// Use standard allocation for the manager itself to avoid recursion/locking issues
	void* raw_mem = _mm_malloc(sizeof(SMemoryManager), 16);
	if (!raw_mem)
	{
		return false;
	}

	// This is the most important line to fix your 0xCDCDCD issue!
	memset(raw_mem, 0, sizeof(SMemoryManager));

	if (!(*ppMemoryManager))
    {
		syserr("Failed to Allocate Memory for MemoryManager");
		return (false);
    }

	psMemoryManager = (MemoryManager)raw_mem;
	*ppMemoryManager = psMemoryManager;

#ifdef _WIN32
	InitializeCriticalSection(&psMemoryManager->lock);
#else
	pthread_mutex_init(&psMemoryManager->lock, NULL);
#endif

	psMemoryManager->head = NULL; // Explicitly NULL the head
	(*ppMemoryManager)->isInitialized = true;
    return (true);
}

void MemoryManager_Destroy(MemoryManager* ppMemoryManager)
{
    if (!ppMemoryManager || !(*ppMemoryManager))
    {
        return;
    }

#ifdef _WIN32
	DeleteCriticalSection(&psMemoryManager->lock);
#else
	pthread_mutex_destroy(&psMemoryManager->lock);
#endif

	_mm_free(*ppMemoryManager);
	*ppMemoryManager = NULL;
}

bool MemoryManager_Validate()
{
	if (!psMemoryManager || !psMemoryManager->isInitialized) return true;

	LockManager(psMemoryManager);

	SMemoryBlockHeader* curr = psMemoryManager->head;
	int index = 0;
	bool is_corrupt = false;

	while (curr)
	{
		// 1. Check Magic Number
		if (curr->magic != 0xDEADBEEF)
		{
			syserr("CRITICAL: Memory Corruption detected at block %d!", index);
			syserr("Block allocated at %s:%d (Type: %s)",
				curr->file, curr->line, curr->typeName ? curr->typeName : "Unknown");

			// If the magic is 0xBAADF00D, we found a node that stayed in the list after free
			if (curr->magic == 0xBAADF00D) {
				syserr("Error: Node is marked as FREED but still exists in the live list!");
			}

			is_corrupt = true;
			// We stop here because if the header is corrupt, the 'next' pointer might be garbage
			break;
		}

		// 2. Cross-link validation (The "Perfect" Check)
		// If I have a next, its 'prev' MUST be me.
		if (curr->next && curr->next->prev != curr)
		{
			syserr("CRITICAL: Linked List pointer corruption at %s:%d (Type: %s)", curr->file, curr->line, curr->typeName ? curr->typeName : "UnKnown");
			is_corrupt = true;
			break;
		}

		curr = curr->next;
		index++;
	}

	UnlockManager(psMemoryManager);

	if (is_corrupt)
	{
		// Force a crash so you can see the callstack in the debugger
		syserr("CRITICAL: Pointer corruption...");
#ifdef _MSC_VER
		__debugbreak(); // This stops the code in Visual Studio right on the line!
#endif
		assert(!is_corrupt);
	}

	return !is_corrupt;
}

void MemoryManager_DumpLeaks()
{
	LockManager(psMemoryManager);
	SMemoryBlockHeader* curr = psMemoryManager->head;

	if (!curr)
	{
		syslog("No leaks detected! Great job.");
	}
	else
	{
		syslog("--- MEMORY LEAK REPORT ---");
		while (curr)
		{
			syslog("Leak: %zu bytes allocated at %s:%d", curr->size, curr->file, curr->line);
			curr = curr->next;
		}
	}
	UnlockManager(psMemoryManager);
}

void MemoryManager_PrintData()
{
	LockManager(psMemoryManager);
	SMemoryBlockHeader* curr = psMemoryManager->head;

	if (!curr)
	{
		syslog("No Current Active Elements.");
	}
	else
	{
		syslog("--- MEMORY MANAGER REPORT ---");
		syslog("Allocation Count: %llu", psMemoryManager->allocationCount);

		char totalAllocated[16], currentAllocated[16], totalFreed[16], peak[16];
		FormatMemorySizeThreadSafe(psMemoryManager->totalAllocated, totalAllocated, sizeof(totalAllocated));
		FormatMemorySizeThreadSafe(psMemoryManager->currentUsage, currentAllocated, sizeof(currentAllocated));
		FormatMemorySizeThreadSafe(psMemoryManager->totalFreed, totalFreed, sizeof(totalFreed));
		FormatMemorySizeThreadSafe(psMemoryManager->peakUsage, peak, sizeof(peak));

		syslog("Total Allocated: %s", totalAllocated);
		syslog("Current Usage: %s", currentAllocated);
		syslog("Current Total Freed: %s", totalFreed);
		syslog("Peak Usage: %s", peak);
		
		while (curr)
		{
			if (curr->typeName != NULL)
			{
				syslog("Object Allocated At %s:%d with size: %s, Type: %s", curr->file, curr->line, FormatMemorySize(curr->size), curr->typeName);
			}
			else 
			{
				syslog("Object Allocated At %s:%d with size: %s", curr->file, curr->line, FormatMemorySize(curr->size));
			}
			curr = curr->next;
		}
	}

	UnlockManager(psMemoryManager);
}

void MemoryManager_PrintTagReport()
{
	LockManager(psMemoryManager);
	syslog("--- MEMORY TAG REPORT ---");
	for (int i = 0; i < MEM_TAG_COUNT; i++)
	{
		syslog("%-12s: %s", MemoryTagNames[i], FormatMemorySize(psMemoryManager->usageByTag[i]));
	}
	UnlockManager(psMemoryManager);
}
void LockManager(MemoryManager mgr)
{
	if (!mgr) return;

#ifdef _WIN32
	EnterCriticalSection(&mgr->lock);
#else
	pthread_mutex_lock(&mgr->lock);
#endif
}

void UnlockManager(MemoryManager mgr)
{
	if (!mgr) return;

#ifdef _WIN32
	LeaveCriticalSection(&mgr->lock);
#else
	pthread_mutex_unlock(&mgr->lock);
#endif
}

MemoryManager GetMemoryManager()
{
    assert(psMemoryManager);
    return (psMemoryManager);
}

void* tracked_malloc_internal(size_t size, const char* file, int line, const char* typeName, EMemoryTag tag)
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

	// 3. Fill the header
	SMemoryBlockHeader* header = (SMemoryBlockHeader*)raw_ptr;
	header->size = size;
	header->magic = 0xDEADBEEF;
	header->file = file;
	header->line = line;
	header->typeName = typeName;
	header->tag = tag;

	// 4. Thread-Safe Linked List Insertion
	LockManager(psMemoryManager);

	header->next = psMemoryManager->head;
	header->prev = NULL;
	if (psMemoryManager->head)
	{
		psMemoryManager->head->prev = header;
	}
	psMemoryManager->head = header;

	psMemoryManager->currentUsage += total_size;
	psMemoryManager->totalAllocated += total_size;
	psMemoryManager->allocationCount++;
	if (psMemoryManager->currentUsage > psMemoryManager->peakUsage)
	{
		psMemoryManager->peakUsage = psMemoryManager->currentUsage;
	}

	psMemoryManager->usageByTag[tag] += size;

	// Unlock The Manager
	UnlockManager(psMemoryManager);

	// Return the pointer shifted by exactly 16 bytes
	// (raw_ptr + 2) if raw_ptr is size_t* (8 bytes each)
	void* user_ptr = (void*)((char*)raw_ptr + sizeof(SMemoryBlockHeader));

#ifdef _DEBUG
    if (((uintptr_t)user_ptr % 16) != 0)
    {
        syserr("Alignment Error! Header size: %zu", sizeof(SMemoryBlockHeader));
    }
#endif

#if defined(ENABLE_MEMORY_LOGS)
	// Use the manager's current usage for the log
	syslog("allocated: %zu bytes. Total system usage: %llu (%s:%d)", size, psMemoryManager->currentUsage, file, line);
#endif#endif

	// Return pointer after the header
	return user_ptr;
}

void* tracked_calloc_internal(size_t count, size_t size, const char* file, int line, const char* typeName, EMemoryTag tag)
{
	size_t total_size = count * size; // actual size

	// We still need our header for the tracker!
	void* ptr = tracked_malloc_internal(total_size, file, line, typeName, tag);
	if (ptr)
	{
		memset(ptr, 0, total_size);
	}

	return (ptr);
}

void* tracked_realloc_internal(void* ptr, size_t new_size, const char* file, int line, const char* typeName)
{
	// If ptr is NULL, it's just a malloc
	if (ptr == NULL)
	{
		return tracked_malloc_internal(new_size, file, line, typeName ? typeName : NULL, MEM_TAG_NONE);
	}

	// If size is 0, it's just a free
	if (new_size == 0)
	{
		tracked_free_internal(ptr, file, line);
		return NULL;
	}

	// Move the pointer back to find the header
	SMemoryBlockHeader* old_header = (SMemoryBlockHeader*)((char*)ptr - sizeof(SMemoryBlockHeader));

	// Safety check: Validate the magic number before doing anything
	if (old_header->magic != 0xDEADBEEF)
	{
		fprintf(stderr, "Critical: realloc on invalid/corrupt pointer!\n");
		abort();
	}

	// Allocate the NEW block
	const char* finalTypeName = (typeName != NULL) ? typeName : old_header->typeName;

	void* new_ptr = tracked_malloc_internal(new_size, file, line, finalTypeName, old_header->tag);
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

char* tracked_strdup_internal(const char* szSource, const char* file, int line, const char* typeName, EMemoryTag tag)
{
	if (!szSource)
	{
		char* emptyStr = (char*)tracked_malloc_internal(1, file, line, typeName ? typeName : "char*", tag);
		if (emptyStr)
		{
			emptyStr[0] = '\0';
		}
		return emptyStr;
	}

	size_t len = strlen(szSource) + 1; // +1 for the null terminator '\0'
	char* newStr = (char*)tracked_malloc_internal(len, file, line, typeName ? typeName : "char*", tag);

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
	SMemoryBlockHeader* header = (SMemoryBlockHeader*)((char*)pObject - sizeof(SMemoryBlockHeader));

	// Validation Check
	if (header->magic != 0xDEADBEEF)
	{
		fprintf(stderr, "MEMORY CORRUPTION! \n");
		// fprintf(stderr, "Attempted free at: %s:%d\n", get_filename(file), line);

		if (header->magic == 0xBAADF00D)
		{
			fprintf(stderr, "Error: DOUBLE FREE detected! (Already freed elsewhere)\n");
		}
		else
		{
			fprintf(stderr, "Error: Pointer was never allocated or is corrupted.\n");
		}
	}

	// 3. Thread-Safe Unlinking
	LockManager(psMemoryManager);

	if (header->prev)
	{
		// Not the head: point the previous node to our next node
		header->prev->next = header->next;
	}
	else
	{
		// This IS the head: move the head pointer to our next node
		psMemoryManager->head = header->next;
	}

	if (header->next)
	{
		// Not the tail: point the next node back to our previous node
		header->next->prev = header->prev;
	}

	// 4. Update Stats
	size_t total_size = header->size + sizeof(SMemoryBlockHeader);
	psMemoryManager->currentUsage -= total_size;
	psMemoryManager->totalFreed += total_size;
	psMemoryManager->allocationCount--;

	// Update tags
	psMemoryManager->usageByTag[header->tag] -= header->size;

	UnlockManager(psMemoryManager);

#if defined(ENABLE_MEMORY_LOGS)
	syslog("Automatically detected and will free: %zu bytes (%s:%d)", header->size, get_filename(file), line);
#endif

	// 5. Clean up the evidence (Defensive Programming)
	header->magic = 0xBAADF00D; // Custom "Already Freed" magic

	// Fill user memory with a garbage pattern to catch "use-after-free"
	memset(pObject, 0xFE, header->size); // Easy to track use-after-free bugs

	_mm_free(header);
}

const char* FormatMemorySize(uint64_t bytes)
{
	static char buffer[32]; // Static buffer for quick logging (not thread-safe!)
	const char* units[] = { "Bytes", "KB", "MB", "GB", "TB" };
	int unit_index = 0;
	double size = (double)bytes;

	while (size >= 1024 && unit_index < 4) {
		size /= 1024;
		unit_index++;
	}

	// Format to 2 decimal places
	snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unit_index]);
	return buffer;
}

void FormatMemorySizeThreadSafe(uint64_t bytes, char* out_buf, size_t buf_size)
{
	const char* units[] = { "Bytes", "KB", "MB", "GB", "TB" };
	int unit_index = 0;
	double size = (double)bytes;

	while (size >= 1024 && unit_index < 4)
	{
		size /= 1024;
		unit_index++;
	}

	snprintf(out_buf, buf_size, "%.2f %s", size, units[unit_index]);
}

