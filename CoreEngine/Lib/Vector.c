#include "vector.h"
#include "../Core/CoreUtils.h"
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

Vector VectorInit(size_t element_size)
{
	Vector vec = (Vector)tracked_malloc(sizeof(SAnubisVector));
	vec->pData = NULL;
	vec->count = 0;
	vec->capacity = 0;
	vec->elemSize = element_size;
	return (vec);
}

Vector VectorInitSize(size_t element_size, size_t initCapcity)
{
	Vector vec = (Vector)tracked_malloc(sizeof(SAnubisVector));
	vec->pData = NULL;

	vec->count = 0;
	vec->capacity = initCapcity;
	vec->elemSize = element_size;
	return (vec);
}

bool VectorPush(Vector vec, const void* pItem)
{
	if (!vec)
	{
		return (false);
	}

	if (vec->count == vec->capacity || vec->pData == NULL)
	{
		size_t newCapacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
		void* newPtr = tracked_realloc(vec->pData, newCapacity * vec->elemSize);
		if (newPtr)
		{
			vec->pData = (unsigned char*)newPtr;
			vec->capacity = newCapacity;
		}
	}

	// Calculate memory address: start + (index * size)
	void* pDest = (unsigned char*)vec->pData + (vec->count * vec->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte
	memcpy(pDest, pItem, vec->elemSize);
	vec->count++;

	return (true);
}

bool VectorReserve(Vector vec, size_t reservedSize)
{
	if (!vec)
	{
		return (false);
	}

	// If already have enough capacity, do nothing
	if (reservedSize <= vec->capacity)
	{
		return (true);
	}

	// tracked_realloc handles everything: alloc, copy, free
	void* newPtr = tracked_realloc(vec->pData, reservedSize * vec->elemSize);
	if (!newPtr)
	{
		return (false);
	}

	vec->pData = (unsigned char*)newPtr;
	vec->capacity = reservedSize;
	return (true);
}

void VectorFree(Vector* ppVec)
{
	if (!ppVec || !*ppVec) 
	{
		return;
	}

	// 2. Grab the actual AnubisVector pointer so we can work with it easily
	Vector vec = *ppVec;

	// 3. Free the internal array data
	tracked_free(vec->pData);
	vec->pData = NULL;

	// Crucial: Set to zero so the vector is "dead" but safe
	vec->count = 0;
	vec->capacity = 0;

	// Free vec from memory
	tracked_free(vec);
	*ppVec = NULL; // Now the caller's pointer is actually NULL
}

// Clear (Keep Memory): Useful if you want to reuse the buffer next frame 
// without re-allocating (saves CPU time!)
bool VectorClear(Vector vec)
{
	if (!vec)
	{
		return (false);
	}
	vec->count = 0; // Just reset the count; keep the capacity and pData!
}

void* VectorGet(Vector vec, size_t index)
{
	if (index >= vec->count)
	{
		return (NULL);
	}

	return (unsigned char*)vec->pData + (index * vec->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte
}