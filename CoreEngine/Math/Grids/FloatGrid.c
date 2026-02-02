#include "FloatGrid.h"
#include "../../Core/CoreUtils.h"
#include <memory.h>

bool FloatGrid_Initialize(FloatGrid* ppFloatGrid, int32_t width, int32_t height)
{
	if (width <= 0 || height <= 0)
	{
		syserr("Cannot Create 0 sized grid");
		return false;
	}

	if (ppFloatGrid == NULL)
	{
		syserr("ppFloatGrid is NULL (invalid address)");
		return false;
	}

	*ppFloatGrid = (FloatGrid)tracked_calloc(1, sizeof(SFloatGrid));

	if (!(*ppFloatGrid)) // Check immediately
	{
		syserr("Failed to Allocate Memory for Float Grid");
		return false;
	}

	FloatGrid pFloatGrid = *ppFloatGrid;
	pFloatGrid->width = width;
	pFloatGrid->height = height;
	pFloatGrid->size = width * height;

	pFloatGrid->pArray = (float*)tracked_calloc(1, pFloatGrid->size * sizeof(float)); // [y][x]

	if (!pFloatGrid->pArray)
	{
		FloatGrid_Destroy(ppFloatGrid);
		syserr("Failed to Allocate Floats Array for Float Grid");
		return (false);
	}

	pFloatGrid->isInitialized = true;
	return (true);
}

void FloatGrid_Destroy(FloatGrid* ppFloatGrid)
{
	if (!ppFloatGrid || !*ppFloatGrid)
	{
		return;
	}

	FloatGrid pFloatGrid = *ppFloatGrid;

	tracked_free(pFloatGrid->pArray);
	pFloatGrid->pArray = NULL;

	tracked_free(pFloatGrid);
	*ppFloatGrid = NULL;
}

void FloatGrid_Clear(FloatGrid pFloatGrid)
{
	if (!pFloatGrid || !pFloatGrid->pArray)
	{
		return;
	}

	// Sets every byte to 0
	memset(pFloatGrid->pArray, 0, pFloatGrid->size * sizeof(float));
}

void FloatGrid_FillValue(FloatGrid pFloatGrid, float fValue)
{
	if (!pFloatGrid || !pFloatGrid->pArray)
	{
		return;
	}

	// Sets every byte to fValue
	for (int32_t i = 0; i < pFloatGrid->size; i++)
	{
		pFloatGrid->pArray[i] = fValue;
	}
}

float FloatGrid_GetAt(FloatGrid pFloatGrid, int32_t row, int32_t col)
{
	if (!pFloatGrid || !pFloatGrid->pArray)
	{
		syserr("Array grid is not set");
		return (0.0f);
	}

	if (row < 0 || row >= pFloatGrid->rows || col < 0 || col >= pFloatGrid->cols)
	{
		syserr("Error! out of float grid array bounds (%d, %d) - (%d, %d)", pFloatGrid->rows, pFloatGrid->cols, row, col);
		// Out of bounds - return a safe default
		return 0.0f;
	}
		
	return pFloatGrid->pArray[row * pFloatGrid->cols + col];
}

void FloatGrid_SetAt(FloatGrid pFloatGrid, int32_t row, int32_t col, float fValue)
{
	if (!pFloatGrid || !pFloatGrid->pArray)
	{
		syserr("Array grid is not set");
		return;
	}

	if (row < 0 || row >= pFloatGrid->rows || col < 0 || col >= pFloatGrid->cols)
	{
		syserr("Error! out of float grid array bounds (%d, %d) - (%d, %d)", pFloatGrid->rows, pFloatGrid->cols, row, col);
		// Out of bounds - return a safe default
		return;
	}

	pFloatGrid->pArray[row * pFloatGrid->cols + col] = fValue;
}

const float* FloatGrid_GetRow(FloatGrid pFloatGrid, int32_t row)
{
	if (!pFloatGrid || !pFloatGrid->pArray)
	{
		syserr("Array grid is not set");
		return (NULL);
	}

	if (row < 0 || row >= pFloatGrid->rows)
	{
		syserr("Error! out of float grid array bounds (%d) - (%d)", pFloatGrid->rows, row);
		// Out of bounds - return a safe default
		return (NULL);
	}

	// Since rows are contiguous, ptr[0]...ptr[width-1] are all valid.
	return &pFloatGrid->pArray[row * pFloatGrid->cols];
}

size_t FloatGrid_GetBytesSize(FloatGrid pFloatGrid)
{
	return (pFloatGrid->size * sizeof(float));
}
