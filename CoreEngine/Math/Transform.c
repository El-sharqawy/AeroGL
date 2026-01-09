#include "Transform.h"

/**
 * @brief Sets the transform's position using individual coordinates.
 *
 * Updates the world-space position of the transform. This completely replaces
 * any previous position.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param x [in] The X coordinate (right/left).
 * @param y [in] The Y coordinate (up/down).
 * @param z [in] The Z coordinate (forward/back).
 */
void TransformSetPosition(Transform pTransform, float x, float y, float z)
{
	pTransform->v3Position = Vector3D(x ,y , z );
}

/**
 * @brief Sets the transform's position using a Vector3.
 *
 * Updates the world-space position of the transform. This completely replaces
 * any previous position.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param v3Pos [in] The new position vector.
 */
void TransformSetPositionV(Transform pTransform, const Vector3 v3Pos)
{
	pTransform->v3Position = v3Pos;
}

/**
 * @brief Sets the transform's scale using individual components.
 *
 * Updates the local scale of the transform. Values greater than 1.0 make the
 * object larger, values less than 1.0 make it smaller. This completely replaces
 * any previous scale.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param sx [in] Scale factor along the X-axis.
 * @param sy [in] Scale factor along the Y-axis.
 * @param sz [in] Scale factor along the Z-axis.
 *
 * @note Use 1.0 for each axis to maintain original size.
 */
void TransformSetScale(Transform pTransform, float sx, float sy, float sz)
{
	pTransform->v3Scale = Vector3D(sx, sy, sz);
}

/**
 * @brief Sets the transform's scale using a Vector3.
 *
 * Updates the local scale of the transform. Values greater than 1.0 make the
 * object larger, values less than 1.0 make it smaller. This completely replaces
 * any previous scale.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param v3Scale [in] The new scale vector.
 *
 * @note Use Vector3D(1, 1, 1) to maintain original size.
 */
void TransformSetScaleV(Transform pTransform, const Vector3 v3Scale)
{
	pTransform->v3Scale = v3Scale;
}

/**
 * @brief Sets the transform's rotation from Euler angles (ZYX order).
 *
 * Replaces the current rotation with a new orientation specified by Euler angles.
 * The rotation order is ZYX (Yaw-Pitch-Roll), which is standard for game characters
 * and vehicles. This completely replaces any previous rotation.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param v3Euler [in] Euler angles: x = pitch (X-axis), y = yaw (Y-axis), z = roll (Z-axis).
 * @param bRadian [in] If true, angles are in radians; if false, degrees.
 *
 * @note For most game objects, use degrees (bRadian = false) for easier tuning.
 */
void TransformSetRotationEuler(Transform pTransform, Vector3 v3Euler, bool bRadian)
{
	pTransform->qOrientation = Quaternion_FromEulerZYX(v3Euler, bRadian);
}

/**
 * @brief Sets the transform's rotation from a quaternion directly.
 *
 * Replaces the current rotation with a pre-computed quaternion. Use this when
 * you already have a quaternion from interpolation, animation, or other sources.
 * This completely replaces any previous rotation.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param qRotation [in] The new rotation quaternion.
 *
 * @note The quaternion should be normalized (unit length) for correct results.
 */
void TransformSetRotationQuat(Transform pTransform, SQuaternion qRotation)
{
	pTransform->qOrientation = qRotation;
}

/**
 * @brief Sets the transform's rotation around a principal axis (X, Y, or Z).
 *
 * Replaces the current rotation with a single-axis rotation around X, Y, or Z.
 * Useful for setting initial orientations like "facing forward" or "upright".
 * This completely replaces any previous rotation.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param axis [in] The axis to rotate around (AXIS_X, AXIS_Y, or AXIS_Z).
 * @param fAngle [in] The rotation angle.
 * @param bRadian [in] If true, fAngle is in radians; if false, degrees.
 *
 * @note Common use: TransformSetRotationAroundAxis(&obj, AXIS_Y, 90, false) to face right.
 */
void TransformSetRotationAroundAxis(Transform pTransform, EAxis axis, float fAngle, bool bRadian)
{
	pTransform->qOrientation = Quaternion_FromRotation(axis, fAngle, bRadian);
}

/**
 * @brief Sets the transform's rotation around an arbitrary axis.
 *
 * Replaces the current rotation with a rotation around any custom axis vector.
 * The axis is automatically normalized before use. Useful for setting rotations
 * along diagonal or custom axes. This completely replaces any previous rotation.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param v3Axis [in] The rotation axis (will be normalized internally, must be non-zero).
 * @param fAngle [in] The rotation angle.
 * @param bRadian [in] If true, fAngle is in radians; if false, degrees.
 *
 * @note Example: Set a door hinge rotation around a tilted axis like Vector3D(0, 1, 0.1f).
 */
void TransformSetRotation(Transform pTransform, Vector3 v3Axis, float fAngle, bool bRadian)
{
	pTransform->qOrientation = Quaternion_MakeRotation(v3Axis, fAngle, bRadian);
}

/**
 * @brief Rotates the transform incrementally around a principal axis (X, Y, or Z).
 *
 * Applies an additional rotation around X, Y, or Z to the existing orientation.
 * This adds to the current rotation rather than replacing it. Use this for
 * continuous rotations in game loops (e.g., spinning objects, camera controls).
 *
 * @param pTransform [in/out] The transform to modify.
 * @param axis [in] The axis to rotate around (AXIS_X, AXIS_Y, or AXIS_Z).
 * @param fAngle [in] The rotation angle to add.
 * @param bRadian [in] If true, fAngle is in radians; if false, degrees.
 *
 * @note Example: TransformRotateAroundAxis(&player, AXIS_Y, mouseX * sensitivity, false)
 *       for FPS camera yaw.
 */
void TransformRotateAroundAxis(Transform pTransform, EAxis axis, float fAngle, bool bRadian)
{
	pTransform->qOrientation = Quaternion_RotateAroundAxis(pTransform->qOrientation, axis, fAngle, bRadian);
}

/**
 * @brief Rotates the transform incrementally around an arbitrary axis (free rotation).
 *
 * Applies an additional rotation around any custom axis to the existing orientation.
 * This enables gimbal-lock-free rotation in any direction. The axis is automatically
 * normalized before use. This adds to the current rotation rather than replacing it.
 *
 * @param pTransform [in/out] The transform to modify.
 * @param v3Axis [in] The axis to rotate around (will be normalized internally, must be non-zero).
 * @param fAngle [in] The rotation angle to add.
 * @param bRadian [in] If true, fAngle is in radians; if false, degrees.
 *
 * @note This is the "free rotation" function - use for propellers on tilted axes,
 *       turrets tracking targets, or orbital camera motion.
 * @note Example: TransformRotateAxis(&propeller, Vector3D(1, 0.5f, 0), 360 * deltaTime, false)
 *       for a propeller spinning on a tilted axis.
 */
void TransformRotateAxis(Transform pTransform, Vector3 v3Axis, float fAngle, bool bRadian)
{
	pTransform->qOrientation = Quaternion_RotateAxis(pTransform->qOrientation, v3Axis, fAngle, bRadian);
}

/**
 * @brief Calculates the model matrix for rendering (Translation * Rotation * Scale).
 *
 * Computes the complete 4x4 transformation matrix that converts local-space coordinates
 * to world-space. This matrix combines position, rotation, and scale in the correct
 * order for rendering. Pass this matrix to your shader's model/world uniform.
 *
 * @param pTransform [in] The transform to calculate the matrix from.
 * @return Matrix4 The model matrix in column-major format (OpenGL-compatible).
 *
 * @note The returned matrix applies transformations in this order:
 *       1. Translate (positions in world space)
 *       2. Rotate (orients the object)
 *       3. Scale (makes object bigger/smaller)
 * @note Usage: Matrix4 model = TransformGetMatrix(&entity.transform);
 *       ShaderSetMatrix4(shader, "u_Model", &model);
 */
Matrix4 TransformGetMatrix(Transform pTransform)
{
	Matrix4 rotationMat = Quaternion_ToMatrix4(pTransform->qOrientation); // Get Rotation

	// Apply TRS Calculations
	Matrix4 mat = S_Matrix4_Identity;
	mat = Matrix4_Translate(mat, pTransform->v3Position); // Get Translation
	mat = Matrix4_Mul(mat, rotationMat);
	mat = Matrix4_Scale(mat, pTransform->v3Scale);

	return (mat);
}
