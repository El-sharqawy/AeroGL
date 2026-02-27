#ifndef __ENGINE_H__
#define __ENGINE_H__

#if defined(_WIN32) || defined(_WIN64)
    #define AERO_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define AERO_PLATFORM_LINUX
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct SGLWindow* GLWindow;
typedef struct SGLCamera* GLCamera;
typedef struct SInput* Input;
typedef struct SDebugRenderer* DebugRenderer;
typedef struct SStateManager* StateManager;
typedef struct STerrainManager* TerrainManager;

typedef struct SEngine
{
	GLWindow window;
	GLCamera camera;
	Input Input;
	DebugRenderer debugRenderer;
	StateManager stateManager;
	TerrainManager terrainManager;
	float deltaTime;
	float lastFrame;
	bool isRunning;
	bool isWireframe;
} SEngine;

typedef struct SEngine* Engine;

// High-level "Life Cycle" functions
bool Engine_Initialize(Engine pEngine);
void Engine_HandleInput(Engine pEngine);
void Engine_Update(Engine pEngine);
void Engine_Render(Engine pEngine);
void Engine_Destroy(Engine pEngine);
void Engine_OnKeyButton(Engine pEngine, int key, int action);
void Engine_OnMouseButton(Engine pEngine, int key, int action);
Engine GetEngine();

// Accessors
GLWindow Engine_GetWindow();
GLCamera Engine_GetCamera();
Input Engine_GetInput();
DebugRenderer Engine_GetDebugRenderer();
StateManager Engine_GetStateManager();
TerrainManager Engine_GetTerrainRenderer();

float Engine_DeltaTime();
void Engine_SetDeltaTime(float deltaTime);
float Engine_LastFrame();
void Engine_SetLastFrame(float lastFrame);
bool Engine_IsRunning();
void Engine_SetIsRunning(bool bIsRunning);
bool Engine_IsWireframe();
void Engine_SetIsWireframe(bool bIsWireframe);

float* Engine_DeltaTimePtr();
float* Engine_LastFramePtr();
bool* Engine_IsRunningPtr();
bool* Engine_IsWireframePtr();

// set callbacks
void framebuffer_size_callback(GLFWwindow* window, GLint iWidth, GLint iHeight);
void cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
void cursorscroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mousebutton_callback(GLFWwindow* window, int button, int action, int mods);

#if defined(__cplusplus)
}
#endif

#endif // __ENGINE_H__
