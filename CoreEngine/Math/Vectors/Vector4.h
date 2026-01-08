#ifndef __VECTOR_4D__
#define __VECTOR_4D__

#define VECTOR4_EPS 1e-6f	// geometry / vectors

#include <math.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1 (for dot product)

/**
 * SVector4Df: A 4D float vector struct.
 *
 * This struct represents a 4D vector with float components. It provides
 * basic vector operations such as addition, subtraction, scalar multiplication,
 * dot product, length calculation, and normalization.
 *
 * Key features:
 * - Efficient float-based operations
 * - Accurate length and normalization calculations
 * - Support for dot product
 * - Conversion to and from GLM vectors for interoperability
 * - Clear and concise implementation adhering to Betty coding standards
 */
typedef struct __declspec(align(16)) SVector4f
{
	union
	{
		struct
		{
			union // First Component: X / Red / S / Width
			{
				float x;      // 3D Space: Horizontal axis
				float r;      // Color: Red channel (0.0 - 1.0)
				float s;      // Texture: U/S coordinate (Horizontal)
				float width;  // Bounds: Extent along X axis
			};
			union // Second Component: Y / Green / T / Height
			{
				float y;      // 3D Space: Vertical axis
				float g;      // Color: Green channel (0.0 - 1.0)
				float t;      // Texture: V/T coordinate (Vertical)
				float height; // Bounds: Extent along Y axis
			};
			union // Third Component: Z / Blue / P / Depth
			{
				float z;      // 3D Space: Depth axis
				float b;      // Color: Blue channel (0.0 - 1.0)
				float p;      // Texture: P/R coordinate (Depth/Cube Maps)
				float depth;  // Bounds: Extent along Z axis
			};
			union // Fourth Component: W / Alpha / Q / Scalar
			{
				float w;      // Homogeneous: Transformation weight (1=Pos, 0=Dir)
				float a;      // Color: Alpha channel (Transparency)
				float q;      // Texture: Perspective divisor coordinate
				float scalar; // Math: General purpose scalar value
			};
		};
		__m128 reg; // The SIMD register representation
		float v4[4]; // Array access: Useful for loops or OpenGL (glUniform4fv)
	};
} Vector4;

// Use _mm_setr_ps (Set Reversed) so the order matches x, y, z, w 
// _mm_set_ps usually takes arguments in reverse order (w, z, y, x)

#define Vector4F(val) ((Vector4){ .reg = _mm_set1_ps(val) })
#define Vector4D(x, y, z, w) ((Vector4){ .reg = _mm_setr_ps(x, y, z, w) })

// Initialization (The "In-place" pattern)
static inline Vector4 Vector4_Fill(float fVal)
{
	Vector4 result;
	// perform init as fast as possible on CPU
	result.reg = _mm_set1_ps(fVal);
	return (result);
}

static inline void Vector4_FillP(Vector4* __restrict vec4, float fVal)
{
	// perform init as fast as possible on CPU
	vec4->reg = _mm_set1_ps(fVal);
}

static inline Vector4 Vector4_Create(float x, float y, float z, float w)
{
	Vector4 result;
	// perform init as fast as possible on CPU
	result.reg = _mm_setr_ps(x, y, z, w);
	return (result);
}

static inline void Vector4_Init(Vector4* __restrict vec4, float x, float y, float z, float w)
{
	// perform init as fast as possible on CPU
	vec4->reg = _mm_setr_ps(x, y, z, w);
}

// Addition
static inline Vector4 Vector4_Add(const Vector4 vec1, const Vector4 vec2)
{
	Vector4 result;
	// Loads vec1 and vec2 into registers, adds them, and stores the result
	result.reg = _mm_add_ps(vec1.reg, vec2.reg);
	return result;
}

// S stand for Scalar
static inline Vector4 Vector4_Adds(const Vector4 vec1, float scalar)
{
	Vector4 result;
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed addition
	result.reg = _mm_add_ps(vec1.reg, scalarVec);
	return (result);
}

static inline void Vector4_Addv(Vector4* __restrict vec, const Vector4* __restrict vec1, const Vector4* __restrict vec2)
{
	// perform addition as fast as possible on CPU
	vec->reg = _mm_add_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Addvs(Vector4* __restrict vec, const Vector4* __restrict vec1, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed addition
	vec->reg = _mm_add_ps(vec1->reg, scalarVec);
}

static inline void Vector4_Addv_Safe(Vector4* vec, const Vector4* vec1, const Vector4* vec2)
{
	// perform addition as fast as possible on CPU
	vec->reg = _mm_add_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Adds_Assign(Vector4* vec, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed addition
	vec->reg = _mm_add_ps(vec->reg, scalarVec);
}

// Subtraction
static inline Vector4 Vector4_Sub(const Vector4 vec1, const Vector4 vec2)
{
	Vector4 result;
	// Loads vec1 and vec2 into registers, subs them, and stores the result
	result.reg = _mm_sub_ps(vec1.reg, vec2.reg);
	return result;
}

// S stand for Scalar
static inline Vector4 Vector4_Subs(const Vector4 vec1, float scalar)
{
	Vector4 result;
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed subtraction
	result.reg = _mm_sub_ps(vec1.reg, scalarVec);
	return (result);
}

static inline void Vector4_Subv(Vector4* __restrict vec, const Vector4* __restrict vec1, const Vector4* __restrict vec2)
{
	// perform subtraction as fast as possible on CPU
	vec->reg = _mm_sub_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Subvs(Vector4* __restrict vec, const Vector4* __restrict vec1, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed subtraction
	vec->reg = _mm_sub_ps(vec1->reg, scalarVec);
}

static inline void Vector4_Subv_Safe(Vector4* vec, const Vector4* vec1, const Vector4* vec2)
{
	// perform subtraction as fast as possible on CPU
	vec->reg = _mm_sub_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Subs_Assign(Vector4* vec, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed subtraction
	vec->reg = _mm_sub_ps(vec->reg, scalarVec);
}

// Multiplication
static inline Vector4 Vector4_Mul(const Vector4 vec1, const Vector4 vec2)
{
	Vector4 result;
	// Loads vec1 and vec2 into registers, multiply them, and stores the result
	result.reg = _mm_mul_ps(vec1.reg, vec2.reg);
	return result;
}

// S stand for Scalar
static inline Vector4 Vector4_Muls(const Vector4 vec1, float scalar)
{
	Vector4 result;
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// Perform packed multiplication
	result.reg = _mm_mul_ps(vec1.reg, scalarVec);
	return (result);
}

static inline void Vector4_Mulv(Vector4* __restrict vec, const Vector4* __restrict vec1, const Vector4* __restrict vec2)
{
	// perform multiplication as fast as possible on CPU
	vec->reg = _mm_mul_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Mulvs(Vector4* __restrict vec, const Vector4* __restrict vec1, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// perform multiplication as fast as possible on CPU
	vec->reg = _mm_mul_ps(vec1->reg, scalarVec);
}

static inline void Vector4_Mulv_Safe(Vector4* vec, const Vector4* vec1, const Vector4* vec2)
{
	// perform multiplication as fast as possible on CPU
	vec->reg = _mm_mul_ps(vec1->reg, vec2->reg);
}

static inline void Vector4_Muls_Assign(Vector4* vec, float scalar)
{
	// Broadcast scalar to all 4 slots: [scalar, scalar, scalar, scalar]
	__m128 scalarVec = _mm_set1_ps(scalar);
	// perform multiplication as fast as possible on CPU
	vec->reg = _mm_mul_ps(vec->reg, scalarVec);
}

// Division
static inline Vector4 Vector4_Div(const Vector4 vec1, const Vector4 vec2)
{
	Vector4 result;
	// 1. Create a mask: which components are greater than EPSILON?
	// _mm_cmpgt_ps returns "all 1s" (True) or "all 0s" (False) for each slot
	__m128 epsVec = _mm_set1_ps(VECTOR4_EPS);
	__m128 absVec = _mm_andnot_ps(_mm_set1_ps(-0.0f), vec2.reg); // Fast Absolute Value
	__m128 maskVec = _mm_cmpgt_ps(absVec, epsVec);

	// 2. Perform the division (this might result in Infinity for zero slots, or just zero as in my case)
	__m128 divisionVec = _mm_div_ps(vec1.reg, vec2.reg);

	// 3. Use the mask to pick: If mask is True, take divResult. If False, take 0.0.
	result.reg = _mm_and_ps(maskVec, divisionVec);
	return (result);
}

// S stand for Scalar
static inline Vector4 Vector4_Divs(const Vector4 vec1, float scalar)
{
	// Pro Tip: Multiply by the reciprocal.
	// The CPU does 1 division and 2 multiplications instead of 2 divisions.
	float inv = 1.0f / scalar;
	return (Vector4) { vec1.x * inv, vec1.y * inv, vec1.z * inv, vec1.w * inv};
}

static inline void Vector4_Divv(Vector4* __restrict vec, const Vector4* __restrict vec1, const Vector4* __restrict vec2)
{
	vec->x = vec1->x / vec2->x;
	vec->y = vec1->y / vec2->y;
	vec->z = vec1->z / vec2->z;
	vec->w = vec1->w / vec2->w;
}

static inline void Vector4_Divvs(Vector4* __restrict vec, const Vector4* __restrict vec1, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x = vec1->x * inv;
	vec->y = vec1->y * inv;
	vec->z = vec1->z * inv;
	vec->w = vec1->w * inv;
}

static inline void Vector4_Divv_Safe(Vector4* vec, const Vector4* vec1, const Vector4* vec2)
{
	if (fabsf(vec2->x) > VECTOR4_EPS)
	{
		vec->x = vec1->x / vec2->x;
	}
	else
	{
		vec->x = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->y) > VECTOR4_EPS)
	{
		vec->y = vec1->y / vec2->y;
	}
	else
	{
		vec->y = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->z) > VECTOR4_EPS)
	{
		vec->z = vec1->z / vec2->z;
	}
	else
	{
		vec->z = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->w) > VECTOR4_EPS)
	{
		vec->w = vec1->w / vec2->w;
	}
	else
	{
		vec->w = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

}

// SAFE: Includes the check. Use this for UI or User Input logic.
static inline void Vector4_Divvs_Safe(Vector4* __restrict vec, const Vector4* __restrict vec1, float s)
{
	if (fabsf(s) > VECTOR4_EPS)
	{
		float inv = 1.0f / s;
		vec->x = vec1->x * inv;
		vec->y = vec1->y * inv;
		vec->z = vec1->z * inv;
		vec->w = vec1->w * inv;
	}
	else
	{
		*vec = (Vector4){ 0.0f, 0.0f, 0.0f, 0.0f };
	}
}

static inline void Vector4_Divs_Assign(Vector4* vec, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x *= inv;
	vec->y *= inv;
	vec->z *= inv;
	vec->w *= inv;
}

static inline float Vector4_LengthSQv(const Vector4* const vec)
{
	//return (vec->x * vec->x + vec->y * vec->y + vec->z * vec->z + vec->w * vec->w);
	// Multiply x*x, y*y, z*z, w*w and sum them into all slots
	__m128 dot = _mm_dp_ps(vec->reg, vec->reg, 0xFF); // "Multi-Media Dot Product Packed Single-precision."

	// Return the first float
	return _mm_cvtss_f32(dot);
}

static inline float Vector4_Lengthv(const Vector4* const vec)
{
	// 0xFF: Multiply x*x, y*y, z*z, w*w and sum them into all slots
	// 0xF1: Multiply all 4 (F), but only store the sum in the 1st slot (1)
	__m128 dot = _mm_dp_ps(vec->reg, vec->reg, 0xFF);

	// _mm_sqrt_ss calculates the square root of only the first (scalar) slot
	__m128 len = _mm_sqrt_ss(dot);

	// "Convert Scalar Single-precision floating-point to 32-bit Float".
	return _mm_cvtss_f32(len);

	// hypotf also works, safer but slower
	// return sqrtf(Vector4_LengthSQv(vec));
}

static inline float Vector4_LengthSQ(const Vector4 vec)
{
	// return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w);
	// 0xFF: Multiply x*x, y*y, z*z, w*w and sum them into all slots
	// 0xF1: Multiply all 4 (F), but only store the sum in the 1st slot (1)
	__m128 dot = _mm_dp_ps(vec.reg, vec.reg, 0xFF);

	// "Convert Scalar Single-precision floating-point to 32-bit Float".
	return _mm_cvtss_f32(dot);
}

static inline float Vector4_Length(const Vector4 vec)
{
	// hypotf also works, safer but slower
	// return sqrtf(Vector4_LengthSQ(vec));
	// 0xFF: Multiply x*x, y*y, z*z, w*w and sum them into all slots
	// 0xF1: Multiply all 4 (F), but only store the sum in the 1st slot (1)
	__m128 dot = _mm_dp_ps(vec.reg, vec.reg, 0xFF);

	// _mm_sqrt_ss calculates the square root of only the first (scalar) slot
	__m128 len = _mm_sqrt_ss(dot);

	// "Convert Scalar Single-precision floating-point to 32-bit Float".
	return _mm_cvtss_f32(len);
}

static inline void Vector4_Normalize(Vector4* vec)
{
	// 1. Calculate the squared length (dot product)
	// 0xFF mask: multiply all 4, sum them, and broadcast to all slots
	__m128 dot = _mm_dp_ps(vec->reg, vec->reg, 0xFF);

	// 2. Calculate 1 / sqrt(dot)
	// _mm_rsqrt_ps is much faster than _mm_sqrt_ps + _mm_div_ps
	__m128 invLen = _mm_rsqrt_ps(dot);

	// 3. Multiply original vector by the reciprocal length
	vec->reg =  _mm_mul_ps(vec->reg, invLen);
}

static inline void Vector4_NormalizePrecise(Vector4* vec)
{
	// 1. Calculate the squared length (dot product)
	// 0xFF mask: multiply all 4, sum them, and broadcast to all slots
	__m128 dot = _mm_dp_ps(vec->reg, vec->reg, 0xFF);

	// 2. Calculate 1 / sqrt(dot)
	// _mm_sqrt_ps + _mm_div_psis much slower than _mm_rsqrt_ps but it's more precise
	__m128 len = _mm_sqrt_ps(dot);

	// 3. divide original vector by the reciprocal length
	vec->reg = _mm_div_ps(vec->reg, len);    // Actual division (slow)
}

static inline Vector4 Vector4_Normalized(const Vector4 vec)
{
	Vector4 result;

	// 1. Calculate the squared length (dot product)
	// 0xFF mask: multiply all 4, sum them, and broadcast to all slots
	__m128 dot = _mm_dp_ps(vec.reg, vec.reg, 0xFF);

	// 2. Calculate 1 / sqrt(dot)
	// _mm_rsqrt_ps is much faster than _mm_sqrt_ps + _mm_div_ps
	__m128 invLen = _mm_rsqrt_ps(dot);

	// 3. Multiply original vector by the reciprocal length
	result.reg = _mm_mul_ps(vec.reg, invLen);

	// 4. Return the normalized vector
	return (result);
}

static inline float Vector4_Dot(const Vector4 vec1, const Vector4 vec2)
{
	// return (vec1.x * vec2.x) + (vec1.y * vec2.y) + (vec1.z * vec2.z) + (vec1.w * vec2.w);
	// 0xFF: multiply all slots, sum all results, store in all slots of result
	__m128 res = _mm_dp_ps(vec1.reg, vec2.reg, 0xFF);
	return _mm_cvtss_f32(res); // Pull the float out of the register
}

static inline float Vector4_Distance(const Vector4 vec1, const Vector4 vec2)
{
	// 1. Get the difference vector (a - b)
	__m128 diff = _mm_sub_ps(vec1.reg, vec2.reg);

	// 2. Square the differences and sum them (Dot Product)
	// Mask 0xF1: Multiply all 4 components, sum them, store in 1st slot.
	__m128 distSq = _mm_dp_ps(diff, diff, 0xF1);

	// 3. Take the square root of the first slot
	__m128 dist = _mm_sqrt_ss(distSq);

	// 4. Return as float
	float fDistance = _mm_cvtss_f32(dist);

	return (fDistance);
}

static inline float Vector4_DistanceSQ(const Vector4 vec1, const Vector4 vec2)
{
	// 1. Get the difference vector (a - b)
	__m128 diff = _mm_sub_ps(vec1.reg, vec2.reg);

	// 2. Square the differences and sum them (Dot Product)
	// Mask 0xF1: Multiply all 4 components, sum them, store in 1st slot.
	__m128 distSq = _mm_dp_ps(diff, diff, 0xF1);

	// 3. Return as float
	float fDistance = _mm_cvtss_f32(distSq);

	return (fDistance);
}

static inline Vector4 Vector4_Lerp(const Vector4 a, const Vector4 b, float t)
{
	return (Vector4)
	{
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t,
		a.w + (b.w - a.w) * t
	};
}

//////////////// MATH PARTS ////////////////
#endif // __VECTOR_4D__