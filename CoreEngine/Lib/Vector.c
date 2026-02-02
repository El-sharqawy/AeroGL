#include "Vector.h"
#include "../Core/CoreUtils.h"

bool Vector_Init(Vector* ppVector, size_t elementSize, bool isSinglePtr)
{
	if (ppVector == NULL)
	{
		syserr("ppVector is NULL (invalid address)");
		return false;
	}

	*ppVector = tracked_calloc(1, sizeof(SVector));

	if (!*ppVector)
	{
		syserr("Failed to Allocate New Vector");
		return (false);
	}

	(*ppVector)->count = 0;					// Number of elements
	(*ppVector)->capacity = 0;				// Allocated slots
	(*ppVector)->elemSize = elementSize;	// Size of one element (e.g., sizeof(SVertex))
	(*ppVector)->destructor = NULL;			// NULL = raw copy, no free
	(*ppVector)->isSinglePtr = isSinglePtr;
	(*ppVector)->isSmartVec = true;

	return (true);
}

bool Vector_InitCapacity(Vector* ppVector, size_t elementSize, size_t capacity, bool isSinglePtr)
{
	if (!Vector_Init(ppVector, elementSize, isSinglePtr)) // Reuse Init logic
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
			void* elementAddr = Vector_Get(vec, i); // This is a element** (double ptr)

			if (vec->isSinglePtr)
			{
				// 2. Extract the pointer stored inside (void*)
				void* actualObject = *(void**)elementAddr;
				if (actualObject)
				{
					vec->destructor(actualObject);
				}
			}
			else
			{
				vec->destructor(elementAddr);
			}
		}
	}

	tracked_free(vec->pData);
	vec->pData = NULL;

	tracked_free(vec);
	*ppVector = NULL;
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
		syserr("Vector Reserve failed to allocate %zu bytes", reservedSize * pVector->elemSize);
		return;
	}

	// NEW: Zero out the newly allocated capacity to avoid garbage pointers
	size_t newBytes = (reservedSize - pVector->capacity) * pVector->elemSize;
	unsigned char* startOfNewMemory = (unsigned char*)newPtr + (pVector->capacity * pVector->elemSize);
	memset(startOfNewMemory, 0, newBytes);

	pVector->pData = newPtr;
	pVector->capacity = reservedSize;
}

void Vector_Resize(Vector pVector, size_t newCount)
{
	if (!pVector)
	{
		return;
	}

	// Case 1: Shrinking
	if (newCount < pVector->count)
	{
		if (pVector->destructor)
		{
			// Clean up elements that are being "cut off"
			for (size_t i = newCount; i < pVector->count; i++)
			{
				void* elementAddr = Vector_Get(pVector, i);
				if (pVector->isSinglePtr)
				{
					void* actualObject = *(void**)elementAddr;
					if (actualObject) pVector->destructor(actualObject);
				}
				else
				{
					pVector->destructor(elementAddr);
				}
			}
		}
		pVector->count = newCount;
	}
	else if (newCount > pVector->count) // Case 2: Growing
	{
		// Ensure we have enough capacity first
		if (newCount > pVector->capacity)
		{
			Vector_Reserve(pVector, newCount);
		}

		// Note: Vector_Reserve already memsets the new space to 0,
		// so these new "active" slots are safely NULL/0.
		pVector->count = newCount;
	}
}

void Vector_Clear(Vector pVector)
{
	if (!pVector)
	{
		return;
	}

	// Free elements from Memory
	for (int32_t i = 0; i < pVector->count; i++)
	{
		if (pVector->destructor)
		{
			void* elementAddr = Vector_Get(pVector, i);

			if (pVector->isSinglePtr)
			{
				void* actualObj = *(void**)elementAddr;
				if (actualObj)
				{
					pVector->destructor(actualObj);
				}
			}
			else
			{
				pVector->destructor(elementAddr);
			}
		}
	}

	// Wipe the memory block to prevent accidental "use-after-clear"
	// This ensures all slots become NULL/0 again.
	if (pVector->pData && pVector->count > 0)
	{
		memset(pVector->pData, 0, pVector->count * pVector->elemSize);
	}

	// Reset Count
	pVector->count = 0;
}

void Vector_PushBack(Vector pVector, const void* element)
{
	if (!pVector)
	{
		return;
	}

	if (pVector->count == pVector->capacity || pVector->pData == NULL)
	{
		size_t newCapacity = (pVector->capacity == 0) ? 4 : pVector->capacity * 2; // 4 elements capacity
		void* newPtr = tracked_realloc(pVector->pData, newCapacity * pVector->elemSize);
		if (!newPtr)
		{
			syserr("Realloc failed! capacity=%zu", newCapacity);
			return;  // Original pData remains valid
		}

		// Calculate where the new memory starts and how much was added
		unsigned char* startOfNewMemory = (unsigned char*)newPtr + (pVector->capacity * pVector->elemSize);
		size_t newBytesAdded = (newCapacity - pVector->capacity) * pVector->elemSize;

		// Clear only the newly added section
		memset(startOfNewMemory, 0, newBytesAdded);

		pVector->pData = newPtr;
		pVector->capacity = newCapacity;
	}

	void* pDest = (unsigned char*)pVector->pData + (pVector->count * pVector->elemSize); // FIX: Cast pData to (unsigned char*) so the + operation knows to move byte-by-byte

	if (pVector->isSmartVec)
	{
		*(const void**)pDest = element; // Directly store the pointer value
	}
	else
	{
		memcpy(pDest, element, pVector->elemSize);
	}

	// syslog("Pushing element ptr %p", element);
	pVector->count++;
}

void Vector_PopBack(Vector pVector)
{
	// Safety check: index must be within the active count
	if (!pVector || pVector->count == 0)
	{
		return;
	}

	if (pVector->destructor)
	{
		void* elementAddr = Vector_Get(pVector, pVector->count - 1);

		if (pVector->isSinglePtr)
		{
			void* actualObj = *(void**)elementAddr;
			if (actualObj)
			{
				pVector->destructor(actualObj);
			}
		}
		else
		{
			pVector->destructor(elementAddr);
		}
	}

	pVector->count--;

	// Optional: Wipe the memory so the "popped" slot is zeroed out
	// 3. Wipe the slot using RAW math to avoid the "Get" safety check
	unsigned char* base = (unsigned char*)pVector->pData;
	void* slotToClear = base + (pVector->count * pVector->elemSize);
	// Safety check before memset
	if (slotToClear)
	{
		memset(slotToClear, 0, pVector->elemSize);
	}
}

void Vector_RemoveAt(Vector pVector, size_t index)
{
	// Safety check: index must be within the active count
	if (!pVector || index >= pVector->count)
	{
		return;
	}

	// Destructor logic (standard for our engine)
	if (pVector->destructor)
	{
		void* elementAddr = Vector_Get(pVector, pVector->count - 1);

		if (pVector->isSinglePtr)
		{
			void* actualObj = *(void**)elementAddr;
			if (actualObj)
			{
				pVector->destructor(actualObj);
			}
		}
		else
		{
			pVector->destructor(elementAddr);
		}
	}

	// Shift elements to fill the gap
	// We only need to shift if we aren't removing the very last element
	if (index < pVector->count - 1)
	{
		unsigned char* pData = (unsigned char*)pVector->pData;
		void* dest = pData + (index * pVector->elemSize);
		void* src = pData + ((index + 1) * pVector->elemSize);
		size_t numElementsToShift = pVector->count - index - 1;

		// memmove because source and destination overlap!
		memmove(dest, src, numElementsToShift * pVector->elemSize);
	}

	// Update count and zero out the now-unused last slot
	pVector->count--;
	void* lastSlot = (unsigned char*)pVector->pData + (pVector->count * pVector->elemSize);
	memset(lastSlot, 0, pVector->elemSize);
}

void Vector_Swap(Vector pVector, size_t indexA, size_t indexB)
{
	if (!pVector || indexA >= pVector->count || indexB >= pVector->count)
	{
		return;
	}

	if (indexA == indexB)
	{
		return;
	}

	unsigned char* pData = (unsigned char*)pVector->pData;
	void* a = pData + (indexA * pVector->elemSize);
	void* b = pData + (indexB * pVector->elemSize);

	size_t size = pVector->elemSize;

	// Use a stack buffer for the swap
	// Note: For very large structs, you'd want a heap temp, 
	// but for engine types (pointers, ints), 512 bytes is plenty.
	// Use a small local buffer for speed
	unsigned char stackTemp[512];
	void* temp = stackTemp;
	bool usedHeap = false;


	// If the struct is crazy big, only then use the slow heap
	if (pVector->elemSize > sizeof(stackTemp))
	{
		// temp = alloca(pVector->elemSize); // could be risky ?

		temp = tracked_malloc(size);
		usedHeap = true;
	}

	memcpy(temp, a, size);
	memcpy(a, b, size);
	memcpy(b, temp, size);

	if (usedHeap)
	{
		tracked_free(temp);
	}
}

void Vector_SwapAndPop(Vector pVector, size_t index)
{
	// Safety check: index must be within the active count
	if (!pVector || index >= pVector->count)
	{
		return;
	}

	// Move the target element to the end of the vector, This is O(1) compared to RemoveAt which is O(n)
	Vector_Swap(pVector, index, pVector->count - 1);

	// Now just pop the back, This will trigger the destructor for the item we swapped to the end
	Vector_PopBack(pVector);
}

void* Vector_Get(Vector vec, size_t index)
{
	if (!vec || !vec->pData || index >= vec->count)
	{
		return NULL;
	}

	// Use unsigned char* to move exactly index * elemSize bytes
	return (unsigned char*)vec->pData + (index * vec->elemSize);
}

/**
 * @brief Directly retrieves a pointer-type element.
 * Use this only if your Vector was initialized with sizeof(void*) or sizeof(SomePointer).
 */
void* Vector_GetPtr(Vector vec, size_t index)
{
	void* slot = Vector_Get(vec, index);
	if (!slot)
	{
		return NULL;
	}

	// This is the "strange" logic moved inside the function
	return *(void**)slot;
}
