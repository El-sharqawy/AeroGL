#ifndef __MATRIX3_H__
#define __MATRIX3_H__

#include <xmmintrin.h> // SSE

#include "../Vectors/Vector3.h"
#include "../MathUtils.h"

typedef Vector3 col3_type; // Column vector type
typedef Vector3 row3_type; // Column vector type

// Forward declarations - zero coupling
struct Matrix4;

/**
 * SMatrix3: A 3x3 float matrix struct.
 *
 * This struct represents a 3x3 matrix with float components. It is designed
 * for use in 3D graphics and transformations, providing efficient storage
 * and access patterns for matrix operations.
 *
 * Key features:
 * - 16-byte alignment for SIMD optimizations
 * - Column-major order storage for compatibility with graphics APIs
 * - Union-based access for both individual elements and SIMD registers
 */
typedef struct __declspec(align(16)) SMatrix3
{
	union
	{
		struct
		{
			// Memory Order: Col 0, then Col 1, then Col 2, then padding
			float m00, m01, m02, pad0;    // Column 0
			float m10, m11, m12, pad1;    // Column 1  
			float m20, m21, m22, pad2;    // Column 2
		};
		col3_type cols[3];      // Vector3 columns + auto-padding to 48 bytes
		float m[12];          // Raw access (last 4 bytes padding)
	};
} Matrix3;

// Scalar/Identity-style Initialization
#define Matrix3_InitF(val) \
    ((Matrix3){ .cols = { \
        { .reg = _mm_setr_ps(val,  0.0f, 0.0f, 0.0f) }, \
        { .reg = _mm_setr_ps(0.0f, val,  0.0f, 0.0f) }, \
        { .reg = _mm_setr_ps(0.0f, 0.0f, val,  0.0f) }, \
    }})


// Vector-based Initialization (Assuming col1 is a Vector4)
#define Matrix3_InitV(col1, col2, col3) \
    ((Matrix3){ .cols = { col1, col2, col3 } })

static const Matrix3 S_Matrix3_Identity = {
	.cols = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
	}
};

static const Matrix3 S_Matrix3_Zero = {
	.cols = {
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
	}
};

Matrix3 Matrix3_InitMatrix4(const Matrix4 Matrix1);

// Addition part
// Return by value (Clean syntax: a = b + c)
static inline Matrix3 Matrix3_Add(const Matrix3 Matrix1, const Matrix3 Matrix2)
{
	Matrix3 result;
	result.cols[0].reg = _mm_add_ps(Matrix1.cols[0].reg, Matrix2.cols[0].reg);
	result.cols[1].reg = _mm_add_ps(Matrix1.cols[1].reg, Matrix2.cols[1].reg);
	result.cols[2].reg = _mm_add_ps(Matrix1.cols[2].reg, Matrix2.cols[2].reg);
	return result;
}

// Scalar version (a = b + 5.0f)
static inline Matrix3 Matrix3_Adds(const Matrix3 Matrix1, float scalar)
{
	Matrix3 result;
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result.cols[0].reg = _mm_add_ps(Matrix1.cols[0].reg, scalarVec);
	result.cols[1].reg = _mm_add_ps(Matrix1.cols[1].reg, scalarVec);
	result.cols[2].reg = _mm_add_ps(Matrix1.cols[2].reg, scalarVec);
	return result;
}

// High-performance pointer version (m3 = m1 + m2)
// 'v' suffix for "void" or "vectorized", not sure yet
static inline void Matrix3_Add_Assign(Matrix3* __restrict result, Matrix3* __restrict Matrix1, Matrix3* __restrict Matrix2)
{
	// __restrict tells the compiler these memory areas do not overlap
	result->cols[0].reg = _mm_add_ps(Matrix1->cols[0].reg, Matrix2->cols[0].reg);
	result->cols[1].reg = _mm_add_ps(Matrix1->cols[1].reg, Matrix2->cols[1].reg);
	result->cols[2].reg = _mm_add_ps(Matrix1->cols[2].reg, Matrix2->cols[2].reg);
}

// In-place scalar version (mat += 5.0f)
static inline void Matrix3_Adds_Assign(Matrix3* result, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result->cols[0].reg = _mm_add_ps(result->cols[0].reg, scalarVec);
	result->cols[1].reg = _mm_add_ps(result->cols[1].reg, scalarVec);
	result->cols[2].reg = _mm_add_ps(result->cols[2].reg, scalarVec);
}

// In-place matrix version (mat += mat)
static inline void Matrix3_Addm_Assign(Matrix3* __restrict result, Matrix3* __restrict mat3)
{
	result->cols[0].reg = _mm_add_ps(result->cols[0].reg, mat3->cols[0].reg);
	result->cols[1].reg = _mm_add_ps(result->cols[1].reg, mat3->cols[1].reg);
	result->cols[2].reg = _mm_add_ps(result->cols[2].reg, mat3->cols[2].reg);
}

//////////////// MULTIPLICATION /////////////////
// Return by value (Clean syntax: a = b * c)
static inline Matrix3 Matrix3_Mul(const Matrix3 Matrix1, const Matrix3 Matrix2)
{
	Matrix3 result;
	for (int i = 0; i < 3; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(Matrix2.cols[i].x);
		__m128 mat2_y = _mm_set1_ps(Matrix2.cols[i].y);
		__m128 mat2_z = _mm_set1_ps(Matrix2.cols[i].z);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(Matrix1.cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1.cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1.cols[2].reg, mat2_z));

		result.cols[i].reg = res;
	}
	return result;
}

// Scalar version (a = b * 5.0f)
static inline Matrix3 Matrix3_Muls(const Matrix3 Matrix1, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);

	Matrix3 result;
	result.cols[0].reg = _mm_mul_ps(Matrix1.cols[0].reg, scalarVec);
	result.cols[1].reg = _mm_mul_ps(Matrix1.cols[1].reg, scalarVec);
	result.cols[2].reg = _mm_mul_ps(Matrix1.cols[2].reg, scalarVec);
	return (result);
}

// High-performance pointer version (m3 = m1 + m2)
// 'v' suffix for "void" or "vectorized", not sure yet
static inline void Matrix3_Mul_Assign(Matrix3* __restrict result, Matrix3* __restrict Matrix1, Matrix3* __restrict Matrix2)
{
	// __restrict tells the compiler these memory areas do not overlap

	// We use a temporary matrix so it's safe if result == m1 or result == m2
	Matrix3 tempMat;
	for (int i = 0; i < 3; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(Matrix2->cols[i].x);
		__m128 mat2_y = _mm_set1_ps(Matrix2->cols[i].y);
		__m128 mat2_z = _mm_set1_ps(Matrix2->cols[i].z);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(Matrix1->cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1->cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1->cols[2].reg, mat2_z));

		tempMat.cols[i].reg = res;

	}

	// Copy the final result back to the pointer
	*result = tempMat;
}

// In-place scalar version (mat *= 5.0f)
static inline void Matrix3_Muls_Assign(Matrix3* result, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result->cols[0].reg = _mm_mul_ps(result->cols[0].reg, scalarVec);
	result->cols[1].reg = _mm_mul_ps(result->cols[1].reg, scalarVec);
	result->cols[2].reg = _mm_mul_ps(result->cols[2].reg, scalarVec);
}

// In-place matrix version (mat *= mat)
static inline void Matrix3_Mulm_Assign(Matrix3* result, Matrix3* mat3)
{
	// No __restrict here because 'result' and 'mat3' might be the same matrix
	// We use a temporary matrix so it's safe if result == m1 or result == m2
	Matrix3 tempMat;
	for (int i = 0; i < 3; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(mat3->cols[i].x);
		__m128 mat2_y = _mm_set1_ps(mat3->cols[i].y);
		__m128 mat2_z = _mm_set1_ps(mat3->cols[i].z);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(result->cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(result->cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(result->cols[2].reg, mat2_z));

		tempMat.cols[i].reg = res;

	}

	// Copy the final result back to the pointer
	*result = tempMat;
}

// Return by value(Clean syntax : a = b * c)
static inline Vector3 Matrix3_Mul_Vec3(const Matrix3 mat, const Vector3 vec3)
{
	Vector3 result;

	// 1. Splat components of Matrix2's column i
	__m128 vec_x = _mm_set1_ps(vec3.x);
	__m128 vec_y = _mm_set1_ps(vec3.y);
	__m128 vec_z = _mm_set1_ps(vec3.z);

	// 2. Linear combination: (Col0 * x) + (Col1 * y) + (Col2 * z) + (Col3 * w)
	result.reg = _mm_mul_ps(mat.cols[0].reg, vec_x);
	result.reg = _mm_add_ps(result.reg, _mm_mul_ps(mat.cols[1].reg, vec_y));
	result.reg = _mm_add_ps(result.reg, _mm_mul_ps(mat.cols[2].reg, vec_z));

	return result;
}

static inline Matrix3 Matrix3_Identity(void)
{
	return S_Matrix3_Identity;
}

static inline Matrix3 Matrix3_Zero(void)
{
	return S_Matrix3_Zero;
}

// Transpose for normal matrix (2.5 cycles!)
static inline Matrix3 Matrix3_Transpose(const Matrix3 mat)
{
	__m128 c0 = mat.cols[0].reg;
	__m128 c1 = mat.cols[1].reg;
	__m128 c2 = mat.cols[2].reg;

	Matrix3 result;
	result.cols[0].reg = _mm_unpacklo_ps(c0, c1);  // row0
	result.cols[1].reg = _mm_unpackhi_ps(c0, c1);  // row1
	result.cols[2].reg = _mm_movelh_ps(c0, c2);    // row2
	return result;
}

static inline Matrix3 Matrix3_TransposeV(const Matrix3 mat)
{
	return (Matrix3) {
		.cols = {
			{ mat.cols[0].x, mat.cols[1].x, mat.cols[2].x, 0.0f },
			{ mat.cols[0].y, mat.cols[1].y, mat.cols[2].y, 0.0f },
			{ mat.cols[0].z, mat.cols[1].z, mat.cols[2].z, 0.0f }
		}
	};
}

/**
 * Transpose 3x3 matrix (scalar, return-by-value).
 * For normal matrices: normalMat = Matrix3_Transpose(Mat4ToMat3(model))
 */
static inline Matrix3 Matrix3_TransposeN(const Matrix3 mat)
{
	Matrix3 result;

	// col0 = row0 -> result.m00=result.m00, result.m10=result.m01, result.m20=result.m02
	result.m00 = mat.m00;
	result.m10 = mat.m01;
	result.m20 = mat.m02;

	// col1 = row1
	result.m01 = mat.m10;
	result.m11 = mat.m11;
	result.m21 = mat.m12;

	// col2 = row2  
	result.m02 = mat.m20;
	result.m12 = mat.m21;
	result.m22 = mat.m22;

	return result;
}

static inline float Matrix3_Determinant(const Matrix3 mat)
{
	// Extract elements for clarity
	const float a = mat.m00, b = mat.m01, c = mat.m02;
	const float d = mat.m10, e = mat.m11, f = mat.m12;
	const float g = mat.m20, h = mat.m21, i = mat.m22;

	// Compute determinant
	const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
	return (det);
}

/**
 * Computes inverse of 3x3 matrix (determinant != 0).
 * For normal matrices: normalMat = Matrix3_Inverse(Transpose(Mat4ToMat3(model)))
 * Returns S_Matrix3_Zero if singular (det != 0).
 */
static inline Matrix3 Matrix3_Inverse(const Matrix3 mat)
{
	// Extract elements for clarity
	const float a = mat.m00, b = mat.m01, c = mat.m02;
	const float d = mat.m10, e = mat.m11, f = mat.m12;
	const float g = mat.m20, h = mat.m21, i = mat.m22;

	// Compute determinant
	const float det = Matrix3_Determinant(mat);

	if (fabsf(det) < 1e-6f)
	{
		return S_Matrix3_Zero;  // Singular matrix
	}

	const float idet = 1.0f / det;

	Matrix3 result;
	// Classical adjugate / det
	result.m00 = (e * i - f * h) * idet;
	result.m10 = -(d * i - f * g) * idet;
	result.m20 = (d * h - e * g) * idet;

	result.m01 = -(b * i - c * h) * idet;
	result.m11 = (a * i - c * g) * idet;
	result.m21 = -(a * h - b * g) * idet;

	result.m02 = (b * f - c * e) * idet;
	result.m12 = -(a * f - c * d) * idet;
	result.m22 = (a * e - b * d) * idet;

	return result;
}

#endif // __MATRIX3_H__