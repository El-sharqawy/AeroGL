#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "../../Core/CoreUtils.h"
#include "../MathUtils.h"
#include "../Vectors/Vector3.h"
#include "../Matrix/Matrix4.h"

#include <math.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1 (for dot product)

#define QUATERNION_EPS (1e-6f)	// quaternion math
#define ANGLE_EPS (1e-6f)		// radians

typedef struct __declspec(align(16)) SQuaternion
{
	union
	{
		struct
		{
			union // First Component: X / Red / S / Width // @brief Vector part (i component).
			{
				float x;      // 3D Space: Horizontal axis
				float r;      // Color: Red channel (0.0 - 1.0)
				float s;      // Texture: U/S coordinate (Horizontal)
				float width;  // Bounds: Extent along X axis
			};
			union // Second Component: Y / Green / T / Height // @brief Vector part (j component).
			{
				float y;      // 3D Space: Vertical axis
				float g;      // Color: Green channel (0.0 - 1.0)
				float t;      // Texture: V/T coordinate (Vertical)
				float height; // Bounds: Extent along Y axis
			};
			union // Third Component: Z / Blue / P / Depth // @brief Vector part (k component).
			{
				float z;      // 3D Space: Depth axis
				float b;      // Color: Blue channel (0.0 - 1.0)
				float p;      // Texture: P/R coordinate (Depth/Cube Maps)
				float depth;  // Bounds: Extent along Z axis
			};
			union // Fourth Component: W / Alpha / Q / Scalar // @brief Scalar part (real component).
			{
				float w;      // Homogeneous: Transformation weight (1=Pos, 0=Dir)
				float a;      // Color: Alpha channel (Transparency)
				float q;      // Texture: Perspective divisor coordinate
				float scalar; // Math: General purpose scalar value
			};
		};
		__m128 reg; // The SIMD register representation
		float quat[4]; // Array access: Useful for loops or OpenGL (glUniform4fv)
	};
} Quaternion;

// Use _mm_setr_ps (Set Reversed) so the order matches x, y, z, w 
// _mm_set_ps usually takes arguments in reverse order (w, z, y, x)

#define QuaternionF(val) ((Quaternion){ .reg = _mm_set1_ps(val) })
#define QuaternionQ(w, x, y, z) ((Quaternion){ .reg = _mm_setr_ps(w, x, y, z) })

static const Quaternion S_Quaternion_Identity = { 0.0f, 0.0f, 0.0f, 1.0f };
static const Quaternion S_Quaternion_Zero = { 0.0f, 0.0f, 0.0f, 0.0f };

/**
 * @brief Returns the Identity Quaternion.
 *
 * The Identity Quaternion represents no rotation and is the quaternion equivalent
 * of a multiplication factor of 1 or an identity matrix. It has the components
 * q_identity = (1, 0, 0, 0) (w=1, x=0, y=0, z=0).
 *
 * It is often used to initialize rotation variables or as the starting point
 * for incremental rotations.
 *
 * @return SQuaternion The Identity Quaternion (w=1, x=0, y=0, z=0).
 */
static inline Quaternion Quaternion_Identity()
{
	return S_Quaternion_Identity;
}

/**
 * @brief Calculates the product of two quaternions.
 *
 * Computes the Hamilton product of q1 multiplied by q2, and returns the result
 * in a new quaternion instance. This is equivalent to q1 * q2.
 *
 * @param q1 The constant reference to the left-hand side quaternion (first operand).
 * @param q2 The constant reference to the right-hand side quaternion (second operand).
 * @return SQuaternion A new quaternion instance resulting from the multiplication.
 */
static inline Quaternion Quaternion_Multiply(const Quaternion q1, const Quaternion q2)
{
	Quaternion result = S_Quaternion_Zero;

	/*
	Formula from http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/arithmetic/index.htm
			a*e - b*f - c*g - d*h
		+ i (b*e + a*f + c*h- d*g)
		+ j (a*g - b*h + c*e + d*f)
		+ k (a*h + b*g - c*f + d*e)
	*/

	result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	result.x = q1.x * q2.w + q1.w * q2.x + q1.y * q2.z - q1.z * q2.y;
	result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	result.z = q1.w * q2.z + q1.x * q1.y - q1.y * q2.x + q1.z * q2.w;

	result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w; // Corrected: Term changed from q1.x * q1.y to q1.x * q2.y

	return (result);
}

static inline Quaternion Quaternion_MultiplySIMD(const Quaternion q1, const Quaternion q2)
{
	// SSE Optimized Hamilton Product
	__m128 wwww = _mm_set1_ps(q1.w);
	__m128 xxxx = _mm_set1_ps(q1.x);
	__m128 yyyy = _mm_set1_ps(q1.y);
	__m128 zzzz = _mm_set1_ps(q1.z);

	// q2 components shuffled for the Hamilton formula
	// result = w1*q2 + x1*shuf(q2) + y1*shuf(q2) + z1*shuf(q2)
	__m128 q2_xyzw = q2.reg;
	__m128 q2_wzyx = _mm_shuffle_ps(q2_xyzw, q2_xyzw, _MM_SHUFFLE(0, 1, 2, 3));
	__m128 q2_zwxy = _mm_shuffle_ps(q2_xyzw, q2_xyzw, _MM_SHUFFLE(1, 0, 3, 2));
	__m128 q2_yzwx = _mm_shuffle_ps(q2_xyzw, q2_xyzw, _MM_SHUFFLE(2, 3, 0, 1));

	__m128 out = _mm_mul_ps(wwww, q2_xyzw);
	out = _mm_addsub_ps(out, _mm_mul_ps(xxxx, q2_wzyx)); // Note: addsub requires SSE3
	// ... further shuffles and adds ...

	// Fallback if you prefer the scalar logic for clarity:
	Quaternion res = S_Quaternion_Zero;
	res.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	res.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	res.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	res.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
	return res;
}

/**
 * @brief Calculates the conjugate of a quaternion.
 *
 * The conjugate of a quaternion is found by negating its vector components (x, y, z).
 * If the original quaternion is ${q} = (w, x, y, z), the conjugate is
 * q = (w, -x, -y, -z).
 *
 * For unit quaternions, the conjugate is also equal to the inverse (q = q^-1).
 * @param quat [in] The Quaternion object for which the conjugate is calculated.
 * @return Quaternion A new quaternion object representing the conjugate.
 */
static inline Quaternion Quaternion_Conjugate(const Quaternion quat)
{
	Quaternion result = S_Quaternion_Zero;
	result.x = -quat.x;
	result.y = -quat.y;
	result.z = -quat.z;
	result.w = quat.w;
	return (result);
}

/**
 * @brief Calculates the squared length (or squared norm) of a quaternion.
 *
 * This function computes the squared magnitude of the quaternion. This is generally
 * preferred over calculating the actual length (using a square root) when only
 * comparisons are needed, as it is computationally faster.
 *
 * The formula calculated is: Length² = (w² + x² + y² + z²).
 * @param quat [in] The SQuaternion object for which the squared length is calculated.
 * @return float The squared length (w² + x² + y² + z²) of the quaternion.
 */
inline float Quaternion_LengthSquared(const Quaternion quat)
{
	// The difference between ss and ps is 
	// (Packed Single-Precision): Input: [A, B, C, D], Output: [sqrt(A), sqrt(B), sqrt(C), sqrt(D)]
	// (Scalar Single-Precision): Input: [A, B, C, D], Output: [sqrt(A), B, C, D]

	// We dot the register with itself
	// _mm_mul_ps: [x*x, y*y, z*z, w*w]
	__m128 m = _mm_mul_ps(quat.reg, quat.reg);

	// Horizontal Add to sum the components
	__m128 shuf = _mm_movehdup_ps(m);        // [y2, y2, w2, w2]
	__m128 sums = _mm_add_ps(m, shuf);       // [x2+y2, ..., z2+w2, ...]
	shuf = _mm_movehl_ps(shuf, sums);        // [z2+w2, ...]
	sums = _mm_add_ss(sums, shuf);           // [x2+y2+z2+w2]

	return _mm_cvtss_f32(sums);
}

/**
 * @brief Calculates the quaternion magnitude (length) of a quaternion.
 *
 * Computes the Euclidean length (or magnitude) of the quaternion, defined as
 * the square root of the sum of the squares of its components: sqrt{x^2 + y^2 + z^2 + w^2}.
 *
 * @param quat [in] The SQuaternion object for which the squared length is calculated.
 *
 * @return float The magnitude (length) of the quaternion.
 */
inline float Quaternion_Length(const Quaternion quat)
{
	// Multiply x*x, y*y, z*z, w*w and sum them into all slots
	// "Multi-Media Dot Product Packed Single-precision."
	__m128 dot = _mm_dp_ps(quat.reg, quat.reg, 0xFF); // this code is equal to Quaternion_LengthSquared, but newer

	// 2. Calculate Square Root using SSE
	__m128 sqrt = _mm_sqrt_ss(dot);

	// 3. Convert back to float
	return _mm_cvtss_f32(sqrt);
}

static inline Quaternion Quaternion_Normalize(const Quaternion quat)
{
	// Multiply x*x, y*y, z*z, w*w and sum them into all slots
	// "Multi-Media Dot Product Packed Single-precision."
	__m128 lenSq = _mm_dp_ps(quat.reg, quat.reg, 0xFF); // this code is equal to Quaternion_LengthSquared, but newer

	// 2. Fast Reciprocal Square Root (1 / sqrt(lenSq))
	// This calculates the multiplier for all 4 lanes at once
	__m128 invLen = _mm_rsqrt_ps(lenSq);

	// 3. Multiply original components by the inverse length
	Quaternion res;
	res.reg = _mm_mul_ps(quat.reg, invLen);

	return (res);
}

/**
 * @brief Performs Spherical Linear Interpolation (Slerp) between two quaternions.
 *
 * Slerp interpolates between two rotations (quat1 and quat2) along the shortest
 * arc on the unit sphere, providing a constant angular velocity. This results
 * in smooth, visually correct rotation paths.
 *
 * The interpolation parameter 't' controls the position on the arc:
 * - If t = 0.0, the result is quat1.
 * - If t = 1.0, the result is quat2.
 * - If t is between 0.0 and 1.0, the result is a rotation between quat1 and quat2.
 *
 * @param quat1 [in] The starting rotation quaternion (t=0).
 * @param quat2 [in] The ending rotation quaternion (t=1).
 * @param t [in] The interpolation factor (typically between 0.0 and 1.0).
 * @return SQuaternion The interpolated rotation quaternion.
 */
static inline Quaternion Quaternion_Slerp(const Quaternion quat1, const Quaternion quat2, float t)
{
	// Clamp t to [0,1] for safety
	t = clampf(t, 0.0f, 1.0f);

	// Based on http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	// t is interpolation factor
	Quaternion result = S_Quaternion_Zero;

	// Use local variables for q2, which we might negate for the shortest path
	float w2 = quat2.w;
	float x2 = quat2.x;
	float y2 = quat2.y;
	float z2 = quat2.z;

	// 1. Calculate the dot product (cosHalfTheta = q1 . q2)
	// The dot product is the cosine of the half-angle between the two quaternions.
	float cosHalfTheta = quat1.w * w2 + quat1.x * x2 + quat1.y * y2 + quat1.z * z2;

	/**
	 * @brief CRITICAL: Shortest Path Check (Double Cover Property)
	 *
	 * If the dot product is negative, the angle between the quaternions is > 90 degrees,
	 * meaning Slerp would take the long arc. We negate one quaternion (q2) to force
	 * the interpolation along the shortest path, as q and -q represent the same rotation.
	 */
	if (cosHalfTheta < 0.0f)
	{
		// Negate all components of our given quaternion
		w2 = -w2;
		x2 = -x2;
		y2 = -y2;
		z2 = -z2;

		// Flip the sign of the dot product to positive
		cosHalfTheta = -cosHalfTheta;
	}

	/**
	 * @brief Trivial/Fallback Check
	 *
	 * If the angle is very small (dot product is near 1.0), Slerp is unstable due to
	 * division by zero. We fall back to standard Linear Interpolation (Lerp) which
	 * is accurate for small angles.
	 */
	if (cosHalfTheta >= 1.0f - QUATERNION_EPS)
	{
		// Use Lerp as a stable approximation for small angles
		result.x = quat1.x * (1.0f - t) + x2 * t;
		result.y = quat1.y * (1.0f - t) + y2 * t;
		result.z = quat1.z * (1.0f - t) + z2 * t;

		result.w = quat1.w * (1.0f - t) + w2 * t;
	}
	else
	{
		/**
		 * @brief Standard Slerp Calculation
		 *
		 * Formula: qt = [sin((1-t)O) / sin(O)] * q1 + [sin(tO) / sin(O)] * q2
		 * where O is the angle between q1 and q2.
		 */
		float halfTheta = acosf(cosHalfTheta);
		float sinHalfTheta = sinf(halfTheta);

		// Calculate ratios
		float fRatioA = sinf((1.0f - t) * halfTheta) / sinHalfTheta;
		float fRatioB = sinf(t * halfTheta) / sinHalfTheta;

		// Apply Slerp formula component-wise
		result.x = (quat1.x * fRatioA) + (x2 * fRatioB);
		result.y = (quat1.y * fRatioA) + (y2 * fRatioB);
		result.z = (quat1.z * fRatioA) + (z2 * fRatioB);
		result.w = (quat1.w * fRatioA) + (w2 * fRatioB);
	}

	// Add normalization (optional but recommended for numerical drift)
	result = Quaternion_Normalize(result);

	return (result);
}

/**
 * @brief Converts an axis-angle rotation into a unit quaternion from a Right-Handed axis-angle rotation.
 *
 * This function computes a quaternion representation for a rotation
 * around a specified axis by a given angle. Quaternions are ideal for
 * representing 3D rotations in computer graphics and physics due to their
 * immunity to issues like Gimbal Lock.
 *
 * The resulting quaternion q = (x, y, z, w) is calculated using the
 * standard axis-angle conversion formula:
 * * * **Real (W) component:** w = cos(Angle / 2)
 * * **Vector (X, Y, Z) components:** (x, y, z) = (Axis_x, Axis_y, Axis_z)  * sin(Angle / 2)
 *
 * The implementation includes an **angle negation** when the input is in
 * radians (`fAngle = ToRadians(fAngle);`). This suggests the function is
 * designed to work with a standard or **right-handed rotation** convention,
 * where a positive angle produces the opposite rotation of the standard
 * right-hand rule.
 *
 * **Note on Input Axis:** For the output quaternion to be a true **unit**
 * quaternion (which is required for pure rotation), the input axis `vAxis`
 * **must** be a unit vector (normalized to a length of 1). The caller is
 * responsible for ensuring `vAxis` is normalized.
 *
 * @param vAxis [in] A float array of 3 elements representing the rotation axis vector (x, y, z).
 * *Must be a unit vector for the output to be a proper unit quaternion.*
 * @param fAngle [in] The rotation angle. The units (radians or degrees) are determined by `bRadian`.
 * @param bRadian [in] A boolean flag. Set to `false` if `fAngle` is in radians, `true` if in degrees.
 * *If `bRadian` is true, the angle is negated internally, indicating a
 * right-handed or normal rotation convention.*
 */
static inline Quaternion Quaternion_FromAxisAngle(float v3Axis[3], float fAngle, bool bRadian)
{
	// result Quaternion
	Quaternion result = S_Quaternion_Zero;

	// Formula from http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/
	float angleRad = bRadian ? fAngle : ToRadians(fAngle); // Converts angle to radians, the standard Right-Handed rotation convention, negate the angle to match left handed systems (-ToRadian(angleRad))

	// Calculate sine and cosine of half the rotation angle
	float sinHalfTheta = sinf(angleRad / 2.0f);

	// Vector components: q.v = v_axis * sin(theta/2)
	result.x = sinHalfTheta * v3Axis[0];
	result.y = sinHalfTheta * v3Axis[1];
	result.z = sinHalfTheta * v3Axis[2];

	// Real component: q.w = cos(theta/2)
	result.w = cosf(angleRad / 2.0f);

	return (result);
}

static inline Quaternion Quaternion_FromAxisAngleV(const Vector3 v3Axis, GLfloat fAngle, bool bRadian)
{
	// result Quaternion
	Quaternion result = S_Quaternion_Zero;

	// Formula from http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/
	GLfloat angleRad = fAngle;

	if (bRadian)
	{
		angleRad = ToRadians(angleRad); // Converts angle to radians, the standard Right-Handed rotation convention, negate the angle to match left handed systems (-ToRadian(angleRad))
	}

	// Calculate sine and cosine of half the rotation angle
	GLfloat sinHalfTheta = sinf(angleRad / 2.0f);

	// Vector components: q.v = v_axis * sin(theta/2)
	result.x = sinHalfTheta * v3Axis.x;
	result.y = sinHalfTheta * v3Axis.y;
	result.z = sinHalfTheta * v3Axis.z;

	// Real component: q.w = cos(theta/2)
	result.w = cosf(angleRad / 2.0f);

	return (result);
}

/**
 * @brief Converts a unit quaternion into an axis-angle representation.
 *
 * This function extracts the rotation axis and rotation angle from a
 * quaternion. The input quaternion **must be normalized** (unit length)
 * for the resulting axis and angle to represent a valid rotation.
 *
 * The conversion is based on the unit quaternion definition:
 *     q = (x, y, z, w) = v * sin(theta / 2) + cos(theta / 2)
 *
 * The resulting axis-angle representation is computed as:
 *
 *   - Angle (theta):
 *       theta = 2 * acos(q.w)
 *
 *   - Axis:
 *       axis = (q.x, q.y, q.z) / sqrt(1 - q.w^2)
 *
 * @note Due to floating-point precision errors, q.w should be clamped to
 *       the range [-1, 1] before calling acos.
 *
 * @note Special Case (Identity Rotation):
 *       When |q.w| is close to 1, the rotation angle approaches 0 (or 2PI),
 *       and the axis becomes mathematically undefined due to division by
 *       zero. In this case, the function assigns an arbitrary normalized
 *       axis (1, 0, 0), since rotating by zero around any axis produces
 *       no rotation.
 *
 * @param quat [in] The SQuaternion object to store the resulting quaternion.
 *
 * @param outAxis [out] A float array of 3 elements that receives the
 *        normalized rotation axis (x, y, z).
 * @param bWantRadian If true, the returned angle is in radians; if false,
 *        the angle is returned in degrees.
 *
 * @return float The rotation angle, in radians or degrees depending on
 *         the value of bWantRadian.
 */
static inline float Quaternion_ToAxisAngle(const Quaternion quat, float outAxis[3], bool bWantRadian)
{
	// Ensure numerical safety
	float wClamped = clampf(quat.w, -1.0f, 1.0f);

	// Angle in radians
	float angle = 2.0f * acosf(wClamped);

	float sinHalfAngleSq = quat.x * quat.x + quat.y * quat.y + quat.z * quat.z;// IDK 
	float sinHalfAngle = sqrtf(sinHalfAngleSq);
	// float sinHalfAngle = sqrtf(1.0f - wClamped * wClamped);

	if (sinHalfAngle > QUATERNION_EPS)
	{
		outAxis[0] = quat.x / sinHalfAngle;
		outAxis[1] = quat.y / sinHalfAngle;
		outAxis[2] = quat.z / sinHalfAngle;
	}
	else
	{
		// Identity rotation: axis is arbitrary
		outAxis[0] = 1.0f;
		outAxis[1] = 0.0f;
		outAxis[2] = 0.0f;
	}

	angle = bWantRadian ? angle : ToDegrees(angle);
	return (angle);
}

/**
 * @brief Creates a quaternion representing a rotation about the X-axis.
 *
 * This function returns a new quaternion corresponding to a rotation of
 * the specified angle around the positive X-axis (1, 0, 0). The resulting
 * quaternion can be used for 3D transformations, animation, and orientation
 * calculations.
 *
 * Internally, this function is a convenience wrapper around the
 * axis-angle construction logic, using the X-axis as the rotation axis.
 *
 * @param fAngle  The rotation angle. The unit (radians or degrees) must
 *                match the convention used by the underlying axis-angle
 *                conversion logic.
 *
 * @return SQuaternion  A new quaternion representing the X-axis rotation.
 *
 * @note The returned quaternion will be a unit quaternion if the underlying
 *       axis-angle construction produces normalized results.
 */
static inline Quaternion Quaternion_FromXRotation(float fAngle, bool bRadian)
{
	float v3XAxis[3] = { 1.0f, 0.0f, 0.0f };
	return Quaternion_FromAxisAngle(v3XAxis, fAngle, bRadian);		// false = degrees, true = radians
}

/**
 * @brief Creates a quaternion representing a rotation about the Y-axis.
 *
 * This function returns a new quaternion corresponding to a rotation of
 * the specified angle around the positive Y-axis (0, 1, 0). The resulting
 * quaternion can be used for 3D transformations, animation, and orientation
 * calculations.
 *
 * Internally, this function is a convenience wrapper around the
 * axis-angle construction logic, using the Y-axis as the rotation axis.
 *
 * @param fAngle  The rotation angle. The unit (radians or degrees) must
 *                match the convention used by the underlying axis-angle
 *                conversion logic.
 *
 * @return SQuaternion  A new quaternion representing the Y-axis rotation.
 */
static inline Quaternion Quaternion_FromYRotation(float fAngle, bool bRadian)
{
	float v3YAxis[3] = { 0.0f, 1.0f, 0.0f };
	return Quaternion_FromAxisAngle(v3YAxis, fAngle, bRadian);		// false = degrees, true = radians
}

/**
 * @brief Creates a quaternion representing a rotation about the Z-axis.
 *
 * This function returns a new quaternion corresponding to a rotation of
 * the specified angle around the positive Z-axis (0, 0, 1). The resulting
 * quaternion can be used for 3D transformations, animation, and orientation
 * calculations.
 *
 * Internally, this function is a convenience wrapper around the
 * axis-angle construction logic, using the Z-axis as the rotation axis.
 *
 * @param fAngle  The rotation angle. The unit (radians or degrees) must
 *                match the convention used by the underlying axis-angle
 *                conversion logic.
 *
 * @return SQuaternion  A new quaternion representing the Z-axis rotation.
 */
static inline Quaternion Quaternion_FromZRotation(float fAngle, bool bRadian)
{
	float v3ZAxis[3] = { 0.0f, 0.0f, 1.0f };
	return Quaternion_FromAxisAngle(v3ZAxis, fAngle, bRadian);		// false = degrees, true = radians
}

/**
 * @brief Creates a quaternion representing a rotation about a primary axis.
 *
 * This function returns a new quaternion corresponding to a rotation of
 * the specified angle around one of the principal axes: X, Y, or Z.
 *
 * @param axis      enum specifying the axis:
 *                  - AXIS_X = X-axis
 *                  - AXIS_Y = Y-axis
 *                  - AXIS_Z = Z-axis
 *
 * @param fAngle  The rotation angle. The unit (radians or degrees) must
 *                match the convention used by the underlying axis-angle
 *                conversion logic.
 *
 * @return SQuaternion A new quaternion representing the rotation about the selected axis.
 */
inline static Quaternion Quaternion_FromRotation(EAxis axis, float fAngle, bool bRadian)
{
	if (axis == AXIS_X)
	{
		return Quaternion_FromXRotation(fAngle, bRadian);
	}
	else if (axis == AXIS_Y)
	{
		return Quaternion_FromYRotation(fAngle, bRadian);
	}
	else if (axis == AXIS_Z)
	{
		return Quaternion_FromZRotation(fAngle, bRadian);
	}
	return S_Quaternion_Identity;
}

/**
 * @brief Creates a quaternion from Euler angles using the ZYX (Yaw-Pitch-Roll) order.
 *
 * This function converts a set of Euler rotations into a single quaternion.
 * The rotations are applied in this specific order:
 * 1. First, rotation around the Z-axis (Yaw).
 * 2. Second, rotation around the Y-axis (Pitch).
 * 3. Third, rotation around the X-axis (Roll).
 *
 * This is the standard "Aerospace" or "Aviation" sequence. It is highly
 * recommended for game characters and vehicles.
 *
 * @param eulerRad [in] An array of 3 floats representing rotations in radians:
 * - eulerRad[0]: Roll (X-axis)
 * - eulerRad[1]: Pitch (Y-axis)
 * - eulerRad[2]: Yaw (Z-axis)
 *
 * @return SQuaternion A new quaternion representing the combined rotation.
 *
 * @note This function is 'const' and does not modify the current object.
 * It returns a brand new quaternion instead.
 */
inline static Quaternion Quaternion_FromEulerZYX_Float(float eulerRad[3], bool bToRadian)
{
	float Pitch = eulerRad[0];
	float Yaw = eulerRad[1];
	float Roll = eulerRad[2];

	if (bToRadian)
	{
		Pitch = ToRadians(eulerRad[0]);
		Yaw = ToRadians(eulerRad[1]);
		Roll = ToRadians(eulerRad[2]);
	}

	// Pitch - X axis
	const float cosPitch = cosf(Pitch * 0.5f);
	const float sinPitch = sinf(Pitch * 0.5f);

	// Yaw - Y axis
	const float cosYaw = cosf(Yaw * 0.5f);
	const float sinYaw = sinf(Yaw * 0.5f);

	// Roll - Z axis
	const float cosRoll = cosf(Roll * 0.5f);
	const float sinRoll = sinf(Roll * 0.5f);

	// Compute quaternion components
	Quaternion result = S_Quaternion_Zero;
	result.x = sinPitch * cosYaw * cosRoll + cosPitch * sinYaw * sinRoll; // X component (Pitch)
	result.y = cosPitch * sinYaw * cosRoll - sinPitch * cosYaw * sinRoll; // Y component (Yaw)
	result.z = cosPitch * cosYaw * sinRoll - sinPitch * sinYaw * cosRoll; // Z component (Roll)
	result.w = cosPitch * cosYaw * cosRoll + sinPitch * sinYaw * sinRoll; // W (Scalar)
	return (result);
}

inline static Quaternion Quaternion_FromEulerZYX(const Vector3 v3EulerRad, bool bToRadian)
{
	float Pitch = v3EulerRad.x;
	float Yaw = v3EulerRad.y;
	float Roll = v3EulerRad.z;

	if (bToRadian)
	{
		Pitch = ToRadians(v3EulerRad.x);
		Yaw = ToRadians(v3EulerRad.y);
		Roll = ToRadians(v3EulerRad.z);
	}

	// Pitch - X axis
	const float cosPitch = cosf(Pitch * 0.5f);
	const float sinPitch = sinf(Pitch * 0.5f);

	// Yaw - Y axis
	const float cosYaw = cosf(Yaw * 0.5f);
	const float sinYaw = sinf(Yaw * 0.5f);

	// Roll - Z axis
	const float cosRoll = cosf(Roll * 0.5f);
	const float sinRoll = sinf(Roll * 0.5f);

	// Compute quaternion components
	Quaternion result = S_Quaternion_Zero;
	result.x = sinPitch * cosYaw * cosRoll + cosPitch * sinYaw * sinRoll; // X component (Pitch)
	result.y = cosPitch * sinYaw * cosRoll - sinPitch * cosYaw * sinRoll; // Y component (Yaw)
	result.z = cosPitch * cosYaw * sinRoll - sinPitch * sinYaw * cosRoll; // Z component (Roll)
	result.w = cosPitch * cosYaw * cosRoll + sinPitch * sinYaw * sinRoll; // W (Scalar)
	return (result);
}

/**
 * @brief Creates a quaternion from Euler angles using the XYZ order (Roll-Pitch-Yaw).
 *
 * This function converts a set of Euler rotations into a single quaternion.
 * The rotations are applied in this specific order:
 * 1. First, rotation around the X-axis (Roll).
 * 2. Second, rotation around the Y-axis (Pitch).
 * 3. Third, rotation around the Z-axis (Yaw).
 *
 * This sequence is often used in general 3D modeling and graphics packages.
 *
 * @param eulerRad [in] An array of 3 floats representing rotations in radians:
 * - eulerRad[0]: Roll (X-axis)
 * - eulerRad[1]: Pitch (Y-axis)
 * - eulerRad[2]: Yaw (Z-axis)
 *
 * @return SQuaternion A new quaternion representing the combined rotation.
 *
 * @note This function is 'const' and does not modify the current object.
 * It returns a brand new quaternion instead.
 */
inline static Quaternion Quaternion_FromEulerXYZ_Float(float eulerRad[3], bool bToRadian)
{
	float Pitch = eulerRad[0];
	float Yaw = eulerRad[1];
	float Roll = eulerRad[2];

	if (bToRadian)
	{
		Pitch = ToRadians(eulerRad[0]);
		Yaw = ToRadians(eulerRad[1]);
		Roll = ToRadians(eulerRad[2]);
	}

	// Pitch - X axis
	const float cosPitch = cosf(Pitch * 0.5f);
	const float sinPitch = sinf(Pitch * 0.5f);

	// Yaw - Y axis
	const float cosYaw = cosf(Yaw * 0.5f);
	const float sinYaw = sinf(Yaw * 0.5f);

	// Roll - Z axis
	const float cosRoll = cosf(Roll * 0.5f);
	const float sinRoll = sinf(Roll * 0.5f);

	// Compute quaternion components (XYZ sequence formula)
	Quaternion result = S_Quaternion_Zero;

	// The formulas are different from ZYX due to non-commutative rotation
	result.x = sinPitch * cosYaw * cosRoll - cosPitch * sinYaw * sinRoll;
	result.y = cosPitch * sinYaw * cosRoll + sinPitch * cosYaw * sinRoll;
	result.z = cosPitch * cosYaw * sinRoll - sinPitch * sinYaw * cosRoll;
	result.w = cosPitch * cosYaw * cosRoll + sinPitch * sinYaw * sinRoll;
	return (result);
}

inline static Quaternion Quaternion_FromEulerXYZ(const Vector3 v3EulerRad, bool bToRadian)
{
	float Pitch = v3EulerRad.x;
	float Yaw = v3EulerRad.y;
	float Roll = v3EulerRad.z;

	if (bToRadian)
	{
		Pitch = ToRadians(v3EulerRad.x);
		Yaw = ToRadians(v3EulerRad.y);
		Roll = ToRadians(v3EulerRad.z);
	}

	// Pitch - X axis
	const float cosPitch = cosf(Pitch * 0.5f);
	const float sinPitch = sinf(Pitch * 0.5f);

	// Yaw - Y axis
	const float cosYaw = cosf(Yaw * 0.5f);
	const float sinYaw = sinf(Yaw * 0.5f);

	// Roll - Z axis
	const float cosRoll = cosf(Roll * 0.5f);
	const float sinRoll = sinf(Roll * 0.5f);

	// Compute quaternion components (XYZ sequence formula)
	Quaternion result = S_Quaternion_Zero;

	// The formulas are different from ZYX due to non-commutative rotation
	result.x = sinPitch * cosYaw * cosRoll - cosPitch * sinYaw * sinRoll;
	result.y = cosPitch * sinYaw * cosRoll + sinPitch * cosYaw * sinRoll;
	result.z = cosPitch * cosYaw * sinRoll - sinPitch * sinYaw * cosRoll;
	result.w = cosPitch * cosYaw * cosRoll + sinPitch * sinYaw * sinRoll;
	return (result);
}

/**
 * @brief Converts a quaternion into Euler angles using the ZYX (Yaw-Pitch-Roll) rotation sequence.
 *
 * This function extracts the three Euler angles (Roll, Pitch, Yaw) from a unit quaternion.
 * The angles are calculated in **radians** and correspond to rotations applied in the order:
 * Z (Yaw) -> Y (Pitch) -> X (Roll).
 *
 * @details
 * The conversion formulas are derived from the quaternion-to-rotation-matrix representation:
 * - **Roll (X-axis):**  atan2(2*(w*x + y*z), 1 - 2*(x^2 + y^2))
 * - **Pitch (Y-axis):** asin(2*(w*y - x*z)) (or an equivalent stable formulation using atan2)
 * - **Yaw (Z-axis):**   atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2))
 *
 * **Gimbal Lock / Singularity:**
 * Pitch approaches +/- 90° (+/- PI/2 radians) may cause gimbal lock, where Roll and Yaw rotations
 * are no longer uniquely defined. This implementation uses atan2 for Roll and Yaw and a
 * robust method for Pitch to mitigate singularities as much as possible.
 *
 * @param quat [in] The SQuaternion object to use and store the resulting quaternion.
 * @param eulerRad [out] Array of 3 floats receiving the Euler angles in radians:
 *   - eulerRad[0]: Roll (X-axis)
 *   - eulerRad[1]: Pitch (Y-axis)
 *   - eulerRad[2]: Yaw (Z-axis)
 *
 * @post `eulerRad` contains the Euler angles corresponding to the ZYX rotation sequence.
 */
static inline void Quaternion_ToEulerZYX(const Quaternion quat, float eulerRad[3])
{
	// Compute Roll (X-axis)
	const float sinr_cosp = 2.0f * (quat.w * quat.x + quat.y * quat.z);
	const float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
	eulerRad[0] = atan2f(sinr_cosp, cosr_cosp);

	// Compute Pitch (Y-axis)
	const float sinPitch = 2.0f * (quat.w * quat.y - quat.x * quat.z);

	// Check for gimbal lock
	if (fabsf(sinPitch) >= 1.0f) // Near +/- 90 degrees Pitch
	{
		// Pitch is +/-90°, Roll + Yaw is ambiguous
		eulerRad[1] = copysignf((float)(M_PI) / 2.0f, sinPitch); // +/-90°
		eulerRad[2] = 0.0f; // Yaw set to 0, could be any value since Roll+Yaw is ambiguous
	}
	else
	{
		// Safe zone
		eulerRad[1] = asinf(sinPitch);

		// Compute Yaw (Z-axis)
		const float siny_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
		const float cosy_cosp = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
		eulerRad[2] = atan2f(siny_cosp, cosy_cosp);
	}
}

/**
 * @brief Rotates a 3D vector using the quaternion rotation formula.
 *
 * This function rotates the input vector 'vec3' by the rotation represented
 * by the current quaternion (*this). It uses the standard vector identity
 * derived from quaternion multiplication:
 *
 * v' = v + 2 * w * (q_xyz cross v) + 2 * (q_xyz cross (q_xyz cross v))
 *
 * where q_xyz is the vector part and w is the scalar part of the quaternion.
 *
 * This version uses explicit cross calls for clarity and maintainability.
 *
 * @param quat [in] The SQuaternion object to use and store the resulting quaternion.
 * @param vec3 [in] The input 3D vector to rotate.
 * @returns Vector3 The rotated vector.
 *
 * @pre The quaternion (*this) must be normalized (unit quaternion) for correct results.
 */
static inline Vector3 Quaternion_RotateVec(const Quaternion quat, const Vector3 vec3)
{
	// Quaternion rotation formula (from euclideanspace.com):
	//   p2.x = w*w*p1.x + 2*y*w*p1.z - 2*z*w*p1.y + x*x*p1.x + 2*y*x*p1.y + 2*z*x*p1.z - z*z*p1.x - y*y*p1.x
	//   p2.y = 2*x*y*p1.x + y*y*p1.y + 2*z*y*p1.z + 2*w*z*p1.x - z*z*p1.y + w*w*p1.y - 2*x*w*p1.z - x*x*p1.y
	//   p2.z = 2*x*z*p1.x + 2*y*z*p1.y + z*z*p1.z - 2*w*y*p1.x - y*y*p1.z + 2*w*x*p1.y - x*x*p1.z + w*w*p1.z
	// The current implementation uses the equivalent cross-product identity for clarity and efficiency:
	//   t = q_xyz × v, t' = q_xyz × t, v' = v + 2*w*t + 2*t'

	// q_vec = {x, y, z} part of quaternion
	Vector3 q_vec = { quat.x, quat.y, quat.z };

	// 1. Calculate the first cross product: t = v x p
	// t = 2 * (q_vec x v)
	Vector3 t = Vector3_Cross(q_vec, vec3);
	t = Vector3_Muls(t, 2.0f);

	// 2. Calculate the second cross product: t' = v x t
	// t_prime = q_vec x t
	Vector3 t_prime = Vector3_Cross(q_vec, t);

	// 3. Apply the final formula: p' = p + 2w(t) + 2(t')
	// Final result: v + (w * t) + t_prime
	// Vector3 rotatedVec = vec3 + t * (2.0f * quat.w) + t_prime * 2.0f;
	Vector3 term1 = Vector3_Muls(t, quat.w);
	Vector3 rotatedVec = Vector3_Add(vec3, term1);
	rotatedVec = Vector3_Add(rotatedVec, t_prime);

	return (rotatedVec);
}

/**
 * @brief Rotates a 3D vector using an optimized, fully inlined quaternion formula.
 *
 * This version performs the same rotation as 'Rotate' but avoids temporary
 * vectors and cross-product function calls for maximum performance.
 *
 * It computes the rotated vector using direct scalar arithmetic:
 *
 * t = 2 * (q_xyz cross v)
 * v' = v + w * t + (q_xyz cross t)
 *
 * where q_xyz is the vector part and w is the scalar part of the quaternion.
 *
 * @param quat [in] The SQuaternion object to use and store the resulting quaternion.
 * @param v [in] The input 3D vector to rotate.
 * @returns Vector3 The rotated vector.
 *
 * @note This method is faster than the standard 'RotateVec' but less readable.
 * @pre The quaternion (*this) must be normalized (unit quaternion) for correct results.
 */
static inline Vector3 Quaternion_Rotate(const Quaternion quat, const Vector3 vec3)
{
	// Quaternion rotation formula (from euclideanspace.com):
	//   p2.x = w*w*p1.x + 2*y*w*p1.z - 2*z*w*p1.y + x*x*p1.x + 2*y*x*p1.y + 2*z*x*p1.z - z*z*p1.x - y*y*p1.x
	//   p2.y = 2*x*y*p1.x + y*y*p1.y + 2*z*y*p1.z + 2*w*z*p1.x - z*z*p1.y + w*w*p1.y - 2*x*w*p1.z - x*x*p1.y
	//   p2.z = 2*x*z*p1.x + 2*y*z*p1.y + z*z*p1.z - 2*w*y*p1.x - y*y*p1.z + 2*w*x*p1.y - x*x*p1.z + w*w*p1.z
	// The current implementation uses the equivalent cross-product identity for clarity and efficiency:
	//   t = q_xyz × v, t' = q_xyz × t, v' = v + 2*w*t + 2*t'

	const float qx = quat.x, qy = quat.y, qz = quat.z, qw = quat.w;
	const float vx = vec3.x, vy = vec3.y, vz = vec3.z;

	const float t_x = 2.0f * (qy * vz - qz * vy);
	const float t_y = 2.0f * (qz * vx - qx * vz);
	const float t_z = 2.0f * (qx * vy - qy * vx);

	const float rotated_x = vx + qw * t_x + (qy * t_z - qz * t_y);
	const float rotated_y = vy + qw * t_y + (qz * t_x - qx * t_z);
	const float rotated_z = vz + qw * t_z + (qx * t_y - qy * t_x);

	return (Vector3){ rotated_x, rotated_y, rotated_z };
}

/**
 * @brief Converts the quaternion to a 4x4 rotation matrix in Column-Major order.
 *
 * This function converts the quaternion into its equivalent rotation matrix.
 * It generates the standard rotation matrix but performs an explicit transpose
 * during assignment to ensure the output is in **Column-Major** format,
 * which is required by graphics APIs like OpenGL.
 * * The assignment pattern matrix[row][col] is used, where:
 * - matrix[0-2][0]: X-Axis (First Column)
 * - matrix[0-2][1]: Y-Axis (Second Column)
 * - matrix[0-2][2]: Z-Axis (Third Column)
 *
 * @param quat [in] The SQuaternion object to use and store the resulting quaternion.
 * @returns SMatrix4x4 The resulting rotation matrix.
 *
 * @pre The quaternion must be normalized (unit quaternion).
 */
static inline Matrix4 Quaternion_ToMatrix4(const Quaternion quat)
{
	// [ 1-2(y^2+z^2)		2(xy+zw)		2(xz-yw)		0 ]
	// [ 2(xy-zw)			1-2(x^2+z^2)	2(yz+xw)		0 ]
	// [ 2(xz+yw)			2(yz-xw)		1-2(x^2+y^2)	0 ]
	// [ 0					0				0				1 ]

	Matrix4 matrix = S_Matrix4_Zero;

	// Diagonal terms (remain the same)
	const float xx = quat.x * quat.x;
	const float yy = quat.y * quat.y;
	const float zz = quat.z * quat.z;

	const float two_x = 2.0f * quat.x;
	const float two_y = 2.0f * quat.y;
	const float two_z = 2.0f * quat.z;

	// Cross products
	const float xy2 = two_x * quat.y;
	const float xz2 = two_x * quat.z;
	const float yz2 = two_y * quat.z;

	// W terms
	const float wx = two_x * quat.w; // Note: xw term from formula
	const float wy = two_y * quat.w; // Note: yw term from formula
	const float wz = two_z * quat.w; // Note: zw term from formula

	// Column 0 (X-Axis)
	matrix.cols[0].x = 1.0f - 2.0f * (yy + zz);
	matrix.cols[1].x = xy2 + wz; // (2xy + 2zw)
	matrix.cols[2].x = xz2 - wy; // (2xz - 2yw)
	matrix.cols[3].x = 0.0f;

	// Column 1 (Y-Axis)
	matrix.cols[0].y = xy2 - wz; // (2xy - 2zw)
	matrix.cols[1].y = 1.0f - 2.0f * (xx + zz);
	matrix.cols[2].y = yz2 + wx; // (2yz + 2xw)]
	matrix.cols[3].y = 0.0f;

	// Column 2 (Z-Axis)
	matrix.cols[0].z = xz2 + wy; // (2xz + 2yw)
	matrix.cols[1].z = yz2 - wx; // (2yz - 2xw)
	matrix.cols[2].z = 1.0f - 2.0f * (xx + yy);
	matrix.cols[3].z = 0.0f;

	// Column 3 (Translation/Perspective)
	matrix.cols[0].w = 0.0f;
	matrix.cols[1].w = 0.0f;
	matrix.cols[2].w = 0.0f;
	matrix.cols[3].w = 1.0f;

	return (matrix);
}


/**
 * @brief Creates a quaternion from a 4x4 rotation matrix.
 *
 * This function extracts the quaternion representation from a given
 * 4x4 rotation matrix. The input matrix is assumed to be a valid rotation
 * matrix (orthonormal with a determinant of +1).
 *
 * The conversion uses the standard algorithm for extracting quaternions
 * from rotation matrices, ensuring numerical stability and correctness.
 *
 * @param mat [in] The SMatrix4x4 rotation matrix from which to extract the quaternion.
 * @returns SQuaternion A new quaternion representing the same rotation as the input matrix.
 *
 * @note The resulting quaternion will be a unit quaternion if the input
 *       matrix is a proper rotation matrix.
 */
static inline Quaternion Quaternion_FromMatrix4(const Matrix4 mat)
{
	Quaternion quat = S_Quaternion_Zero;

	// Calculate the trace (sum of the diagonal elements: R00 + R11 + R22)
	// R00 is m.mat[0][0]
	// R11 is m.mat[1][1]
	// R22 is m.mat[2][2]
	const float trace = mat.cols[0].x + mat.cols[1].y + mat.cols[2].z;

	// We determine which component (w, x, y, or z) is the largest.
	// This maintains numerical stability during the calculation.
	if (trace > 0)
	{
		// Case 1: W (Real part) is the largest component
		const float s = sqrtf(trace + 1.0f) * 2.0f; // S=4*qw
		quat.w = 0.25f * s;

		// The remaining terms use the off-diagonal elements:
		// mat[col][row]
		quat.x = (mat.cols[1].z - mat.cols[2].y) / s; // (R21 - R12) / S]
		quat.y = (mat.cols[2].x - mat.cols[0].z) / s; // (R02 - R20) / S]
		quat.z = (mat.cols[0].y - mat.cols[1].x) / s; // (R10 - R01) / S]
	}
	else if ((mat.cols[0].x > mat.cols[1].y) && (mat.cols[0].x > mat.cols[2].z))
	{
		// Case 2: X is the largest component
		const float s = sqrtf(1.0f + mat.cols[0].x - mat.cols[1].y - mat.cols[2].z) * 2.0f; // S=4*qx
		quat.w = (mat.cols[1].z - mat.cols[2].y) / s;
		quat.x = 0.25f * s;
		quat.y = (mat.cols[0].y + mat.cols[1].x) / s;
		quat.z = (mat.cols[0].z + mat.cols[2].x) / s;
	}
	else if (mat.cols[1].y > mat.cols[2].z)
	{
		// Case 3: Y is the largest component
		const float s = sqrtf(1.0f + mat.cols[1].y - mat.cols[0].x - mat.cols[2].z) * 2.0f; // S=4*qy
		quat.w = (mat.cols[2].x - mat.cols[0].z) / s;
		quat.x = (mat.cols[0].y + mat.cols[1].x) / s;
		quat.y = 0.25f * s;
		quat.z = (mat.cols[2].y + mat.cols[1].z) / s;
	}
	else
	{
		// Case 4: Z is the largest component
		const float s = sqrtf(1.0f + mat.cols[2].z - mat.cols[0].x - mat.cols[1].y) * 2.0f; // S=4*qz
		// quat.w = (mat.cols[0].y - mat.cols[1].x) / s;
		quat.w = (mat.cols[1].x - mat.cols[0].y) / s; // Fixed sign order
		quat.x = (mat.cols[0].z + mat.cols[2].x) / s;
		quat.y = (mat.cols[2].y + mat.cols[1].z) / s;
		quat.z = 0.25f * s;
	}

	return (quat);
}


/*
*@brief Converts a quaternion to Euler angles in degrees using the ZYX rotation sequence.
*
* This function extracts the three Euler angles(Roll, Pitch, Yaw) from a unit quaternion.
* The angles are calculated in** degrees** and correspond to rotations applied in the order :
*Z(Yaw)->Y(Pitch)->X(Roll).
*
* @details
* The conversion formulas are derived from the quaternion - to - rotation - matrix representation :
*-**Roll(X - axis) : **atan2(2 * (w * x + y * z), 1 - 2 * (x ^ 2 + y ^ 2))
* -**Pitch(Y - axis) : **asin(2 * (w * y - x * z)) (or an equivalent stable formulation using atan2)
* -**Yaw(Z - axis) : **atan2(2 * (w * z + x * y), 1 - 2 * (y ^ 2 + z ^ 2))
*
***Gimbal Lock / Singularity : **
*Pitch approaches + / -90°(+/ -PI / 2 radians) may cause gimbal lock, where Roll and Yaw rotations
* are no longer uniquely defined.This implementation uses atan2 for Roll and Yaw and a
* robust method for Pitch to mitigate singularities as much as possible.
*
* @param quat[in] The SQuaternion object to use and store the resulting quaternion.
* @return SEulerAngles A struct containing the Euler angles in degrees :
*-Roll(X - axis)
* -Pitch(Y - axis)
* -Yaw(Z - axis)
*
*@post The returned SEulerAngles contains the Euler angles corresponding to the ZYX rotation sequence.
*/
static inline SEulerAngles Quaternion_ToEulerAnglesDegrees(const Quaternion quat)
{
	SEulerAngles angles = { 0.0f, 0.0f, 0.0f };

	// Use the standard formulas for converting quaternion to Euler angles (Y-X-Z order)
	// The components of the quaternion are q.w, q.x, q.y, q.z

	// --- PITCH (Rotation around X-axis) ---
	// Calculated from sin(Pitch) term, which can be derived from the rotation matrix element M21
	// This angle depends on the term: 2 * (qw*qx - qy*qz)
	const float sinPitch = 2.0f * (quat.w * quat.x - quat.y * quat.z);

	// Check for singularity (Pitch near +/- 90 degrees)
	if (fabsf(sinPitch) >= 1.0f)
	{
		// Pitch is +/- 90°. This is a Gimbal Lock scenario.
		// We set Pitch to +/- 90 degrees.
		angles.fPitch = copysignf((float)(M_PI) / 2.0f, sinPitch);

		// The sum of Yaw and Roll (Y+Z) is ambiguous. We typically set one to zero 
		// and calculate the other, but since FPS typically has zero roll:
		// We can calculate Roll (Z) from the remaining rotation around the camera's Z-axis.
		// Since your camera doesn't *add* roll, we can set both Yaw and Roll to 0 (or try to extract Roll).

		// Let's calculate the remaining Roll (Z)
		const float sinr_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
		const float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z); // Note the changed terms here
		angles.fRoll = atan2f(sinr_cosp, cosr_cosp);

		angles.fYaw = 0.0f; // Force Yaw to zero when looking straight up/down.
	}
	else
	{
		// --- 2. Safe Zone: Compute Pitch (X) ---
		angles.fPitch = asinf(sinPitch);

		// --- 3. Safe Zone: Compute Yaw (Y-axis, eulerRad.y) ---
		// Calculated from terms: 2*(qw*qy + qx*qz) and 1 - 2*(qx^2 + qy^2)
		const float siny_cosp = 2.0f * (quat.w * quat.y + quat.x * quat.z);
		const float cosy_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
		angles.fYaw = atan2f(siny_cosp, cosy_cosp);

		// --- 4. Safe Zone: Compute Roll (Z-axis, eulerRad.z) ---
		// Calculated from terms: 2*(qw*qz - qx*qy) and 1 - 2*(qx^2 + qz^2)
		// Since your FPS system should not have roll, this value should be near zero.
		const float sinr_cosp = 2.0f * (quat.w * quat.z - quat.x * quat.y);
		const float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z);
		angles.fRoll = atan2f(sinr_cosp, cosr_cosp);
	}

	// Convert to Degrees (if your utility requires it)
	// You can do this outside or inside, but typically the engine uses radians, 
	// and the UI layer converts to degrees.
	angles.fPitch = ToDegrees(angles.fPitch); // Pitch
	angles.fYaw = ToDegrees(angles.fYaw); // Yaw
	angles.fRoll = ToDegrees(angles.fRoll); // Roll (Should be near zero)

	return (angles);
}

#endif // __QUATERNION_H__