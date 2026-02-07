#include "Camera.h"
#include "../Stdafx.h"
#include "../Math/Projection/OrthographicProjection.h"
#include "../Math/Quaternion/Quaternion.h"
#include "../Buffers/UniformBufferObject.h"

typedef struct SGLCamera
{
	// View port Width
	float Width;

	// View Port Height
	float Height;

	// The world coordinates (X, Y, Z) of the camera's current location.
	Vector3 v3Position;

	// The camera's local Forward vector (F). Where the camera is currently looking. 
	// Used for calculating the View Matrix and for forward/backward movement (W/S).
	Vector3 v3Front;

	// The camera's local Right vector (R). Perpendicular to m_v3Front and m_v3Up. 
	// Used for constructing the View Matrix and for strafing (A/D) movement.
	Vector3 v3Right;

	// The camera's local Up vector (U). Used for constructing the View Matrix 
	// and ensuring the camera remains upright (no roll).
	Vector3 v3Up;

	// Camera View Matrix
	Matrix4 ViewMatrix;

	// Camera Projection Matrix
	Matrix4 ProjectionMatrix;

	// Camera View-Projection Matrix
	Matrix4 ViewProjectionMatrix;

	// matrix to ensure that billboards always face the camera, regardless of its orientation.
	Matrix4 ViewMatrixBillboard;

	// Stores the mouse cursor's last known X and Y screen coordinates.
	// Used to calculate the mouse delta for rotation (Yaw and Pitch).
	Vector2 v2MousePos;

	// The field-of-view (FOV) value used for zooming in/out (often controlled by the scroll wheel).
	float CameraZoom;

	// The base speed (units per second) at which the camera moves (e.g., 5.0f).
	float CameraSpeed;

	// The accumulated horizontal rotation angle (in degrees). Rotation around the local Up axis.
	float Yaw;

	// The accumulated vertical rotation angle (in degrees). Rotation around the local Right axis.
	float Pitch;

	// The accumulated roll rotation angle (in degrees). Rotation around the local Front axis.
	float Roll;

	// The multiplier applied to mouse delta to control the speed of rotation (e.g., 0.1f).
	float Sensitivity;

	// The camera Type (Perspective - Orthographic)
	ECameraType CameraType;

	// Camera Perspective Projection Data
	SPersProjInfo PerspectiveProjection;

	// Camera Orthographic Projection Data
	SOrthoProjInfo OrthographicProjection;
	
	// Accumulates all Yaw and Pitch rotations into a single quaternion.
	SQuaternion OrientationQuaternion;

	// Camera Matrix State Flags
	// Set to true when position/rotation changes
	bool bViewDirty;

	// Set to true when zoom/window size changes
	bool bProjectionDirty;

	// Set to true when either view or projection is dirty
	bool bViewProjDirty;

	// Set to true when view matrix changes
	bool bBillboardDirty;

	// Use Quaternion to calculate camera rotations
	bool bUseQuaternion;

	// Camera Metrices Uniform Buffer Object
	UniformBufferObject cameraUBO;

	// metrices Data
	SCameraUBO cameraSUBO; // struct Data
} SGLCamera;

bool InitializeCamera(GLCamera *ppCamera, float Width, float Height)
{
	if (ppCamera == NULL)
	{
		syserr("ppCamera is NULL (invalid address)");
		return false;
	}

	// make sure it's all elements set to zero bytes
	*ppCamera = engine_new_zero(SGLCamera, 1, MEM_TAG_ENGINE);

	GLCamera pCamera = *ppCamera;

	if (pCamera == (void*)-1 || pCamera == NULL)
	{
		syserr("Memory allocation returned an invalid address!");
		return (false);
	}

	UpdateCameraDeminsions(pCamera, Width, Height);

	// Initial orientation
	pCamera->v3Position = Vector3_Create(0.0f, 0.0f, -5.0f); // Start 5 units back
	pCamera->v3Front = s_v3WorldView;

	// Calculate initial Right vector: R = Front x WorldUp
	pCamera->v3Right = Vector3_Cross(pCamera->v3Front, s_v3WorldUp);
	pCamera->v3Right = Vector3_Normalized(pCamera->v3Right);

	pCamera->v3Up = Vector3_Cross(pCamera->v3Right, pCamera->v3Front);
	pCamera->v3Up = Vector3_Normalized(pCamera->v3Up);

	pCamera->CameraZoom = 45.0f;
	pCamera->CameraSpeed = 50.0f;

	pCamera->v2MousePos = (Vector2){ 0.0f, 0.0f };

	pCamera->Pitch = 00.0f;	// Rotation around X - Axis
	pCamera->Yaw = -120.0f;		// Rotation around Y - Axis
	pCamera->Roll = 0.0f;		// Rotation around Z - Axis

	pCamera->Sensitivity = 0.1f;

	pCamera->CameraType = CAMERA_PERSPECTIVE;//CAMERA_ORTHOGRAPHIC;

	pCamera->bUseQuaternion = true;

	Vector3 eulerXYZ = { ToRadians(pCamera->Pitch), ToRadians(pCamera->Yaw), ToRadians(pCamera->Roll) };

	pCamera->OrientationQuaternion = Quaternion_FromEulerZYX(eulerXYZ, false);
	Vector3 vForward = { 0.0f, 0.0f, -1.0f };
	pCamera->v3Front = Quaternion_RotateVec(pCamera->OrientationQuaternion, vForward);
	pCamera->v3Front = Vector3_Normalized(pCamera->v3Front);

	pCamera->v3Right = Vector3_Normalized(Vector3_Cross(pCamera->v3Front, s_v3WorldUp));
	pCamera->v3Up = Vector3_Normalized(Vector3_Cross(pCamera->v3Right, pCamera->v3Front));

	if (!InitializeUniformBufferObject(&pCamera->cameraUBO, sizeof(SCameraUBO), UBO_BP_CAMERA, "Camera UBO"))
	{
		syserr("Failed To Create Camera UBO");
	}

	pCamera->cameraSUBO = (SCameraUBO){ S_Matrix4_Identity, S_Matrix4_Identity, S_Matrix4_Identity, S_Matrix4_Identity };

	UpdateProjections(pCamera);

	return (pCamera != NULL);
}

void UpdateProjections(GLCamera pCamera)
{
	// Configure perspective projection parameters
	pCamera->PerspectiveProjection.FOV = pCamera->CameraZoom;							// Field of view in degrees
	pCamera->PerspectiveProjection.Width = pCamera->Width;								// Viewport width
	pCamera->PerspectiveProjection.Height = pCamera->Height;							// Viewport height
	pCamera->PerspectiveProjection.zNear = 0.1f;										// Near clipping plane
	pCamera->PerspectiveProjection.zFar = 10000.0f;										// Far clipping plane

	// Configure orthographic projection parameters
	const GLfloat fAspetRatio = pCamera->Width / pCamera->Height;
	pCamera->OrthographicProjection.Left = -pCamera->CameraZoom * fAspetRatio;			/** The left plane of the orthogonal frustum */
	pCamera->OrthographicProjection.Right = pCamera->CameraZoom * fAspetRatio;			/** The right plane of the orthogonal frustum */
	pCamera->OrthographicProjection.Bottom = -pCamera->CameraZoom;						/** The bottom plane of the orthogonal frustum */
	pCamera->OrthographicProjection.Top = pCamera->CameraZoom;							/** The top plane of the orthogonal frustum */
	pCamera->OrthographicProjection.zNear = 0.1f;										/** The near clipping plane distance */
	pCamera->OrthographicProjection.zFar = 10000.0f;									/** The far clipping plane distance */
	pCamera->OrthographicProjection.Width = pCamera->Width;								/** The width of the orthogonal view in world-space units */
	pCamera->OrthographicProjection.Height = pCamera->Height;							/** The height of the orthogonal view in world-space units */

	pCamera->bProjectionDirty = true;			// Set to true when zoom/window size changes
	pCamera->bViewProjDirty = true;			// Set to true when either view or projection is dirty
}

void DestroyCamera(GLCamera* ppCamera)
{
	if (!ppCamera || !*ppCamera)
	{
		return;
	}

	GLCamera pCamera = *ppCamera;

	DestroyUniformBufferObject(&pCamera->cameraUBO);

	tracked_free(pCamera);

	*ppCamera = NULL;
}

Matrix4 GetViewMatrix(GLCamera pCamera)
{
	if (pCamera->bViewDirty)
	{
		pCamera->ViewMatrix = LookAtRH(pCamera->v3Position, Vector3_Add(pCamera->v3Position, pCamera->v3Front), pCamera->v3Up);
		pCamera->bViewDirty = false;
	}

	return (pCamera->ViewMatrix);
}

Matrix4 GetProjectionMatrix(GLCamera pCamera)
{
	if (pCamera->bProjectionDirty)
	{
		if (pCamera->CameraType == CAMERA_PERSPECTIVE)
		{
			pCamera->ProjectionMatrix = PerspectiveRH(pCamera->PerspectiveProjection);
		}
		else if (pCamera->CameraType == CAMERA_ORTHOGRAPHIC)
		{
			pCamera->ProjectionMatrix = OrthographicRH(pCamera->OrthographicProjection);
		}

		pCamera->bProjectionDirty = false;
	}
	return (pCamera->ProjectionMatrix);
}

Matrix4 GetViewProjectionMatrix(GLCamera pCamera)
{
	// Ensure the parents are up to date first
	// Get the latest parents (these handle their own dirty flags internally)
	Matrix4 Projection = GetProjectionMatrix(pCamera);
	Matrix4 View = GetViewMatrix(pCamera);

	// If we were manually told we are dirty, or if the parents just updated
	// Note: This requires the parent getters to return if they were dirty.
	// A simpler way is just to check the flags BEFORE calling the parent getters.

	if (pCamera->bViewProjDirty)
	{
		pCamera->ViewProjectionMatrix = Matrix4_Mul(Projection, View);
		pCamera->bViewProjDirty = false;
	}

	return (pCamera->ViewProjectionMatrix);
}

Matrix4 GetViewBillboardMatrix(GLCamera pCamera)
{
	if (pCamera->bBillboardDirty)
	{
		pCamera->ViewMatrixBillboard = GetViewMatrix(pCamera);

		// Zero out the translation part (4th column)
		pCamera->ViewMatrixBillboard.cols[3].x = 0.0f;
		pCamera->ViewMatrixBillboard.cols[3].y = 0.0f;
		pCamera->ViewMatrixBillboard.cols[3].z = 0.0f;
		// Ensure the bottom right component is 1.0 (W_w)
		pCamera->ViewMatrixBillboard.cols[3].w = 1.0f;

		// Transpose the upper-left 3x3 rotation part to invert it
		for (int i = 0; i < 3; ++i)
		{
			for (int j = i + 1; j < 3; ++j)
			{
				// We need a temp variable to perform a swap
				float temp = pCamera->ViewMatrixBillboard.cols[i].v4[j];
				// Swap the row/column indices
				pCamera->ViewMatrixBillboard.cols[i].v4[j] = pCamera->ViewMatrixBillboard.cols[j].v4[i];
				pCamera->ViewMatrixBillboard.cols[j].v4[i] = temp;
			}
		}

		// Mark as clean so we don't do this work again until the camera moves
		pCamera->bBillboardDirty = false;
	}

	return (pCamera->ViewMatrixBillboard);
}

void ProcessCameraKeboardInput(GLCamera pCamera, ECameraDirections cameraDir, float deltaTime)
{
	GLfloat fVelocity = pCamera->CameraSpeed * deltaTime;
	
	switch (cameraDir)
	{
		case DIRECTION_FORWARD:
			pCamera->v3Position = Vector3_Add(pCamera->v3Position, Vector3_Muls(pCamera->v3Front, fVelocity));
			break;

		case DIRECTION_BACKWARD:
			pCamera->v3Position = Vector3_Sub(pCamera->v3Position, Vector3_Muls(pCamera->v3Front, fVelocity));
			break;

		case DIRECTION_RIGHT:
			pCamera->v3Position = Vector3_Add(pCamera->v3Position, Vector3_Muls(pCamera->v3Right, fVelocity));
			break;

		case DIRECTION_LEFT:
			pCamera->v3Position = Vector3_Sub(pCamera->v3Position, Vector3_Muls(pCamera->v3Right, fVelocity));
			break;
	}

	pCamera->bViewDirty = true;     // View needs update
	pCamera->bBillboardDirty = true;
	pCamera->bViewProjDirty = true;
}

void ProcessCameraMouse(GLCamera pCamera)
{
	// Accumulate the rotation changes
	if (pCamera->bUseQuaternion)
	{
		// Quaternion-based rotation accumulation
		GLfloat fDeltaYaw = -GetInput()->v2MouseDelta.x * pCamera->Sensitivity;
		GLfloat fDeltaPitch = GetInput()->v2MouseDelta.y * pCamera->Sensitivity;

		// Compute right axis from current orientation
		Vector3 localRight = Quaternion_Rotate(pCamera->OrientationQuaternion, s_v3WorldRight);

		// Create rotation quaternions
		SQuaternion pitchQuat = Quaternion_FromAxisAngleV(localRight, fDeltaPitch, true);
		SQuaternion yawQuat = Quaternion_FromAxisAngleV(s_v3WorldUp, fDeltaYaw, true);

		// Accumulate orientation
		SQuaternion yawMulPitch = Quaternion_MultiplySIMD(yawQuat, pitchQuat);
		pCamera->OrientationQuaternion = Quaternion_MultiplySIMD(yawMulPitch, pCamera->OrientationQuaternion);

		pCamera->OrientationQuaternion = Quaternion_Normalize(pCamera->OrientationQuaternion);
	}
	else
	{
		// Use the pre-calculated delta from our Input System
		pCamera->Yaw -= GetInput()->v2MouseDelta.x * pCamera->Sensitivity;
		pCamera->Pitch += GetInput()->v2MouseDelta.y * pCamera->Sensitivity;

		// Clamp the pitch angle to [-89.0f, 89.0f] degrees to prevent the camera from 
		// flipping over (looking directly above 90 or below -90 degrees).
		pCamera->Pitch = clampf(pCamera->Pitch, -89.0f, 89.0f); // Prevent gimbal lock
	}

	UpdateCameraVectors(pCamera);
}

void ProcessCameraZoom(GLCamera pCamera)
{
	// 1. Adjust the zoom level (clamped to keep it sensible)
	pCamera->CameraZoom -= GetInput()->mouseScroll;
	pCamera->CameraZoom = clampf(pCamera->CameraZoom, 1.0f, 90.0f);

	// 2. Sync the change to the Projection data
	if (pCamera->CameraType == CAMERA_PERSPECTIVE)
	{
		pCamera->PerspectiveProjection.FOV = pCamera->CameraZoom;
	}
	else if (pCamera->CameraType == CAMERA_ORTHOGRAPHIC)
	{
		const GLfloat fAspetRatio = pCamera->OrthographicProjection.Width / pCamera->OrthographicProjection.Height;
		pCamera->OrthographicProjection.Left = -pCamera->CameraZoom * fAspetRatio;			/** The left plane of the orthogonal frustum */
		pCamera->OrthographicProjection.Right = pCamera->CameraZoom * fAspetRatio;			/** The right plane of the orthogonal frustum */
		pCamera->OrthographicProjection.Bottom = -pCamera->CameraZoom;						/** The bottom plane of the orthogonal frustum */
		pCamera->OrthographicProjection.Top = pCamera->CameraZoom;							/** The top plane of the orthogonal frustum */
	}

	// 3. Trip the flags!
	pCamera->bProjectionDirty = true;
	pCamera->bViewProjDirty = true;
}

void UpdateCameraVectors(GLCamera pCamera)
{
	if (pCamera->bUseQuaternion == true)
	{
		// Rotate canonical axes
		pCamera->v3Front = Quaternion_Rotate(pCamera->OrientationQuaternion, s_v3WorldView);	// Forward vector
		pCamera->v3Right = Quaternion_Rotate(pCamera->OrientationQuaternion, s_v3WorldRight);	// Right vector
		pCamera->v3Up = Quaternion_Rotate(pCamera->OrientationQuaternion, s_v3WorldUp);			// Up vector

		// Normalize (Optional, could save math operations!)
		// pCamera->v3Front = Vector3_Normalized(pCamera->v3Front);
		// pCamera->v3Right = Vector3_Normalized(pCamera->v3Right);
		// pCamera->v3Up = Vector3_Normalized(pCamera->v3Up);
	}
	else
	{
		float PitchRad = ToRadians(pCamera->Pitch);
		float YawRad = ToRadians(pCamera->Yaw);

		// calculate the new Front vector (derived from spherical coordinates)
		// X component: cos(Yaw) * cos(Pitch)
		pCamera->v3Front.x = cosf(YawRad) * cosf(PitchRad);

		// Y component: sin(Pitch) (Vertical direction)
		pCamera->v3Front.y = sinf(PitchRad);

		// Z component: sin(Yaw) * cos(Pitch)
		pCamera->v3Front.z = sinf(YawRad) * cosf(PitchRad);

		// Normalize the vector to maintain unit length
		pCamera->v3Front = Vector3_Normalized(pCamera->v3Front);

		// also re-calculate the Right vector (s) using the Cross Product of Front and World Up.
		// This ensures the Right vector is perpendicular to both.
		// Calculate initial Right vector: R = Front x WorldUp
		pCamera->v3Right = Vector3_Cross(pCamera->v3Front, s_v3WorldUp);
		pCamera->v3Right = Vector3_Normalized(pCamera->v3Right);

		pCamera->v3Up = Vector3_Cross(pCamera->v3Right, pCamera->v3Front);
		pCamera->v3Up = Vector3_Normalized(pCamera->v3Up);
	}

	pCamera->bViewDirty = true;     // View needs update
	pCamera->bBillboardDirty = true;
	pCamera->bViewProjDirty = true; // VP needs update because View changed
}

void UpdateCameraDeminsions(GLCamera pCamera, float width, float height)
{
	pCamera->Width = width;
	pCamera->Height = height;

	UpdateProjections(pCamera);
}

void UpdateCamera(GLCamera pCamera)
{
	if (!pCamera)
	{
		return;
	}

	UpdateUniformBufferObject(pCamera);
}

void UpdateUniformBufferObject(GLCamera pCamera)
{
	bool needsUpload = false;

	// Note: GetViewProjectionMatrix() internally updates the cached matrices 
	// and clears the dirty flags!
	if (pCamera->bViewDirty)
	{
		// pCamera->cameraSUBO.viewMat = GetViewMatrix(pCamera);
		needsUpload = true;
	}

	if (pCamera->bProjectionDirty)
	{
		// pCamera->cameraSUBO.projectionMat = GetProjectionMatrix(pCamera);
		needsUpload = true;
	}

	if (pCamera->bViewProjDirty)
	{
		pCamera->cameraSUBO.viewProjectionMat = GetViewProjectionMatrix(pCamera);
		needsUpload = true;
	}

	if (pCamera->bBillboardDirty)
	{
		pCamera->cameraSUBO.viewBillBoard = GetViewBillboardMatrix(pCamera);
		needsUpload = true;
	}

	if (needsUpload)
	{
		if (pCamera->cameraUBO->isPersistent)
		{
			// COPY data to existing GPU pointer (never reassign!)
			memcpy(pCamera->cameraUBO->pBufferData, &pCamera->cameraSUBO, sizeof(SCameraUBO));

			// Optional: Flush range (modern drivers auto with COHERENT_BIT)
			// glFlushMappedNamedBufferRange(pCamera->cameraUBO->bufferID, 0, sizeof(SCameraUBO));
		}
		else
		{
			UniformBufferObject_Update(pCamera->cameraUBO, &pCamera->cameraSUBO, sizeof(SCameraUBO), 0, false);
		}
	}
}
