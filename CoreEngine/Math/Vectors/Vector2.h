#ifndef __VECTOR_2D__
#define __VECTOR_2D__

#define VECTOR2_EPS 1e-6f	// geometry / vectors

#include <math.h>

/**
 * SVector2Df: A 2D vector struct.
 *
 * This struct represents a 2D vector with components. It provides
 * basic vector operations such as addition, subtraction, scalar multiplication,
 * dot product, length calculation, and normalization.
 *
 * Key features:
 * - Efficient type-based operations
 * - Accurate length and normalization calculations
 * - Support for dot product
 * - Conversion to and from GLM vectors for interoperability
 * - Clear and concise implementation adhering to Betty coding standards
 */
typedef struct SVector2f
{
	union
	{
		struct
		{
			union // First Component (Horizontal / Red)
			{
				float x;      // Spatial
				float r;      // Color (Red)
				float u;      // Texture (Horizontal)
				float s;      // Texture (Alternative)
				float width;  // Dimension
			};
			union // Second Component (Vertical / Green)
			{
				float y;      // Spatial
				float g;      // Color (Green)
				float v;      // Texture (Vertical)
				float t;      // Texture (Alternative)
				float height; // Dimension
			};
		};
		float v2[2]; // Array access: Useful for loops or OpenGL (glUniform2fv)
	};
} Vector2;

#define Vector2F(val) ((Vector2){ (val), (val) })
#define Vector2D(x, y) ((Vector2){ (x), (y) })

// Initialization (The "In-place" pattern)
static inline Vector2 Vector2_Fill(float fVal)
{
	return (Vector2) { .x = fVal, .y = fVal };
}

static inline void Vector2_FillP(Vector2* __restrict vec2, float fVal)
{
	vec2->x = fVal;
	vec2->y = fVal;
}

static inline Vector2 Vector2_Create(float x, float y)
{
	return (Vector2) { .x = x, .y = y };
}

static inline void Vector2_Init(Vector2* __restrict vec2, float x, float y)
{
	vec2->x = x;
	vec2->y = y;
}

// Addition
static inline Vector2 Vector2_Add(const Vector2 vec1, const Vector2 vec2)
{
	return (Vector2) { vec1.x + vec2.x, vec1.y + vec2.y };
}

// S stand for Scalar
static inline Vector2 Vector2_Adds(const Vector2 vec1, float scalar)
{
	return (Vector2) { vec1.x + scalar, vec1.y + scalar};
}

static inline void Vector2_Addv(Vector2* __restrict vec, const Vector2* __restrict vec1, const Vector2* __restrict vec2)
{
	vec->x = vec1->x + vec2->x;
	vec->y = vec1->y + vec2->y;
}

static inline void Vector2_Addvs(Vector2* __restrict vec, const Vector2* __restrict vec1, float scalar)
{
	vec->x = vec1->x + scalar;
	vec->y = vec1->y + scalar;
}

static inline void Vector2_Addv_Safe(Vector2* vec, const Vector2* vec1, const Vector2* vec2)
{
	vec->x = vec1->x + vec2->x;
	vec->y = vec1->y + vec2->y;
}

static inline void Vector2_Adds_Assign(Vector2* vec, float scalar)
{
	vec->x += scalar;
	vec->y += scalar;
}

// Subtraction
static inline Vector2 Vector2_Sub(const Vector2 vec1, const Vector2 vec2)
{
	return (Vector2) { vec1.x - vec2.x, vec1.y - vec2.y };
}

// S stand for Scalar
static inline Vector2 Vector2_Subs(const Vector2 vec1, float scalar)
{
	return (Vector2) { vec1.x - scalar, vec1.y - scalar };
}

static inline void Vector2_Subv(Vector2* __restrict vec, const Vector2* __restrict vec1, const Vector2* __restrict vec2)
{
	vec->x = vec1->x - vec2->x;
	vec->y = vec1->y - vec2->y;
}

static inline void Vector2_Subvs(Vector2* __restrict vec, const Vector2* __restrict vec1, float scalar)
{
	vec->x = vec1->x - scalar;
	vec->y = vec1->y - scalar;
}

static inline void Vector2_Subv_Safe(Vector2* vec, const Vector2* vec1, const Vector2* vec2)
{
	vec->x = vec1->x - vec2->x;
	vec->y = vec1->y - vec2->y;
}

static inline void Vector2_Subs_Assign(Vector2* vec, float scalar)
{
	vec->x -= scalar;
	vec->y -= scalar;
}

// Multiplication
static inline Vector2 Vector2_Mul(const Vector2 vec1, const Vector2 vec2)
{
	return (Vector2) { vec1.x * vec2.x, vec1.y * vec2.y };
}

// S stand for Scalar
static inline Vector2 Vector2_Muls(const Vector2 vec1, float scalar)
{
	return (Vector2) { vec1.x * scalar, vec1.y * scalar };
}

static inline void Vector2_Mulv(Vector2* __restrict vec, const Vector2* __restrict vec1, const Vector2* __restrict vec2)
{
	vec->x = vec1->x * vec2->x;
	vec->y = vec1->y * vec2->y;
}

static inline void Vector2_Mulvs(Vector2* __restrict vec, const Vector2* __restrict vec1, float scalar)
{
	vec->x = vec1->x * scalar;
	vec->y = vec1->y * scalar;
}

static inline void Vector2_Mulv_Safe(Vector2* vec, const Vector2* vec1, const Vector2* vec2)
{
	vec->x = vec1->x * vec2->x;
	vec->y = vec1->y * vec2->y;
}

static inline void Vector2_Muls_Assign(Vector2* vec, float scalar)
{
	vec->x *= scalar;
	vec->y *= scalar;
}

// Division
static inline Vector2 Vector2_Div(const Vector2 vec1, const Vector2 vec2)
{
	Vector2 result = { 0.0f , 0.0f };
	if (fabsf(vec2.x) > VECTOR2_EPS)
	{
		result.x = vec1.x / vec2.x;
	}
	else
	{
		result.x = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2.y) > VECTOR2_EPS)
	{
		result.x = vec1.y / vec2.y;

	}
	else
	{
		result.y = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	return (result);
}

// S stand for Scalar
static inline Vector2 Vector2_Divs(const Vector2 vec1, float scalar)
{
	// Pro Tip: Multiply by the reciprocal.
	// The CPU does 1 division and 2 multiplications instead of 2 divisions.
	float inv = 1.0f / scalar;
	return (Vector2) { vec1.x * inv, vec1.y * inv};
}

static inline void Vector2_Divv(Vector2* __restrict vec, const Vector2* __restrict vec1, const Vector2* __restrict vec2)
{
	vec->x = vec1->x / vec2->x;
	vec->y = vec1->y / vec2->y;
}

static inline void Vector2_Divvs(Vector2* __restrict vec, const Vector2* __restrict vec1, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x = vec1->x * inv;
	vec->y = vec1->y * inv;
}

static inline void Vector2_Divv_Safe(Vector2* vec, const Vector2* vec1, const Vector2* vec2)
{
	if (fabsf(vec2->x) > VECTOR2_EPS)
	{
		vec->x = vec1->x / vec2->x;
	}
	else
	{
		vec->x = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->y) > VECTOR2_EPS)
	{
		vec->y = vec1->y / vec2->y;
	}
	else
	{
		vec->y = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}
}

// SAFE: Includes the check. Use this for UI or User Input logic.
static inline void Vector2_Divvs_Safe(Vector2* __restrict vec, const Vector2* __restrict vec1, float s)
{
	if (fabsf(s) > VECTOR2_EPS)
	{
		float inv = 1.0f / s;
		vec->x = vec1->x * inv;
		vec->y = vec1->y * inv;
	}
	else
	{
		*vec = (Vector2){ 0, 0 };
	}
}

static inline void Vector2_Divs_Assign(Vector2* vec, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x *= inv;
	vec->y *= inv;
}

static inline float Vector2_LengthSQv(const Vector2* vec)
{
	return (vec->x * vec->x + vec->y * vec->y);
}

static inline float Vector2_Lengthv(const Vector2* vec)
{
	// hypotf also works, safer but slower
	return sqrtf(Vector2_LengthSQv(vec));
}

static inline float Vector2_LengthSQ(const Vector2 vec)
{
	return (vec.x * vec.x + vec.y * vec.y);
}

static inline float Vector2_Length(const Vector2 vec)
{
	// hypotf also works, safer but slower
	return sqrtf(Vector2_LengthSQ(vec));
}

static inline void Vector2_Normalize(Vector2* vec)
{
	float length = Vector2_Lengthv(vec);
	if (length > VECTOR2_EPS) // Length is always positive, so fabsf isn't strictly needed
	{
		float invLength = 1.0f / length;
		vec->x *= invLength;
		vec->y *= invLength;
	}
}

static inline Vector2 Vector2_Normalized(const Vector2 vec)
{
	float length = Vector2_Length(vec);
	if (length > VECTOR2_EPS)
	{
		float invLength = 1.0f / length;
		return (Vector2) { vec.x* invLength, vec.y* invLength };
	}
	return (Vector2) { 0, 0 };
}

static inline float Vector2_Dot(const Vector2 vec1, const Vector2 vec2)
{
	return (vec1.x * vec2.x) + (vec1.y * vec2.y);
}

//////////////// MATH PARTS ////////////////
#endif // __VECTOR_2D__