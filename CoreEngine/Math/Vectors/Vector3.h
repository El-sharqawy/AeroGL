#ifndef __VECTOR_3D__
#define __VECTOR_3D__

#define VECTOR3_EPS 1e-6f	// geometry / vectors

#include <math.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1 (for dot product)
#include "../MathUtils.h"

/**
 * SVector3Df: A 3D float vector struct.
 *
 * This struct represents a 3D vector with float components. It provides
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
typedef struct __declspec(align(16)) SVector3f
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
			union // Third Component (Depth / Blue)
			{
				float z;      // Spatial (Depth)
				float b;      // Color (Blue)
				float w;      // Texture (Depth/3D textures)
				float p;      // Texture (Alternative)
				float depth;  // Dimension
			};
			float pad; // Padding to align to 16 bytes
		};
		__m128 reg;
		float v3[3]; // Array access: Useful for loops or OpenGL (glUniform3fv)
	};
} Vector3;

#define Vector3F(val) ((Vector3){ .reg = _mm_setr_ps((float)val, (float)val, (float)val, 0.0f) })
#define Vector3D(x, y, z) ((Vector3){ .reg = _mm_setr_ps((float)x, (float)y, (float)z, 0.0f) })
#define Vector3Zero(val) ((Vector3){ .reg = _mm_setr_ps(0.0f, 0.0f, 0.0f, 0.0f) })
#define Vector3One(val) ((Vector3){ .reg = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f) })

// Initialization (The "In-place" pattern)
static inline Vector3 Vector3_Fill(float fVal)
{
	return (Vector3) { fVal, fVal, fVal };
}

static inline void Vector3_FillP(Vector3* __restrict vec3, float fVal)
{
	vec3->x = fVal;
	vec3->y = fVal;
	vec3->z = fVal;
}

static inline Vector3 Vector3_Create(float x, float y, float z)
{
	return (Vector3) { x, y, z };
}

static inline void Vector3_Init(Vector3* __restrict vec3, float x, float y, float z)
{
	vec3->x = x;
	vec3->y = y;
	vec3->z = z;
}

// Addition
static inline Vector3 Vector3_Add(const Vector3 vec1, const Vector3 vec2)
{
	// return (Vector3D) { vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z };

	Vector3 result;
	// Adds X, Y, Z, and the Padding lane all at once
	result.reg = _mm_add_ps(vec1.reg, vec2.reg);
	return result;

}

// S stand for Scalar
static inline Vector3 Vector3_Adds(const Vector3 vec1, float scalar)
{
	// return (Vector3D) { vec1.x + scalar, vec1.y + scalar, vec1.z + scalar };

	Vector3 result;
	// Set the scalar into x, y, z but keep the 4th slot 0
	__m128 scalarVec = _mm_setr_ps(scalar, scalar, scalar, 0.0f);
	result.reg = _mm_add_ps(vec1.reg, scalarVec); // Add all lanes
	return result;
}

static inline void Vector3_Addv(Vector3* __restrict vec, const Vector3* __restrict vec1, const Vector3* __restrict vec2)
{
	vec->x = vec1->x + vec2->x;
	vec->y = vec1->y + vec2->y;
	vec->z = vec1->z + vec2->z;
}

static inline void Vector3_Addvs(Vector3* __restrict vec, const Vector3* __restrict vec1, float scalar)
{
	vec->x = vec1->x + scalar;
	vec->y = vec1->y + scalar;
	vec->z = vec1->z + scalar;
}

static inline void Vector3_Addv_Safe(Vector3* vec, const Vector3* vec1, const Vector3* vec2)
{
	vec->x = vec1->x + vec2->x;
	vec->y = vec1->y + vec2->y;
	vec->z = vec1->z + vec2->z;
}

static inline void Vector3_Adds_Assign(Vector3* vec, float scalar)
{
	vec->x += scalar;
	vec->y += scalar;
	vec->z += scalar;
}

static inline void Vector3_Addv_Assign(Vector3* vec, const Vector3 vec1)
{
	vec->x += vec1.x;
	vec->y += vec1.y;
	vec->z += vec1.z;
}

// Subtraction
static inline Vector3 Vector3_Sub(const Vector3 vec1, const Vector3 vec2)
{
	return (Vector3) { vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z };
}

// S stand for Scalar
static inline Vector3 Vector3_Subs(const Vector3 vec1, float scalar)
{
	return (Vector3) { vec1.x - scalar, vec1.y - scalar, vec1.z - scalar };
}

static inline void Vector3_Subv(Vector3* __restrict vec, const Vector3* __restrict vec1, const Vector3* __restrict vec2)
{
	vec->x = vec1->x - vec2->x;
	vec->y = vec1->y - vec2->y;
	vec->z = vec1->z - vec2->z;
}

static inline void Vector3_Subvs(Vector3* __restrict vec, const Vector3* __restrict vec1, float scalar)
{
	vec->x = vec1->x - scalar;
	vec->y = vec1->y - scalar;
	vec->z = vec1->z - scalar;
}

static inline void Vector3_Subv_Safe(Vector3* vec, const Vector3* vec1, const Vector3* vec2)
{
	vec->x = vec1->x - vec2->x;
	vec->y = vec1->y - vec2->y;
	vec->z = vec1->z - vec2->z;
}

static inline void Vector3_Subs_Assign(Vector3* vec, float scalar)
{
	vec->x -= scalar;
	vec->y -= scalar;
	vec->z -= scalar;
}

// Multiplication
static inline Vector3 Vector3_Mul(const Vector3 vec1, const Vector3 vec2)
{
	return (Vector3) { vec1.x * vec2.x, vec1.y * vec2.y, vec1.z * vec2.z };
}

// S stand for Scalar
static inline Vector3 Vector3_Muls(const Vector3 vec1, float scalar)
{
	return (Vector3) { vec1.x * scalar, vec1.y * scalar, vec1.z * scalar };
}

static inline void Vector3_Mulv(Vector3* __restrict vec, const Vector3* __restrict vec1, const Vector3* __restrict vec2)
{
	vec->x = vec1->x * vec2->x;
	vec->y = vec1->y * vec2->y;
	vec->z = vec1->z * vec2->z;
}

static inline void Vector3_Mulvs(Vector3* __restrict vec, const Vector3* __restrict vec1, float scalar)
{
	vec->x = vec1->x * scalar;
	vec->y = vec1->y * scalar;
	vec->z = vec1->z * scalar;
}

static inline void Vector3_Mulv_Safe(Vector3* vec, const Vector3* vec1, const Vector3* vec2)
{
	vec->x = vec1->x * vec2->x;
	vec->y = vec1->y * vec2->y;
	vec->z = vec1->z * vec2->z;
}

static inline void Vector3_Muls_Assign(Vector3* vec, float scalar)
{
	vec->x *= scalar;
	vec->y *= scalar;
	vec->z *= scalar;
}

// Division
static inline Vector3 Vector3_Div(const Vector3 vec1, const Vector3 vec2)
{
	// High-performance engines usually default to 0 on div-by-zero
	Vector3 result = { 0.0f , 0.0f, 0.0f };
	if (fabsf(vec2.x) > VECTOR3_EPS)
	{
		result.x = vec1.x / vec2.x;
	}

	if (fabsf(vec2.y) > VECTOR3_EPS)
	{
		result.y = vec1.y / vec2.y;

	}

	if (fabsf(vec2.z) > VECTOR3_EPS)
	{
		result.z = vec1.z / vec2.z;

	}

	return (result);
}

// S stand for Scalar
static inline Vector3 Vector3_Divs(const Vector3 vec1, float scalar)
{
	// Pro Tip: Multiply by the reciprocal.
	// The CPU does 1 division and 2 multiplications instead of 2 divisions.
	float inv = 1.0f / scalar;
	return (Vector3) { vec1.x * inv, vec1.y * inv, vec1.z * inv};
}

static inline void Vector3_Divv(Vector3* __restrict vec, const Vector3* __restrict vec1, const Vector3* __restrict vec2)
{
	vec->x = vec1->x / vec2->x;
	vec->y = vec1->y / vec2->y;
	vec->z = vec1->z / vec2->z;
}

static inline void Vector3_Divvs(Vector3* __restrict vec, const Vector3* __restrict vec1, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x = vec1->x * inv;
	vec->y = vec1->y * inv;
	vec->z = vec1->z * inv;
}

static inline void Vector3_Divv_Safe(Vector3* vec, const Vector3* vec1, const Vector3* vec2)
{
	if (fabsf(vec2->x) > VECTOR3_EPS)
	{
		vec->x = vec1->x / vec2->x;
	}
	else
	{
		vec->x = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->y) > VECTOR3_EPS)
	{
		vec->y = vec1->y / vec2->y;
	}
	else
	{
		vec->y = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

	if (fabsf(vec2->z) > VECTOR3_EPS)
	{
		vec->z = vec1->z / vec2->z;
	}
	else
	{
		vec->z = 0.0f; // High-performance engines usually default to 0 on div-by-zero
	}

}

// SAFE: Includes the check. Use this for UI or User Input logic.
static inline void Vector3_Divvs_Safe(Vector3* __restrict vec, const Vector3* __restrict vec1, float s)
{
	if (fabsf(s) > VECTOR3_EPS)
	{
		float inv = 1.0f / s;
		vec->x = vec1->x * inv;
		vec->y = vec1->y * inv;
		vec->z = vec1->z * inv;
	}
	else
	{
		*vec = (Vector3){ 0.0f, 0.0f, 0.0f };
	}
}

static inline void Vector3_Divs_Assign(Vector3* vec, float scalar)
{
	float inv = 1.0f / scalar;
	vec->x *= inv;
	vec->y *= inv;
	vec->z *= inv;
}

static inline float Vector3_LengthSQv(const Vector3* vec)
{
	return (vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}

static inline float Vector3_Lengthv(const Vector3* vec)
{
	// hypotf also works, safer but slower
	return sqrtf(Vector3_LengthSQv(vec));
}

static inline float Vector3_LengthSQ(const Vector3 vec)
{
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

static inline float Vector3_Length(const Vector3 vec)
{
	// hypotf also works, safer but slower
	return sqrtf(Vector3_LengthSQ(vec));
}

static inline void Vector3_Normalize(Vector3* vec)
{
	float length = Vector3_Lengthv(vec);
	if (length > VECTOR3_EPS) // Length is always positive, so fabsf isn't strictly needed
	{
		float invLength = 1.0f / length;
		vec->x *= invLength;
		vec->y *= invLength;
		vec->z *= invLength;
	}
}

static inline Vector3 Vector3_Normalized(const Vector3 vec)
{
	float length = Vector3_Length(vec);
	if (length > VECTOR3_EPS)
	{
		float invLength = 1.0f / length;
		return (Vector3) { vec.x * invLength, vec.y * invLength, vec.z * invLength };
	}
	return (Vector3) { 0.0f, 0.0f, 0.0f };
}

static inline float Vector3_Dot(const Vector3 vec1, const Vector3 vec2)
{
	// return (vec1.x * vec2.x) + (vec1.y * vec2.y) + (vec1.z * vec2.z);

	// 0x7F tells the hardware: "Ignore the 4th component of both vectors"
	__m128 res = _mm_dp_ps(vec1.reg, vec2.reg, 0x7F);
	return _mm_cvtss_f32(res);
}

static inline Vector3 Vector3_Cross(const Vector3 vec1, const Vector3 vec2)
{
	/*
	return (Vector3D)
	{
		(vec1.y * vec2.z) - (vec1.z * vec2.y),
		(vec1.z * vec2.x) - (vec1.x * vec2.z),
		(vec1.x * vec2.y) - (vec1.y * vec2.x)
	};*/

	// Shuffles: _MM_SHUFFLE(w, z, y, x)
	// We want: (a.y, a.z, a.x) and (b.z, b.x, b.y)

	__m128 vec1_yzx = _mm_shuffle_ps(vec1.reg, vec1.reg, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 vec2_zxy = _mm_shuffle_ps(vec2.reg, vec2.reg, _MM_SHUFFLE(3, 1, 0, 2));

	__m128 vec1_zxy = _mm_shuffle_ps(vec1.reg, vec1.reg, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 vec2_yzx = _mm_shuffle_ps(vec2.reg, vec2.reg, _MM_SHUFFLE(3, 0, 2, 1));

	Vector3 result;
	// (a.y*b.z - a.z*b.y), (a.z*b.x - a.x*b.z), (a.x*b.y - a.y*b.x)
	result.reg = _mm_sub_ps(
		_mm_mul_ps(vec1_yzx, vec2_zxy),
		_mm_mul_ps(vec1_zxy, vec2_yzx)
	);

	return (result);
}

static inline float Vector3_Distance(const Vector3 vec1, const Vector3 vec2)
{
	float fDeltaX = vec1.x - vec2.x;
	float fDeltaY = vec1.y - vec2.y;
	float fDeltaZ = vec1.z - vec2.z;

	float fDistance = sqrtf(fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ);
	return (fDistance);
}

static inline float Vector3_DistanceSQ(const Vector3 vec1, const Vector3 vec2)
{
	float fDeltaX = vec1.x - vec2.x;
	float fDeltaY = vec1.y - vec2.y;
	float fDeltaZ = vec1.z - vec2.z;

	return (fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ);
}

static inline Vector3 Vector3_Lerp(const Vector3 a, const Vector3 b, float t)
{
	return (Vector3)
	{
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

static inline Vector3 Vector3_Negate(const Vector3 vec3)
{
	return (Vector3) { -vec3.x, -vec3.y, -vec3.z };
}

static inline Vector3 Vector3_Random()
{
	return (Vector3) { random_float(), random_float(), random_float() };
}

static inline Vector3 Vector3_RandomRange(float min, float max)
{
	return (Vector3) { random_float_range(min, max), random_float_range(min, max), random_float_range(min, max) };
}

static inline Vector3 Vector3_RandomInsideUnitSphere()
{
	while (true)
	{
		Vector3 vec = Vector3_RandomRange(-1.0f, 1.0f);
		float lenSq = Vector3_LengthSQ(vec);

		if (Vector3_LengthSQ(vec) >= 1.0f)
			continue;

		return vec;
	}
}

static inline Vector3 Vector3_RandomNormalized()
{
	return Vector3_Normalized(Vector3_RandomInsideUnitSphere());
}

static inline Vector3 Vector3_RandomOnHemisphere(const Vector3 normal)
{
	Vector3 on_unit_sphere = Vector3_RandomNormalized();
	if (Vector3_Dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
	{
		return on_unit_sphere;
	}
	else
	{
		return Vector3_Negate(on_unit_sphere);
	}
}

static inline bool Vector3_NearZero(const Vector3 vec)
{
	// Return true if the vector is close to zero in all dimensions.
	float s = 1e-8f;
	return (fabs(vec.v3[0]) < VECTOR3_EPS) && (fabs(vec.v3[1]) < VECTOR3_EPS) && (fabs(vec.v3[2]) < VECTOR3_EPS);
}

static inline Vector3 Vector3_Reflect(const Vector3 vec, const Vector3 noramlVec)
{
	// v - 2 * dot(v, n) * n
	float dot = Vector3_Dot(vec, noramlVec);
	Vector3 scaled_n = Vector3_Muls(noramlVec, 2.0f * dot);
	return Vector3_Sub(vec, scaled_n);
}

static inline Vector3 Vector3_Refract(const Vector3 vec, const Vector3 normal, float etai_over_etat)
{
	float cos_theta = fminf(Vector3_Dot(Vector3_Negate(vec), normal), 1.0f);
	Vector3 r_out_perp = Vector3_Muls(Vector3_Add(vec, Vector3_Muls(normal, cos_theta)), etai_over_etat);
	Vector3 r_out_parallel = Vector3_Muls(normal, -sqrtf(fabsf(1.0f - Vector3_LengthSQ(r_out_perp))));
	return Vector3_Add(r_out_perp, r_out_parallel);
}

//////////////// MATH PARTS ////////////////
#endif // __VECTOR_3D__