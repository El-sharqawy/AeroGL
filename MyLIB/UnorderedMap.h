#ifndef __UNORDERED_MAP_H__
#define __UNORDERED_MAP_H__

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "../Resources/MemoryTags.h"

// The "Offset Basis" is a specific starting value (seed) that ensures 
// even an empty string doesn't result in a hash of 0.
// It effectively "primes" the internal state of the hash.
static const uint32_t FNV_OFFSET_BASIS_32 = 0x811c9dc5; // 2166136261u

// The "Prime" is a carefully chosen prime number that ensures the bits 
// are shifted and mixed thoroughly across the 32-bit integer range 
// during the multiplication step.
static const uint32_t FNV_PRIME_32 = 0x01000193; // 16777619u

static uint32_t fnv1a_32(const char* buf, size_t buf_size)
{
	// Initialize our hash with the starting offset
	uint32_t hash = FNV_OFFSET_BASIS_32;

	for (size_t i = 0; i < buf_size; i++)
	{
		uint8_t c = buf[i]; // tolower(buf[i]); // Active this for case insensitivity (text == TEXT)

		// XOR the lower 8 bits of the hash with the current byte.
		// This mixes the new data into the existing state.
		hash ^= c;

		/* multiply by FNV prime, overflow on unsigned is well-defined */
		// This "spreads" the impact of the XORed byte across all 32 bits.
		// In C/C++, unsigned integer overflow is well-defined as a 
		// modulo operation (it wraps around), which is exactly what we want.
		hash *= FNV_PRIME_32;
	}

	return (hash);
}

static uint32_t fnv1a_str(const char* str)
{
	uint32_t hash = FNV_OFFSET_BASIS_32;
	while (*str)
	{
		uint8_t c = *str++; // tolower(*str++); // Active this for case insensitivity (text == TEXT)

		// XOR the lower 8 bits of the hash with the current byte.
		// This mixes the new data into the existing state.
		hash ^= c;

		/* multiply by FNV prime, overflow on unsigned is well-defined */
		// This "spreads" the impact of the XORed byte across all 32 bits.
		// In C/C++, unsigned integer overflow is well-defined as a 
		// modulo operation (it wraps around), which is exactly what we want.
		hash *= FNV_PRIME_32;
	}

	return (hash);
}

typedef void (*AeroUnorderedMapDestructor)(void* pValue);

typedef struct SAeroUnorderedMapNode
{
	char* szKey;
	void* pValue;
	uint32_t hash;							// Cache the hash for fast resizing/comparisons
	struct SAeroUnorderedMapNode* next;		// pointer to the next node element
} SAeroUnorderedMapNode;

typedef SAeroUnorderedMapNode* AeroUnorderedMapNode;

typedef struct SAeroUnorderedMap
{
	SAeroUnorderedMapNode** ppBuckets;			// Double pointer for the array of linked lists
	uint32_t bucketCount;
	uint32_t elementCount;
	EMemoryTag tag;								// The "Blame" tag, to locate the leaks
	AeroUnorderedMapDestructor pfnDestructor;	// function to clean up pValue
} SAeroUnorderedMap;

typedef SAeroUnorderedMap* AeroUnorderedMap;

typedef struct SAeroUnorderedMapIterator {
	AeroUnorderedMap _map;      // The map we are walking through
	int32_t _bucketIndex;        // Which bucket are we currently in?
	AeroUnorderedMapNode node;  // The current node we are looking at

	// Publicly accessible data
	char* key;
	void* value;
} SAeroUnorderedMapIterator;

typedef SAeroUnorderedMapIterator* AeroUnorderedMapIterator;

static const int32_t unordered_map_init_val = 11; // Prime number

bool UnorderedMap_Initialize(AeroUnorderedMap* ppUnorderedMap, EMemoryTag tag);
void UnoderedMap_Destroy(AeroUnorderedMap* ppUnorderedMap);

void UnorderedMap_Clear(AeroUnorderedMap map);
bool UnorderedMap_Insert(AeroUnorderedMap map, const char* key, void* value);

void* UnorderedMap_Find(AeroUnorderedMap map, const char* key);
void UnorderedMap_Remove(AeroUnorderedMap map, const char* key);
void UnorderedMap_Resize(AeroUnorderedMap map, uint32_t newSize);

SAeroUnorderedMapIterator UnorderedMap_Begin(AeroUnorderedMap map);
SAeroUnorderedMapIterator UnorderedMap_End(AeroUnorderedMap map);
bool UnorderedMap_IterCompare(SAeroUnorderedMapIterator a, SAeroUnorderedMapIterator b);
bool UnorderedMap_IteratorNext(AeroUnorderedMapIterator iterator);

uint32_t UnorderedMap_Count(AeroUnorderedMap map);
uint32_t UnorderedMap_Capactiy(AeroUnorderedMap map);
bool UnorderedMap_IsEmpty(AeroUnorderedMap map);

#endif // __UNORDERED_MAP_H__