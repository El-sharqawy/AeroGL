#include "Vector.h"
#include "../Core/CoreUtils.h"
#include <memory.h>

bool Vector_Init(Vector* ppVector, size_t elementSize)
{
	*ppVector = (Vector)tracked_calloc(1, sizeof(SVector));

	if (!*ppVector)
	{
		syserr("Failed to Allocate New Vector");
		return (false);
	}

	(*ppVector)->count = 0;					// Number of elements
	(*ppVector)->capacity = 0;				// Allocated slots
	(*ppVector)->elemSize = elementSize;	// Size of one element (e.g., sizeof(SVertex))
	(*ppVector)->destructor = NULL;			// NULL = raw copy, no free

	return (true);
}

bool Vector_InitCapacity(Vector* ppVector, size_t elementSize, size_t capacity)
{
	if (!Vector_Init(ppVector, elementSize)) // Reuse Init logic
	{
		return false;
	}

	if (capacity > 0)
	{
		Vector_Reserve(*ppVector, capacity);  // capacity=0 safe
	}

	return true;
}

void Vector_Destroy(Vector* ppVector)
{
	if (!ppVector || !*ppVector)
	{
		return;
	}

	Vector vec = *ppVector;

	if (vec->destructor)
	{
		for (size_t i = 0; i < vec->count; ++i)
		{
			void* elem = (char*)vec->pData + i * vec->elemSize;
			vec->destructor(&elem);
		}
	}

	tracked_free(vec->pData);
	vec->pData = NULL;

	tracked_free(vec);
	*ppVector = NULL;
}

void Vector_Clear(Vector pVector)
{
	if (!pVector)
	{
		return;
	}

	pVector->count = 0;
}

void Vector_Reserve(Vector pVector, size_t reservedSize)
{
	if (!pVector)
	{
		return;
	}

	// If already have enough capacity, do nothing
	if (reservedSize <= pVector->capacity)
	{
		return;
	}

	// tracked_realloc handles everything: alloc, copy, free
	void* newPtr = tracked_realloc(pVector->pData, reservedSize * pVector->elemSize);
	if (!newPtr)
	{
		return;
	}

	pVector->pData = (unsigned char*)newPtr;
	pVector->capacity = reservedSize;
}

bool Vector_PushBack(Vector* ppVector, const void* element)
{
	if (!ppVector || !*ppVector || !element)
		return false;

	Vector vec = *ppVector;  // Local copy for safety

	if (vec->count == vec->capacity || vec->pData == NULL)
	{
		size_t newCapacity = (vec->capacity == 0) ? 4 : vec->capacity * 2; // 4 elements capacity
		void* newPtr = tracked_realloc(vec->pData, newCapacity * vec->elemSize);
		if (!newPtr)
		{
			syserr("Realloc failed! capacity=%zu", newCapacity);
			return (false);  // Original pData remains valid
		}

		vec->pData = newPtr;
		vec->capacity = newCapacity;
	}

	void* pDest = (unsigned char*)vec->pData + (vec->count * vec->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte
	memcpy(pDest, element, vec->elemSize);

	// syslog("Pushing element ptr %p", element);
	vec->count++;

	*ppVector = vec;  // Copy changes back!

	return (true);
}

void* Vector_Get(Vector vec, size_t index)
{
	if (index >= vec->count)
	{
		return (NULL);
	}

	return (unsigned char*)vec->pData + (index * vec->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte
}