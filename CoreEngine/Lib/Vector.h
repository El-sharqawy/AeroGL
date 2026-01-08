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

typedef SAnubisVector* AnubisVector;

AnubisVector VectorInit(size_t element_size);
void VectorPush(AnubisVector vec, const void* pItem);
void VectorPush(AnubisVector vec, const void* pItem);
void VectorFree(AnubisVector* vec);
void VectorClear(AnubisVector vec);
void* VectorGet(AnubisVector v, size_t index);

#define ANUBIS_VECTOR_PUSH(vec_ptr, type, item) \
    do { \
        type _temp = (item); \
        VectorPush((vec_ptr), &_temp); \
    } while (0)

#endif // __VECTOR_H__