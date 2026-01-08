#ifndef __ENGINE_MATH_H__
#define __ENGINE_MATH_H__

#include "Matrix/Matrix4.h"
#include "Projection/PerspectiveProjection.h"
#include "Projection/OrthographicProjection.h"

static inline Matrix4 LookAtRH(const Vector3 v3Eye, const Vector3 v3Center, const Vector3 v3Up)
{
	// Forward vector: f = (v_center - v_eye) / ||v_center - v_eye||
	Vector3 f = Vector3_Sub(v3Center, v3Eye);
	f = Vector3_Normalized(f);

	// Right vector: s = (f x v_up) / ||f x v_up||
	// where 'x' denotes cross product
	Vector3 s = Vector3_Cross(f, v3Up);
	s = Vector3_Normalized(s);

	// Up vector: u = s x f
	// where 'x' denotes cross product
	Vector3 u = Vector3_Cross(s, f); // their cross product u will already have a length of 1.0, so we can skip the normalization for performance.

	// Construct the view matrix M:
	//
	//     [  s_x    s_y    s_z   -s.p ]
	// M = [  u_x    u_y    u_z   -u.p ]
	//     [ -f_x   -f_y   -f_z    f.p ]
	//     [   0      0      0      1  ]
	//
	// where s = (s_x, s_y, s_z) is the right vector
	//       u = (u_x, u_y, u_z) is the up vector
	//       f = (f_x, f_y, f_z) is the forward vector
	//       p is the camera position (v3Eye)
	//       '.' denotes dot product
	//
	// Matrix is indexed as M[column][row] for OpenGL compatibility
	Matrix4 ViewMatrix = S_Matrix4_Identity;
	ViewMatrix.cols[0].x = s.x; ViewMatrix.cols[1].x = s.y; ViewMatrix.cols[2].x = s.z;
	ViewMatrix.cols[0].y = u.x; ViewMatrix.cols[1].y = u.y; ViewMatrix.cols[2].y = u.z;
	ViewMatrix.cols[0].z = -f.x; ViewMatrix.cols[1].z = -f.y; ViewMatrix.cols[2].z = -f.z;

	// Translation components (negated dot products with camera position)
	ViewMatrix.cols[3].x = -Vector3_Dot(s, v3Eye);
	ViewMatrix.cols[3].y = -Vector3_Dot(u, v3Eye);
	ViewMatrix.cols[3].z = Vector3_Dot(f, v3Eye); // Note: f is already subbed, -(-f.p) becomes f.p

	return (ViewMatrix);
}

/**
 * Constructs a perspective projection matrix for 3D rendering.
 * Maps view space coordinates to normalized device coordinates (NDC).
 *
 * Uses a 45-degree field of view and clips geometry between near (0.1)
 * and far (1000.0) planes. Compatible with OpenGL's coordinate system.
 *
 * @return 4x4 perspective projection matrix in column-major order
 */
static inline Matrix4 PerspectiveRH(const SPersProjInfo persProj)
{
	// Perspective projection parameters
	float Fov = persProj.FOV;  // Field of view in degrees
	float HalfTanFOV = tanf(ToRadians(Fov) / 2.0f);
	float AspectRatio = persProj.Width / persProj.Height;
	float NearZ = persProj.zNear;    // Near clipping plane
	float FarZ = persProj.zFar;  // Far clipping plane
	float ZRange = FarZ - NearZ;

	// Construct the perspective projection matrix P:
	//
	//     [ 1/(t*a)     0           0              0     ]
	// P = [    0      1/t           0              0     ]
	//     [    0       0     -(f+n)/(f-n)   -2fn/(f-n)   ]
	//     [    0       0          -1              0      ]
	//
	// where t = tanf(fov/2)  (half tangent of field of view)
	//       a = aspect ratio (width/height)
	//       n = near clipping plane distance
	//       f = far clipping plane distance
	//
	// This maps view space to clip space with:
	//   X: [-aspect*tan(fov/2), aspect*tan(fov/2)] -> [-1, 1]
	//   Y: [-tan(fov/2), tan(fov/2)] -> [-1, 1]
	//   Z: [-n, -f] -> [-1, 1] (OpenGL depth range)
	Matrix4 ProjectionMatrix = S_Matrix4_Zero;

	// Column 0: X-axis scaling (accounts for aspect ratio)
	ProjectionMatrix.cols[0].x = 1.0f / (HalfTanFOV * AspectRatio);

	// Column 1: Y-axis scaling (based on FOV)
	ProjectionMatrix.cols[1].y = 1.0f / HalfTanFOV;

	// Column 2: Z-axis mapping and perspective division trigger
	ProjectionMatrix.cols[2].z = -(FarZ + NearZ) / (FarZ - NearZ);
	ProjectionMatrix.cols[2].w = -1.0f;	// Triggers perspective division (w = -z)

	// Column 3: Z translation component
	ProjectionMatrix.cols[3].z = -(2.0f * FarZ * NearZ) / (FarZ - NearZ);

	return (ProjectionMatrix);
}

/**
 * @brief Constructs an orthographic projection matrix (Right-Handed, Z depth from -1 to 1).
 *
 * This function creates a matrix that maps a rectangular viewing volume
 * [Left, Right], [Bottom, Top], [NearZ, FarZ] into the Normalized Device Coordinates (NDC) cube.
 * Parallel lines remain parallel (no perspective distortion).
 *
 * @param orthoProj Structure containing the orthographic viewing volume parameters.
 * @return Matrix4 The resulting 4x4 orthographic projection matrix.
 */
static inline Matrix4 OrthographicRH(const SOrthoProjInfo orthoProj)
{
	// Orthographic projection parameters
	float Left = orthoProj.Left;
	float Right = orthoProj.Right;
	float Bottom = orthoProj.Bottom;
	float Top = orthoProj.Top;
	float NearZ = orthoProj.zNear;
	float FarZ = orthoProj.zFar;

	// Pre-calculate differences and sums for matrix elements
	float R_minus_L = Right - Left;
	float T_minus_B = Top - Bottom;
	float F_minus_N = FarZ - NearZ;

	// Construct the orthographic projection matrix P:
	//
	// 			[ 2/(r-l)       0             0        -(r+l)/(r-l) ]
	// 		P = [    0       2/(t-b)          0        -(t+b)/(t-b) ]
	// 			[    0           0        -2/(f-n)     -(f+n)/(f-n) ]
	// 			[    0           0             0             1      ]
	//
	// where l = Left boundary of the view volume
	// 		 r = Right boundary of the view volume
	// 		 t = Top boundary of the view volume
	// 		 b = Bottom boundary of the view volume
	// 		 n = Near clipping plane distance
	// 		 f = Far clipping plane distance
	//
	// This maps the view volume to the clip space NDC cube [-1, 1] with:
	// 	 X: [l, r] -> [-1, 1]
	// 	 Y: [b, t] -> [-1, 1]
	// 	 Z: [-n, -f] -> [-1, 1] (OpenGL depth range)

	Matrix4 ProjectionMatrix = S_Matrix4_Identity;

	// Column 0: X-axis scaling (accounts for aspect ratio)
	ProjectionMatrix.cols[0].x = 2.0f / R_minus_L;
	ProjectionMatrix.cols[1].y = 2.0f / T_minus_B;

	ProjectionMatrix.cols[3].x = -(Right + Left) / R_minus_L;
	ProjectionMatrix.cols[3].y = -(Top + Bottom) / T_minus_B;

	ProjectionMatrix.cols[2].z = -2.0f / F_minus_N;
	ProjectionMatrix.cols[3].z = -(FarZ + NearZ) / F_minus_N;

	return (ProjectionMatrix);
}

static inline Vector3 GetSpherePos(float cx, float cy, float cz, float r, float phi, float theta)
{
	return (Vector3){
		cx + r * sinf(phi) * cosf(theta),
		cy + r * sinf(phi) * sinf(theta),
		cz + r * cosf(phi)
	};
}

#endif // __ENGINE_MATH_H__