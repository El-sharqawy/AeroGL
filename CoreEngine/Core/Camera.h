#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdbool.h>

#include "../Math/Vectors/Vector2.h"
#include "../Math/Vectors/Vector3.h"
#include "../Math/Matrix/Matrix4.h"
#include "../Math/EngineMath.h"

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


bool InitializeCamera(GLCamera* ppCamera, float Width, float Height);
void DestroyCamera(GLCamera* pCamera);

Matrix4 GetViewMatrix(GLCamera pCamera);
Matrix4 GetProjectionMatrix(GLCamera pCamera);
Matrix4 GetViewProjectionMatrix(GLCamera pCamera);
Matrix4 GetViewBillboardMatrix(GLCamera pCamera);

void UpdateProjections(GLCamera pCamera);

void ProcessCameraKeboardInput(GLCamera pCamera, ECameraDirections cameraDir, float deltaTime);
void ProcessCameraMouse(GLCamera pCamera);
void ProcessCameraZoom(GLCamera pCamera);

void UpdateCameraVectors(GLCamera pCamera);

void UpdateCameraDeminsions(GLCamera pCamera, float width, float height);

#endif // __CAMERA_H__