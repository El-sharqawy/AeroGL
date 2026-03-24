#include "Engine.h"
#include "Stdafx.h"
#include "AeroLib/Vector.h"
#include "UserInterface/Interface_imgui.h"
#include <time.h>

static Engine s_Instance = NULL; // Hidden from other files

static void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR) {
		syserr("GL ERROR: %s", message);
		__debugbreak(); // Automatically pauses your debugger on the exact line!
	}
}

bool Engine_Initialize(Engine pEngine)
{
	uint32_t startSeed = (uint32_t)time(NULL);
	srand(startSeed);
	syslog("Engine started with Seed: %u", startSeed);

	s_Instance = pEngine;

	// Validation Check
	if (!Window_Initialize(&pEngine->window))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	Window_SetTitle(pEngine->window, "AeroGL");
	Window_SetMode(pEngine->window, WINDOWED);
	if (!Window_InitializeGLWindow(pEngine->window))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	if (!Input_Initialize(&pEngine->Input))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	// Validation Check
	if (!Camera_Initialize(&pEngine->camera, Window_GetWidthF(pEngine->window), Window_GetHeightF(pEngine->window)))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	// CreateRenderer(&pEngine->renderer, pEngine->camera, "MainRenderer");
	if (!CreateDebugRenderer(&pEngine->debugRenderer, pEngine->camera, "DebugRenderer"))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	if (!StateManager_Initialize(&pEngine->stateManager))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	if (!TerrainManager_Initialize(&pEngine->terrainManager))
	{
		Engine_Destroy(pEngine);
		return (false);
	}

	pEngine->deltaTime = 0.0f;
	pEngine->lastFrame = 0.0f;

	pEngine->isRunning = true;
	pEngine->isWireframe = true;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);


	// Set the window pointer
	glfwSetWindowUserPointer(Window_GetGLWindow(pEngine->window), pEngine);

	// set GLFW callbacks
	glfwSetFramebufferSizeCallback(Window_GetGLWindow(pEngine->window), framebuffer_size_callback);
	glfwSetCursorPosCallback(Window_GetGLWindow(pEngine->window), cursorpos_callback);
	glfwSetScrollCallback(Window_GetGLWindow(pEngine->window), cursorscroll_callback);

	glfwSetKeyCallback(Window_GetGLWindow(pEngine->window), keyboard_callback);
	glfwSetMouseButtonCallback(Window_GetGLWindow(pEngine->window), mousebutton_callback);

	// Enable it
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Makes sure the error happens on the line that caused it
	glDebugMessageCallback(MessageCallback, 0);

	ImGui_Init(Window_GetGLWindow(pEngine->window));

	return (true);;
}

void Engine_HandleInput(Engine pEngine)
{
	if (Input_IsKeyDown(pEngine->Input, GLFW_KEY_W))
	{
		Camera_ProcessCameraKeboardInput(pEngine->camera, DIRECTION_FORWARD, pEngine->deltaTime);
	}
	if (Input_IsKeyDown(pEngine->Input, GLFW_KEY_D))
	{
		Camera_ProcessCameraKeboardInput(pEngine->camera, DIRECTION_RIGHT, pEngine->deltaTime);
	}
	if (Input_IsKeyDown(pEngine->Input, GLFW_KEY_S))
	{
		Camera_ProcessCameraKeboardInput(pEngine->camera, DIRECTION_BACKWARD, pEngine->deltaTime);
	}
	if (Input_IsKeyDown(pEngine->Input, GLFW_KEY_A))
	{
		Camera_ProcessCameraKeboardInput(pEngine->camera, DIRECTION_LEFT, pEngine->deltaTime);
	}
}

void Engine_Update(Engine pEngine)
{
	// 1. Time Update
	float currentFrame = (float)glfwGetTime();
	pEngine->deltaTime = currentFrame - pEngine->lastFrame;
	pEngine->lastFrame = currentFrame;

	// 2. Events & Input
	glfwPollEvents();
	Input_Update(pEngine->Input);
	Engine_HandleInput(pEngine); // Engine Handle Input
	Camera_UpdateCamera(pEngine->camera);

	// Update ImgUI
	ImGui_NewFrame();

	TerrainManager_Update(pEngine->terrainManager);

	// 3. Render
	glClearColor(0.2f, 0.2f, 0.2f, 0.5f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Engine_Render(pEngine);

	// Render on top of everything
	ImGui_Render();

	// 4. Swap Window Buffers
	glfwSwapBuffers(Window_GetGLWindow(pEngine->window));
}

void Engine_Render(Engine pEngine)
{
 	if (pEngine->isWireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	RenderDebugRenderer(pEngine->debugRenderer);
	TerrainManager_Render(pEngine->terrainManager);
}

void Engine_Destroy(Engine pEngine)
{
	syslog("Attemp to shutdown the engine...");

	ImGui_Shutdown();

	TerrainManager_Destroy(&pEngine->terrainManager);

	StateManager_Destroy(&pEngine->stateManager);

	Camera_Destroy(&pEngine->camera);

	// DestroyRenderer(&pEngine->renderer);
	DestroyDebugRenderer(&pEngine->debugRenderer);

	Input_Destroy(&pEngine->Input);

	Window_Deallocate(&pEngine->window);

	glfwTerminate();
}

void Engine_OnKeyButton(Engine pEngine, int key, int action)
{
	if (key > GLFW_KEY_LAST || key < 0)
	{
		return;
	}

	Input_OnKeyButton(pEngine->Input, key, action);
}

void Engine_OnMouseButton(Engine pEngine, int key, int action)
{
	if (key > GLFW_MOUSE_BUTTON_LAST || key < 0)
	{
		return;
	}

	Input_OnMouseButton(pEngine->Input, key, action);
}

Engine GetEngine()
{
	return (s_Instance);
}

GLWindow Engine_GetWindow()
{
	return (GetEngine()->window);
}

GLCamera Engine_GetCamera()
{
	return (GetEngine()->camera);
}

Input Engine_GetInput()
{
	return (GetEngine()->Input);
}

DebugRenderer Engine_GetDebugRenderer()
{
	return (GetEngine()->debugRenderer);
}

StateManager Engine_GetStateManager()
{
	return (GetEngine()->stateManager);
}

TerrainManager Engine_GetTerrainRenderer()
{
	return (GetEngine()->terrainManager);
}

float Engine_DeltaTime()
{
	return (GetEngine()->deltaTime);
}

void Engine_SetDeltaTime(float deltaTime)
{
	GetEngine()->deltaTime = deltaTime;
}

float Engine_LastFrame()
{
	return (GetEngine()->lastFrame);
}

void Engine_SetLastFrame(float lastFrame)
{
	GetEngine()->lastFrame = lastFrame;
}

bool Engine_IsRunning()
{
	return (GetEngine()->isRunning);
}

void Engine_SetIsRunning(bool bIsRunning)
{
	GetEngine()->isRunning = bIsRunning;
}

bool Engine_IsWireframe()
{
	return (GetEngine()->isWireframe);
}

void Engine_SetIsWireframe(bool bIsWireframe)
{
	GetEngine()->isWireframe = bIsWireframe;
}

float* Engine_DeltaTimePtr()
{
	return (&GetEngine()->deltaTime);
}

float* Engine_LastFramePtr()
{
	return (&GetEngine()->lastFrame);
}

bool* Engine_IsRunningPtr()
{
	return (&GetEngine()->isRunning);
}

bool* Engine_IsWireframePtr()
{
	return (&GetEngine()->isWireframe);
}

void framebuffer_size_callback(GLFWwindow* window, GLint iWidth, GLint iHeight)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Window_UpdateDeminsions(pEngine->window, iWidth, iHeight);
	Camera_UpdateCameraDeminsions(pEngine->camera, (float)iWidth, (float)iHeight);

	// syslog("Window Resized: (%d, %d)", GetWindowWidth(pEngine->window), GetWindowHeight(pEngine->window));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, iWidth, iHeight);
}

void cursorpos_callback(GLFWwindow* window, double xpos, double ypos)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Input_OnMousePosition(pEngine->Input, (float)xpos, (float)ypos);
}

void cursorscroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Input_OnMouseScroll(pEngine->Input, (float)yoffset);
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Engine_OnKeyButton(pEngine, key, action);
}

void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Engine_OnMouseButton(pEngine, button, action);
}