#ifndef __FLOAT_GRID_H__
#define __FLOAT_GRID_H__

#include <stdint.h>
#include <stdbool.h>
#include "../../Resources/MemoryTags.h"

typedef struct SFloatGrid
{
	float* pArray; // 2D Array [y][x]
	union
	{
		int32_t width;
		int32_t cols; // X-axis
	};
	union
	{
		int32_t height;
		int32_t rows; // Y-axis
	};
	int32_t size;
	bool isInitialized;
} SFloatGrid;

typedef struct SFloatGrid* FloatGrid;

bool FloatGrid_Initialize(FloatGrid* ppFloatGrid, int32_t width, int32_t height, EMemoryTag tag);
void FloatGrid_Destroy(FloatGrid* ppFloatGrid);
void FloatGrid_Clear(FloatGrid pFloatGrid);
void FloatGrid_FillValue(FloatGrid pFloatGrid, float fValue);
float FloatGrid_GetAt(FloatGrid pFloatGrid, int32_t y, int32_t x);
void FloatGrid_SetAt(FloatGrid pFloatGrid, int32_t y, int32_t x, float fValue);

const float* FloatGrid_GetRow(FloatGrid pFloatGrid, int32_t row);

size_t FloatGrid_GetBytesSize(FloatGrid pFloatGrid);

#endif // __FLOAT_GRID_H__