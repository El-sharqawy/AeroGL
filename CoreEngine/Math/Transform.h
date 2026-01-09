#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "Vectors/Vector3.h"
#include "Quaternion/Quaternion.h"
#include "Matrix/Matrix4.h"

typedef struct STransform
{
	Vector3 v3Position;			// Position
	Vector3 v3Scale;			// Scale
	SQuaternion qOrientation;	// Rotation
} STransform;

typedef struct STransform* Transform;


static inline STransform TransformInit()
{
	STransform transform;
	transform.v3Position = Vector3F(0.0f);
	transform.v3Scale = Vector3F(1.0f);
	transform.qOrientation = S_Quaternion_Identity;

	return (transform);
}

static inline STransform TransformInitP(float x, float y, float z)
{
	STransform transform;
	transform.v3Position = Vector3D(x, y, z);
	transform.v3Scale = Vector3F(1.0f);
	transform.qOrientation = S_Quaternion_Identity;

	return (transform);
}

// ===== Position + Rotation (Euler Angles) =====
static inline STransform TransformInitPR(float x, float y, float z, float pitch, float yaw, float roll, bool bDegreesInput)
{
	STransform transform;
	transform.v3Position = Vector3D(x, y, z);
	transform.v3Scale = Vector3F(1.0f);

	Vector3 eulerAngles = Vector3D(pitch, yaw, roll);
	transform.qOrientation = Quaternion_FromEulerZYX(eulerAngles, bDegreesInput);

	return (transform);
}

static inline STransform TransformInitPS(float x, float y, float z, float sx, float sy, float sz)
{
	STransform transform;
	transform.v3Position = Vector3D(x, y, z);
	transform.v3Scale = Vector3D(sx, sy, sz);
	transform.qOrientation = S_Quaternion_Identity;

	return (transform);
}

static inline STransform TransformInitPSR(float x, float y, float z, float sx, float sy, float sz, float pitch, float yaw, float roll, bool bDegreesInput)
{
	STransform transform;
	transform.v3Position = Vector3D(x, y, z);
	transform.v3Scale = Vector3D(sx, sy, sz);

	Vector3 eulerAngles = Vector3D(pitch, yaw, roll);
	transform.qOrientation = Quaternion_FromEulerZYX(eulerAngles, bDegreesInput);

	return (transform);
}

static inline STransform TransformInitVP(const Vector3 v3Pos)
{
	STransform transform;
	transform.v3Position = v3Pos;
	transform.v3Scale = Vector3F(1.0f);
	transform.qOrientation = S_Quaternion_Identity;

	return (transform);
}

static inline STransform TransformInitVPR(const Vector3 v3Pos, const Vector3 v3RotationAngles, bool bDegreesInput)
{
	STransform transform;
	transform.v3Position = v3Pos;
	transform.v3Scale = Vector3F(1.0f);
	transform.qOrientation = Quaternion_FromEulerZYX(v3RotationAngles, bDegreesInput);

	return (transform);
}

static inline STransform TransformInitVPS(const Vector3 v3Pos, const Vector3 v3Scale)
{
	STransform transform;
	transform.v3Position = v3Pos;
	transform.v3Scale = v3Scale;
	transform.qOrientation = S_Quaternion_Identity;

	return (transform);
}

static inline STransform TransformInitVPSR(const Vector3 v3Pos, const Vector3 v3Scale, const Vector3 v3RotationAngles, bool bDegreesInput)
{
	STransform transform;
	transform.v3Position = v3Pos;
	transform.v3Scale = v3Scale;
	transform.qOrientation = Quaternion_FromEulerZYX(v3RotationAngles, bDegreesInput);

	return (transform);
}

// Position
void TransformSetPosition(Transform pTransform, float x, float y, float z);
void TransformSetPositionV(Transform pTransform, const Vector3 v3Pos);

// Scale
void TransformSetScale(Transform pTransform, float sx, float sy, float sz);
void TransformSetScaleV(Transform pTransform, const Vector3 v3Scale);

// Rotation
void TransformSetRotationQuat(Transform pTransform, SQuaternion qRotation);
void TransformSetRotationEuler(Transform pTransform, Vector3 v3Euler, bool bRadian);
void TransformSetRotationAroundAxis(Transform pTransform, EAxis axis, float fAngle, bool bRadian);
void TransformSetRotation(Transform pTransform, Vector3 v3Axis, float fAngle, bool bRadian);
void TransformRotateAroundAxis(Transform pTransform, EAxis axis, float fAngle, bool bRadian);
void TransformRotateAxis(Transform pTransform, Vector3 v3Axis, float fAngle, bool bRadian);

// Calculate Matrix
Matrix4 TransformGetMatrix(Transform pTransform);

#endif // __TRANSFORM_H__