#ifndef __PERSPECTIVE_PROJECTION_H__
#define __PERSPECTIVE_PROJECTION_H__

/**
 * Structure to hold parameters for a perspective projection matrix
 *
 * This structure contains all the necessary information to create a perspective projection matrix
 * for rendering 3D scenes. The perspective projection simulates depth, making objects appear smaller
 * as they get farther from the camera, similar to how the human eye perceives depth.
 *
 * Members:
 *   - FOV: Field of View, defines the vertical angle (in degrees or radians) for the camera's view.
 *   - Width: The width of the viewport (screen/window) for rendering.
 *   - Height: The height of the viewport for rendering.
 *   - zNear: The near clipping plane distance. Objects closer than this will not be rendered.
 *   - zFar: The far clipping plane distance. Objects farther than this will not be rendered.
 */
typedef struct SPersProjInfo
{
	float FOV;      /* Vertical Field of View in degrees or radians */
	float Width;    /* Width of the viewport (screen/window) */
	float Height;   /* Height of the viewport */
	float zNear;    /* Near clipping plane distance */
	float zFar;     /* Far clipping plane distance */
} SPersProjInfo;

#endif // __PERSPECTIVE_PROJECTION_H__