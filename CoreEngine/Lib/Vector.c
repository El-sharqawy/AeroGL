#include "vector.h"
#include "../Core/CoreUtils.h"
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

AnubisVector VectorInit(size_t element_size)
{
	AnubisVector vec = (AnubisVector)tracked_malloc(sizeof(SAnubisVector));
	vec->pData = NULL;
	vec->count = 0;
	vec->capacity = 0;
	vec->elemSize = element_size;
	return (vec);
}

void VectorPush(AnubisVector vec, const void* pItem)
{
	if (vec->count == vec->capacity)
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
}

void VectorFree(AnubisVector* ppVec)
{
	if (!ppVec || !*ppVec) 
	{
		return;
	}

	// 2. Grab the actual AnubisVector pointer so we can work with it easily
	AnubisVector vec = *ppVec;

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
void VectorClear(AnubisVector vec)
{
	if (!vec)
	{
		return;
	}
	vec->count = 0; // Just reset the count; keep the capacity and pData!
}

void* VectorGet(AnubisVector vec, size_t index)
{
	if (index >= vec->count)
	{
		return (NULL);
	}

	return (unsigned char*)vec->pData + (index * vec->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte
}