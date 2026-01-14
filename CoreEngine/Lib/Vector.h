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
} SVector;

typedef struct SVector* Vector;

bool Vector_Init(Vector* ppVector, size_t elementSize);
bool Vector_InitCapacity(Vector* ppVector, size_t elementSize, size_t capacity);
void Vector_Destroy(Vector* ppVector);
void Vector_Clear(Vector pVector);
void Vector_Reserve(Vector pVector, size_t reservedSize);

bool Vector_PushBack(Vector* ppVector, const void* element);
void* Vector_Get(Vector vec, size_t index);

#define VECTOR_GET(vec, index, type) \
    (*(type*)Vector_Get(vec, index));

#define VECTOR_PUSH(vec_ptr, value) \
    do { \
        typeof(value) _temp = (value); \
        Vector_PushBack(&(vec_ptr), &_temp); \
    } while (0)

#endif // __VECTOR_H__