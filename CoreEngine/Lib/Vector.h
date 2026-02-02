#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdbool.h>
#include <stdio.h>

typedef void (*VectorDestructorFn)(void* element);

typedef struct SVector
{
	void* pData;      // Raw memory block
	size_t count;     // Number of elements
	size_t capacity;  // Allocated slots
	size_t elemSize;  // Size of one element (e.g., sizeof(SVertex))
	VectorDestructorFn destructor;  // NULL = raw copy, no free
	bool isSinglePtr;
	bool isSmartVec;
} SVector;

typedef struct SVector* Vector;

bool Vector_Init(Vector* ppVector, size_t elementSize, bool isSinglePtr);
bool Vector_InitCapacity(Vector* ppVector, size_t elementSize, size_t capacity, bool isSinglePtr);

void Vector_Destroy(Vector* ppVector);

void Vector_Reserve(Vector pVector, size_t reservedSize);
void Vector_Resize(Vector pVector, size_t newCount);
void Vector_Clear(Vector pVector);

void Vector_PushBack(Vector pVector, const void* element);
void Vector_PopBack(Vector pVector);
void Vector_RemoveAt(Vector pVector, size_t index);
void Vector_Swap(Vector pVector, size_t indexA, size_t indexB);
void Vector_SwapAndPop(Vector pVector, size_t index); // effective to remove elements with high speed

void* Vector_Get(Vector vec, size_t index);
void* Vector_GetPtr(Vector vec, size_t index);

#define Vector_PushBackValue(pVec, value) do {                               \
	bool isSmart = pVec->isSmartVec;						\
	typeof(value) _temp = (value);                                 \
	pVec->isSmartVec = false;										\
    Vector_PushBack(pVec, &_temp);                                  \
	pVec->isSmartVec = isSmart;										\
} while (0)

#define Vector_GetValue(pVec, index, type) (*(type*)Vector_Get(pVec, index))

#endif // __NEW_VECTOR_H__