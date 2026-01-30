#ifndef __MATH_UTILS_H__
#define __MATH_UTILS_H__

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64)
#define M_E        2.71828182845904523536   // e
#define M_LOG2E    1.44269504088896340736   // log2(e)
#define M_LOG10E   0.434294481903251827651  // log10(e)
#define M_LN2      0.693147180559945309417  // ln(2)
#define M_LN10     2.30258509299404568402   // ln(10)
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2
#define M_PI_4     0.785398163397448309616  // pi/4
#define M_1_PI     0.318309886183790671538  // 1/pi
#define M_2_PI     0.636619772367581343076  // 2/pi
#define M_2_SQRTPI 1.12837916709551257390   // 2/sqrt(pi)
#define M_SQRT2    1.41421356237309504880   // sqrt(2)
#define M_SQRT1_2  0.707106781186547524401  // 1/sqrt(2)
#endif

#define ToRadians(degrees) (float)(((degrees) * M_PI) / 180.0f)
#define ToDegrees(radians) (float)(((radians) * 180.0f) / M_PI)

typedef struct SEulerAngles
{
	float fYaw;
	float fPitch;
	float fRoll;
} SEulerAngles;

typedef enum EAngleUnit
{
	ANGLE_DEGREE,
	ANGLE_RADIAN
} EAngleUnit;

typedef enum EMatrixLayout
{
	MATRIX_ROW_MAJOR,
	MATRIX_COLUMN_MAJOR
} EMatrixLayout;


typedef enum EQuaternion
{
	QUATERNION_LEFT_HANDED,
	QUATERNION_RIGHT_HANDED
} EQuaternion;

typedef enum EEulerAngleOrder
{
	EULER_ANGLE_ORDER_XYZ,
	EULER_ANGLE_ORDER_XZY,
	EULER_ANGLE_ORDER_YXZ,
	EULER_ANGLE_ORDER_YZX,
	EULER_ANGLE_ORDER_ZXY,
	EULER_ANGLE_ORDER_ZYX
} EEulerAngleOrder;

typedef enum EAxis
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
	AXIS_UNDEFINED
} EAxis;

static inline float clampf(float val, float min, float max)
{
	if (val > max)
	{
		return max;
	}
	if (val < min)
	{
		return min;
	}
	return val;
}

static inline float random_float()
{
	// Returns a random real in [0,1).
	return (float)rand() / ((float)RAND_MAX + 1.0f);
}

static inline float random_float_range(float min, float max)
{
	// Returns a random real in [min,max).
	return min + (max - min) * random_float();
}

static inline int32_t clampi(int32_t val, int32_t min, int32_t max)
{
	if (val > max)
	{
		return max;
	}
	if (val < min)
	{
		return min;
	}
	return val;
}

static inline int32_t random_int()
{
	// Returns a random real in [0,1).
	return (int32_t)rand() / ((int32_t)RAND_MAX + 1);
}

static inline int32_t random_int_range(int32_t min, int32_t max)
{
	// Returns a random real in [min,max).
	return min + (max - min) * random_int();
}

#endif // __MATH_UTILS_H__