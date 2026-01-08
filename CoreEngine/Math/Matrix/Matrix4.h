#ifndef __MATRIX4_H__
#define __MATRIX4_H__

#include <xmmintrin.h> // SSE

#include "../Vectors/Vector3.h"
#include "../Vectors/Vector4.h"
#include "../MathUtils.h"

typedef Vector4 col_type; // Column vector type
typedef Vector4 row_type; // Column vector type

/**
 * SMatrix4: A 4x4 float matrix struct.
 *
 * This struct represents a 4x4 matrix with float components. It is designed
 * for use in 3D graphics and transformations, providing efficient storage
 * and access patterns for matrix operations.
 *
 * Key features:
 * - 16-byte alignment for SIMD optimizations
 * - Column-major order storage for compatibility with graphics APIs
 * - Union-based access for both individual elements and SIMD registers
 */
typedef struct __declspec(align(16)) SMatrix4
{
	union
	{
		struct
		{
			// Memory Order: Col 0, then Col 1, then Col 2, then Col 3
			float m00, m01, m02, m03;	// Column 0 (X Axis + 0)
			float m10, m11, m12, m13;	// Column 1 (Y Axis + 0)
			float m20, m21, m22, m23;	// Column 2 (Z Axis + 0)
			float m30, m31, m32, m33;
		};
		Vector4 cols[4]; // Each column is a 4D Vector (SIMD Register)
		float m[16]; // Array access for convenience

	};
} Matrix4;

// Full Initialization
#define Matrix4_Init(m00, m01, m02, m03, \
                     m10, m11, m12, m13, \
                     m20, m21, m22, m23, \
                     m30, m31, m32, m33) \
    ((Matrix4){ .cols = { \
        { .reg = _mm_setr_ps(m00, m01, m02, m03) }, \
        { .reg = _mm_setr_ps(m10, m11, m12, m13) }, \
        { .reg = _mm_setr_ps(m20, m21, m22, m23) }, \
        { .reg = _mm_setr_ps(m30, m31, m32, m33) }  \
    }})

// Scalar/Identity-style Initialization
#define Matrix4_InitF(val) \
    ((Matrix4){ .cols = { \
        { .reg = _mm_setr_ps(val,  0.0f, 0.0f, 0.0f) }, \
        { .reg = _mm_setr_ps(0.0f, val,  0.0f, 0.0f) }, \
        { .reg = _mm_setr_ps(0.0f, 0.0f, val,  0.0f) }, \
        { .reg = _mm_setr_ps(0.0f, 0.0f, 0.0f, val ) }  \
    }})


// Vector-based Initialization (Assuming col1 is a Vector4)
#define Matrix4_InitV(col1, col2, col3, col4) \
    ((Matrix4){ .cols = { col1, col2, col3, col4 } })

static const Matrix4 S_Matrix4_Identity = {
	.cols = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	}
};

static const Matrix4 S_Matrix4_Zero = {
	.cols = {
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f}
	}
};

// Addition part
// Return by value (Clean syntax: a = b + c)
static inline Matrix4 Matrix4_Add(const Matrix4 Matrix1, const Matrix4 Matrix2)
{
	Matrix4 result;
	result.cols[0].reg = _mm_add_ps(Matrix1.cols[0].reg, Matrix2.cols[0].reg);
	result.cols[1].reg = _mm_add_ps(Matrix1.cols[1].reg, Matrix2.cols[1].reg);
	result.cols[2].reg = _mm_add_ps(Matrix1.cols[2].reg, Matrix2.cols[2].reg);
	result.cols[3].reg = _mm_add_ps(Matrix1.cols[3].reg, Matrix2.cols[3].reg);
	return result;
}

// Scalar version (a = b + 5.0f)
static inline Matrix4 Matrix4_Adds(const Matrix4 Matrix1, float scalar)
{
	Matrix4 result;
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result.cols[0].reg = _mm_add_ps(Matrix1.cols[0].reg, scalarVec);
	result.cols[1].reg = _mm_add_ps(Matrix1.cols[1].reg, scalarVec);
	result.cols[2].reg = _mm_add_ps(Matrix1.cols[2].reg, scalarVec);
	result.cols[3].reg = _mm_add_ps(Matrix1.cols[3].reg, scalarVec);
	return result;
}

// High-performance pointer version (m3 = m1 + m2)
// 'v' suffix for "void" or "vectorized", not sure yet
static inline void Matrix4_Add_Assign(Matrix4* __restrict result ,Matrix4* __restrict Matrix1, Matrix4* __restrict Matrix2)
{
	// __restrict tells the compiler these memory areas do not overlap
	result->cols[0].reg = _mm_add_ps(Matrix1->cols[0].reg, Matrix2->cols[0].reg);
	result->cols[1].reg = _mm_add_ps(Matrix1->cols[1].reg, Matrix2->cols[1].reg);
	result->cols[2].reg = _mm_add_ps(Matrix1->cols[2].reg, Matrix2->cols[2].reg);
	result->cols[3].reg = _mm_add_ps(Matrix1->cols[3].reg, Matrix2->cols[3].reg);
}

// In-place scalar version (mat += 5.0f)
static inline void Matrix4_Adds_Assign(Matrix4* result, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result->cols[0].reg = _mm_add_ps(result->cols[0].reg, scalarVec);
	result->cols[1].reg = _mm_add_ps(result->cols[1].reg, scalarVec);
	result->cols[2].reg = _mm_add_ps(result->cols[2].reg, scalarVec);
	result->cols[3].reg = _mm_add_ps(result->cols[3].reg, scalarVec);
}

// In-place matrix version (mat += mat)
static inline void Matrix4_Addm_Assign(Matrix4* __restrict result, Matrix4* __restrict mat4)
{
	result->cols[0].reg = _mm_add_ps(result->cols[0].reg, mat4->cols[0].reg);
	result->cols[1].reg = _mm_add_ps(result->cols[1].reg, mat4->cols[1].reg);
	result->cols[2].reg = _mm_add_ps(result->cols[2].reg, mat4->cols[2].reg);
	result->cols[3].reg = _mm_add_ps(result->cols[3].reg, mat4->cols[3].reg);
}

//////////////// MULTIPLICATION /////////////////
// Return by value (Clean syntax: a = b * c)
static inline Matrix4 Matrix4_Mul(const Matrix4 Matrix1, const Matrix4 Matrix2)
{
	Matrix4 result;
	for (int i = 0; i < 4; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(Matrix2.cols[i].x);
		__m128 mat2_y = _mm_set1_ps(Matrix2.cols[i].y);
		__m128 mat2_z = _mm_set1_ps(Matrix2.cols[i].z);
		__m128 mat2_w = _mm_set1_ps(Matrix2.cols[i].w);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(Matrix1.cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1.cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1.cols[2].reg, mat2_z));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1.cols[3].reg, mat2_w));

		result.cols[i].reg = res;
	}
	return result;
}

// Scalar version (a = b * 5.0f)
static inline Matrix4 Matrix4_Muls(const Matrix4 Matrix1, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);

	Matrix4 result;
	result.cols[0].reg = _mm_mul_ps(Matrix1.cols[0].reg, scalarVec);
	result.cols[1].reg = _mm_mul_ps(Matrix1.cols[1].reg, scalarVec);
	result.cols[2].reg = _mm_mul_ps(Matrix1.cols[2].reg, scalarVec);
	result.cols[3].reg = _mm_mul_ps(Matrix1.cols[3].reg, scalarVec);
	return (result);
}

// High-performance pointer version (m3 = m1 + m2)
// 'v' suffix for "void" or "vectorized", not sure yet
static inline void Matrix4_Mul_Assign(Matrix4* __restrict result, Matrix4* __restrict Matrix1, Matrix4* __restrict Matrix2)
{
	// __restrict tells the compiler these memory areas do not overlap

	// We use a temporary matrix so it's safe if result == m1 or result == m2
	Matrix4 tempMat;
	for (int i = 0; i < 4; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(Matrix2->cols[i].x);
		__m128 mat2_y = _mm_set1_ps(Matrix2->cols[i].y);
		__m128 mat2_z = _mm_set1_ps(Matrix2->cols[i].z);
		__m128 mat2_w = _mm_set1_ps(Matrix2->cols[i].w);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(Matrix1->cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1->cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1->cols[2].reg, mat2_z));
		res = _mm_add_ps(res, _mm_mul_ps(Matrix1->cols[3].reg, mat2_w));

		tempMat.cols[i].reg = res;

	}

	// Copy the final result back to the pointer
	*result = tempMat;
}

// In-place scalar version (mat *= 5.0f)
static inline void Matrix4_Muls_Assign(Matrix4* result, float scalar)
{
	// Broadcast scalar once to use for all columns
	__m128 scalarVec = _mm_set1_ps(scalar);
	result->cols[0].reg = _mm_mul_ps(result->cols[0].reg, scalarVec);
	result->cols[1].reg = _mm_mul_ps(result->cols[1].reg, scalarVec);
	result->cols[2].reg = _mm_mul_ps(result->cols[2].reg, scalarVec);
	result->cols[3].reg = _mm_mul_ps(result->cols[3].reg, scalarVec);
}

// In-place matrix version (mat *= mat)
static inline void Matrix4_Mulm_Assign(Matrix4*  result, Matrix4* mat4)
{
	// No __restrict here because 'result' and 'mat4' might be the same matrix
	// We use a temporary matrix so it's safe if result == m1 or result == m2
	Matrix4 tempMat;
	for (int i = 0; i < 4; i++)
	{
		// 1. Splat components of Matrix2's column i
		__m128 mat2_x = _mm_set1_ps(mat4->cols[i].x);
		__m128 mat2_y = _mm_set1_ps(mat4->cols[i].y);
		__m128 mat2_z = _mm_set1_ps(mat4->cols[i].z);
		__m128 mat2_w = _mm_set1_ps(mat4->cols[i].w);

		// 2. Multiply A's columns by the splatted values and sum them
		__m128 res = _mm_mul_ps(result->cols[0].reg, mat2_x);
		res = _mm_add_ps(res, _mm_mul_ps(result->cols[1].reg, mat2_y));
		res = _mm_add_ps(res, _mm_mul_ps(result->cols[2].reg, mat2_z));
		res = _mm_add_ps(res, _mm_mul_ps(result->cols[3].reg, mat2_w));

		tempMat.cols[i].reg = res;

	}

	// Copy the final result back to the pointer
	*result = tempMat;
}

// Return by value(Clean syntax : a = b * c)
static inline Vector4 Matrix4_Mul_Vec4(const Matrix4 mat, const Vector4 vec4)
{
	Vector4 result;

	// 1. Splat components of Matrix2's column i
	__m128 vec_x = _mm_set1_ps(vec4.x);
	__m128 vec_y = _mm_set1_ps(vec4.y);
	__m128 vec_z = _mm_set1_ps(vec4.z);
	__m128 vec_w = _mm_set1_ps(vec4.w);

	// 2. Linear combination: (Col0 * x) + (Col1 * y) + (Col2 * z) + (Col3 * w)
	result.reg = _mm_mul_ps(mat.cols[0].reg, vec_x);
	result.reg = _mm_add_ps(result.reg, _mm_mul_ps(mat.cols[1].reg, vec_y));
	result.reg = _mm_add_ps(result.reg, _mm_mul_ps(mat.cols[2].reg, vec_z));
	result.reg = _mm_add_ps(result.reg, _mm_mul_ps(mat.cols[3].reg, vec_w));

	return result;
}

static inline Matrix4 Matrix4_Identity(void)
{
	return S_Matrix4_Identity;
}

static inline Matrix4 Matrix4_Zero(void)
{
	return S_Matrix4_Zero;
}

static inline Matrix4 Matrix4_RotateX(const Matrix4 mat, float angleRadians)
{
	float sinRad = sinf(angleRadians);
	float cosRad = cosf(angleRadians);

	// Create a rotation matrix for just this axis
	Matrix4 rotationZ = Matrix4_Identity();

	// Column 2
	rotationZ.cols[1].y = cosRad;
	rotationZ.cols[1].z = sinRad;

	// Column 3
	rotationZ.cols[2].y = -sinRad;
	rotationZ.cols[2].z = cosRad;

	// Combine it with the existing matrix
	return Matrix4_Mul(mat, rotationZ);
}

static inline Matrix4 Matrix4_RotateY(const Matrix4 mat, float angleRadians)
{
	float sinRad = sinf(angleRadians);
	float cosRad = cosf(angleRadians);

	// Create a rotation matrix for just this axis
	Matrix4 rotationZ = Matrix4_Identity();

	// Column 1
	rotationZ.cols[0].x = cosRad;
	rotationZ.cols[0].z = -sinRad;

	// Column 3
	rotationZ.cols[2].x = sinRad;
	rotationZ.cols[2].z = cosRad;

	// Combine it with the existing matrix
	return Matrix4_Mul(mat, rotationZ);
}

static inline Matrix4 Matrix4_RotateZ(const Matrix4 mat, float angleRadians)
{
	float sinRad = sinf(angleRadians);
	float cosRad = cosf(angleRadians);

	// Create a rotation matrix for just this axis
	Matrix4 rotationZ = Matrix4_Identity();

	// Column 1
	rotationZ.cols[0].x = cosRad;
	rotationZ.cols[0].y = sinRad;

	// Column 2
	rotationZ.cols[1].x = -sinRad;
	rotationZ.cols[1].y = cosRad;

	// Combine it with the existing matrix
	return Matrix4_Mul(mat, rotationZ);
}
	
/**
 * @brief Initializes the matrix for a rotation transformation using the ZYX Euler rotation sequence.
 *
 * This function creates individual rotation matrices for the X, Y, and Z axes
 * and combines them in the order rz * ry * rx. This order applies the
 * rotations in the sequence: X-axis, Y-axis, then Z-axis, relative to the
 * local coordinate system.
 *
 * @param angleRadians: The angle of rotation around the All Axis in degrees.
 */
static inline Matrix4 Matrix4_RotateAll(const Matrix4 mat, float angleRadians)
{
	const float angle = ToRadians(angleRadians);

	Matrix4_RotateZ(mat, angleRadians);
	Matrix4_RotateY(mat, angleRadians);
	Matrix4_RotateX(mat, angleRadians);

	return (mat);
}

/**
 * @brief Initializes the matrix for a rotation transformation using the XYZ Euler rotation sequence.
 *
 * This function creates individual rotation matrices for the X, Y, and Z axes
 * and combines them in the order rx * ry * rz. This order applies the
 * rotations in the sequence: X-axis, Y-axis, then Z-axis, relative to the
 * local coordinate system.
 *
 * @param RotateX: The angle of rotation around the X-axis in degrees.
 * @param RotateY: The angle of rotation around the Y-axis in degrees.
 * @param RotateZ: The angle of rotation around the Z-axis in degrees.
 */
static inline Matrix4 Matrix4_RotateXYZ(const Matrix4 mat, float RotateX, float RotateY, float RotateZ)
{
	const float angleX = ToRadians(RotateX);
	const float angleY = ToRadians(RotateY);
	const float angleZ = ToRadians(RotateZ);

	// Chain the results!
	Matrix4 result = mat;
	result = Matrix4_RotateX(result, angleX);
	result = Matrix4_RotateY(result, angleY);
	result = Matrix4_RotateZ(result, angleZ);

	return result;
}

/**
 * @brief Initializes the matrix for a rotation transformation using the ZYX Euler rotation sequence.
 *
 * This function creates individual rotation matrices for the X, Y, and Z axes
 * and combines them in the order rz * ry * rx. This order applies the
 * rotations in the sequence: X-axis, Y-axis, then Z-axis, relative to the
 * local coordinate system.
 *
 * @param RotateX: The angle of rotation around the X-axis in degrees.
 * @param RotateY: The angle of rotation around the Y-axis in degrees.
 * @param RotateZ: The angle of rotation around the Z-axis in degrees.
 */
static inline Matrix4 Matrix4_RotateZYX(const Matrix4 mat, float RotateX, float RotateY, float RotateZ)
{
	const float angleX = ToRadians(RotateX);
	const float angleY = ToRadians(RotateY);
	const float angleZ = ToRadians(RotateZ);

	// Chain the results!
	Matrix4 result = mat;
	result = Matrix4_RotateZ(result, angleZ);
	result = Matrix4_RotateY(result, angleY);
	result = Matrix4_RotateX(result, angleX);

	return result;
}

/**
 * @brief Initializes the matrix for a rotation transformation using the XYZ Euler rotation sequence.
 *
 * This function creates individual rotation matrices for the X, Y, and Z axes
 * and combines them in the order rx * ry * rz. This order applies the
 * rotations in the sequence: X-axis, Y-axis, then Z-axis, relative to the
 * local coordinate system.
 *
 * @param v3Rotation: The angles of rotation around the All-Axis in degrees.
 */
static inline Matrix4 Matrix4_VRotateXYZ(const Matrix4 mat, const Vector3 v3Rotation)
{
	const float angleX = ToRadians(v3Rotation.x);
	const float angleY = ToRadians(v3Rotation.y);
	const float angleZ = ToRadians(v3Rotation.z);

	// Chain the results!
	Matrix4 result = mat;
	result = Matrix4_RotateX(result, angleX);
	result = Matrix4_RotateY(result, angleY);
	result = Matrix4_RotateZ(result, angleZ);

	return result;
}

/**
 * @brief Initializes the matrix for a rotation transformation using the ZYX Euler rotation sequence.
 *
 * This function creates individual rotation matrices for the X, Y, and Z axes
 * and combines them in the order rz * ry * rx. This order applies the
 * rotations in the sequence: X-axis, Y-axis, then Z-axis, relative to the
 * local coordinate system.
 *
 * @param v3Rotation: The angles of rotation around the All-Axis in degrees.
 */
static inline Matrix4 Matrix4_VRotateZYX(const Matrix4 mat, const Vector3 v3Rotation)
{
	const float angleX = ToRadians(v3Rotation.x);
	const float angleY = ToRadians(v3Rotation.y);
	const float angleZ = ToRadians(v3Rotation.z);

	// Chain the results!
	Matrix4 result = mat;
	result = Matrix4_RotateZ(result, angleZ);
	result = Matrix4_RotateY(result, angleY);
	result = Matrix4_RotateX(result, angleX);

	return result;
}

/**
 * @brief Applies a rotation of a given angle around an arbitrary axis to an existing 4x4 matrix.
 *
 * This function implements the rotation matrix derived from Rodrigues' formula for
 * the axis-angle representation. It calculates the rotation matrix and then
 * pre-multiplies the input matrix, effectively applying the rotation transformation
 * in world space (assuming standard column-major conventions).
 *
 * @param mat The existing 4x4 matrix (e.g., Model matrix) to be rotated.
 * @param fAngle The angle of rotation in degrees.
 * @param v3Axis The 3D vector defining the axis of rotation (will be normalized internally).
 * @return SMatrix4x4 The resulting 4x4 transformation matrix (mat * Rotation).
 */
static inline Matrix4 Matrix4_Rotate(const Matrix4 mat, float fAngle, const Vector3 v3Axis)
{
	const float rotationAngle = ToRadians(fAngle);
	const float sinAngle = sinf(rotationAngle);
	const float cosAngle = cosf(rotationAngle);
	const float fSub = 1.0f - cosAngle;

	const Vector3 vAxis = Vector3_Normalized(v3Axis);
	const Vector3 vMultiplied = Vector3_Muls(vAxis, fSub);

	// Start with Identity so cols[3] is {0,0,0,1}
	Matrix4 rotationMat = Matrix4_Identity();
	rotationMat.cols[0].x = cosAngle + vMultiplied.x * vAxis.x;
	rotationMat.cols[0].y = vMultiplied.x * vAxis.y + sinAngle * vAxis.z;
	rotationMat.cols[0].z = vMultiplied.x * vAxis.z - sinAngle * vAxis.y;

	rotationMat.cols[1].x = vMultiplied.y * vAxis.x - sinAngle * vAxis.z;
	rotationMat.cols[1].y = cosAngle + vMultiplied.y * vAxis.y;
	rotationMat.cols[1].z = vMultiplied.y * vAxis.z + sinAngle * vAxis.x;

	rotationMat.cols[2].x = vMultiplied.z * vAxis.x + sinAngle * vAxis.y;
	rotationMat.cols[2].y = vMultiplied.z * vAxis.y - sinAngle * vAxis.x;
	rotationMat.cols[2].z = cosAngle + vMultiplied.z * vAxis.z;


	// Use your existing SIMD multiply to combine them, keep translation and scale
	return Matrix4_Mul(mat, rotationMat);
}

static inline Matrix4 Matrix4_TranslateWorldF(const Matrix4 mat, float translateX, float translateY, float translateZ)
{
	Matrix4 result = mat;
	// In Column-Major, the 4th column (index 3) is the translation column
	// We multiply the first 3 columns by the 4th's W (usually 1) and add the translation
	// But for a simple translate, we just modify the 4th column directly:
	result.cols[3].x += translateX;
	result.cols[3].y += translateY;
	result.cols[3].z += translateZ;
	return result;
}

static inline Matrix4 Matrix4_TranslateWorld(const Matrix4 mat, Vector3 pos)
{
	Matrix4 result = mat;
	// In Column-Major, the 4th column (index 3) is the translation column
	// We multiply the first 3 columns by the 4th's W (usually 1) and add the translation
	// But for a simple translate, we just modify the 4th column directly:
	result.cols[3].x += pos.x;
	result.cols[3].y += pos.y;
	result.cols[3].z += pos.z;
	return result;
}

static inline Matrix4 Matrix4_TranslateF(const Matrix4 mat, float translateX, float translateY, float translateZ)
{
	Matrix4 result = mat;
	// In Column-Major, the 4th column (index 3) is the translation column
	// We multiply the first 3 columns by the 4th's W (usually 1) and add the translation
	// But for a simple translate, we just modify the 4th column directly:

	// Splat translation components
	__m128 tx = _mm_set1_ps(translateX);
	__m128 ty = _mm_set1_ps(translateY);
	__m128 tz = _mm_set1_ps(translateZ);

	// New Pos = (Col0 * x) + (Col1 * y) + (Col2 * z) + ExistingPos
	__m128 newPos = _mm_mul_ps(mat.cols[0].reg, tx);
	newPos = _mm_add_ps(newPos, _mm_mul_ps(mat.cols[1].reg, ty));
	newPos = _mm_add_ps(newPos, _mm_mul_ps(mat.cols[2].reg, tz));
	newPos = _mm_add_ps(newPos, mat.cols[3].reg);

	result.cols[3].reg = newPos;
	return result;
}

static inline Matrix4 Matrix4_Translate(const Matrix4 mat, Vector3 pos)
{
	Matrix4 result = mat;
	// In Column-Major, the 4th column (index 3) is the translation column
	// We multiply the first 3 columns by the 4th's W (usually 1) and add the translation
	// But for a simple translate, we just modify the 4th column directly:

	// Splat translation components
	__m128 tx = _mm_set1_ps(pos.x);
	__m128 ty = _mm_set1_ps(pos.y);
	__m128 tz = _mm_set1_ps(pos.z);

	// New Pos = (Col0 * x) + (Col1 * y) + (Col2 * z) + ExistingPos
	__m128 newPos = _mm_mul_ps(mat.cols[0].reg, tx);
	newPos = _mm_add_ps(newPos, _mm_mul_ps(mat.cols[1].reg, ty));
	newPos = _mm_add_ps(newPos, _mm_mul_ps(mat.cols[2].reg, tz));
	newPos = _mm_add_ps(newPos, mat.cols[3].reg);

	result.cols[3].reg = newPos;
	return result;
}

static inline Matrix4 Matrix4_ScaleF(const Matrix4 mat, float scaleX, float scaleY, float scaleZ)
{
	Matrix4 scaleMatrix; // No need to init, we fill every byte below

	// Directly scale the basis vectors of the existing matrix
	scaleMatrix.cols[0].reg = _mm_mul_ps(mat.cols[0].reg, _mm_set1_ps(scaleX));
	scaleMatrix.cols[1].reg = _mm_mul_ps(mat.cols[1].reg, _mm_set1_ps(scaleY));
	scaleMatrix.cols[2].reg = _mm_mul_ps(mat.cols[2].reg, _mm_set1_ps(scaleZ));

	// Pass the translation through untouched
	scaleMatrix.cols[3].reg = mat.cols[3].reg;

	return (scaleMatrix);
}

static inline Matrix4 Matrix4_Scale(const Matrix4 mat, Vector3 v3Scale)
{
	Matrix4 scaleMatrix; // No need to init, we fill every byte below

	// Directly scale the basis vectors of the existing matrix
	scaleMatrix.cols[0].reg = _mm_mul_ps(mat.cols[0].reg, _mm_set1_ps(v3Scale.x));
	scaleMatrix.cols[1].reg = _mm_mul_ps(mat.cols[1].reg, _mm_set1_ps(v3Scale.y));
	scaleMatrix.cols[2].reg = _mm_mul_ps(mat.cols[2].reg, _mm_set1_ps(v3Scale.z));

	// Pass the translation through untouched
	scaleMatrix.cols[3].reg = mat.cols[3].reg;

	return (scaleMatrix);
}
#endif // __MATRIX4_H__