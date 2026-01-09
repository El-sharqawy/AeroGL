#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdio.h>
#include <stdint.h>

typedef struct SAnubisVector
{
	void* pData;      // Raw memory block
	size_t count;     // Number of elements
	size_t capacity;  // Allocated slots
	size_t elemSize;  // Size of one element (e.g., sizeof(SVertex))
} SAnubisVector;

typedef SAnubisVector* Vector;

Vector VectorInit(size_t element_size);
Vector VectorInitSize(size_t element_size, size_t initCapcity);
void VectorPush(Vector vec, const void* pItem);
void VectorPush(Vector vec, const void* pItem);
void VectorFree(Vector* vec);
void VectorClear(Vector vec);
void* VectorGet(Vector v, size_t index);

#define ANUBIS_VECTOR_PUSH(vec_ptr, type, item) \
    do { \
        type _temp = (item); \
        VectorPush((vec_ptr), &_temp); \
    } while (0)

#endif // __VECTOR_H__