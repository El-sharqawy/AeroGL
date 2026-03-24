#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdbool.h>

#include "../Math/Vectors/Vector2.h"
#include "../Math/Vectors/Vector3.h"
#include "../Math/Matrix/Matrix4.h"
#include "../Math/EngineMath.h"

typedef struct SCameraUBO
{
	Matrix4 viewMat;
	Matrix4 projectionMat;
	Matrix4 viewProjectionMat;
	Matrix4 viewBillBoard;
} SCameraUBO;

typedef struct SGLCamera* GLCamera;

// The global World Up vector (U_World), typically (0, 1, 0). 
// Used as the fixed axis for Yaw (horizontal) rotation.
static const Vector3 s_v3WorldUp = { 0.0f, 1.0f, 0.0f };	// View along Y Axis (0,1,0)

// The global World Up vector (R_World), typically (1, 0, 0). 
// Used as the fixed axis for Roll (vertical) rotation.
static const Vector3 s_v3WorldRight = { 1.0f, 0.0f, 0.0f };	// View along X Axis (1,0,0)

// The initial or temporary view vector used during the quaternion rotation calculations 
// before being assigned to m_v3Front. (Often initialized to (0, 0, 1) or (0, 0, -1)).
static const Vector3 s_v3WorldView = { 0.0f, 0.0f, -1.0f }; // View along Z Axis (0,0,-1)

static const Vector3 s_v3WorldPosition = { 0.0f, 0.0f, 5.0f }; // View along Z Axis (0,0,5)

typedef enum ECameraDirections
{
	DIRECTION_FORWARD,
	DIRECTION_RIGHT,
	DIRECTION_BACKWARD,
	DIRECTION_LEFT
} ECameraDirections;

typedef enum ECameraType
{
	CAMERA_PERSPECTIVE,
	CAMERA_ORTHOGRAPHIC,
} ECameraType;


bool Camera_Initialize(GLCamera* ppCamera, float Width, float Height);
void Camera_Destroy(GLCamera* pCamera);

Matrix4 Camera_GetViewMatrix(GLCamera pCamera);
Matrix4 Camera_GetProjectionMatrix(GLCamera pCamera);
Matrix4 Camera_GetViewProjectionMatrix(GLCamera pCamera);
Matrix4 Camera_GetViewBillboardMatrix(GLCamera pCamera);

void Camera_UpdateProjections(GLCamera pCamera);

void Camera_ProcessCameraKeboardInput(GLCamera pCamera, ECameraDirections cameraDir, float deltaTime);
void Camera_ProcessCameraMouse(GLCamera pCamera);
void Camera_ProcessCameraZoom(GLCamera pCamera);

void Camera_UpdateCameraVectors(GLCamera pCamera);
void Camera_UpdateCameraDeminsions(GLCamera pCamera, float width, float height);
void Camera_UpdateCamera(GLCamera pCamera);
void Camera_UpdateUniformBufferObject(GLCamera pCamera);

#endif // __CAMERA_H__