#include "Engine.h"
#include "Stdafx.h"
#include "Lib/Vector.h"
#include "UserInterface/Interface_imgui.h"
#include <time.h>

static Engine s_Instance = NULL; // Hidden from other files

void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR) {
		syserr("GL ERROR: %s", message);
		__debugbreak(); // Automatically pauses your debugger on the exact line!
	}
}

bool InitializeEngine(Engine pEngine)
{
	uint32_t startSeed = (uint32_t)time(NULL);
	srand(startSeed);
	syslog("Engine started with Seed: %u", startSeed);

	s_Instance = pEngine;

	// Validation Check
	if (!Window_Initialize(&pEngine->window))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	Window_SetTitle(pEngine->window, "AeroGL");
	Window_SetMode(pEngine->window, WINDOWED);
	if (!Window_InitializeGLWindow(pEngine->window))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	if (!Input_Initialize(&pEngine->Input))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	// Validation Check
	if (!InitializeCamera(&pEngine->camera, Window_GetWidthF(pEngine->window), Window_GetHeightF(pEngine->window)))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	// CreateRenderer(&pEngine->renderer, pEngine->camera, "MainRenderer");
	if (!CreateDebugRenderer(&pEngine->debugRenderer, pEngine->camera, "DebugRenderer"))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	if (!StateManager_Initialize(&pEngine->stateManager))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	if (!TerrainManager_Initialize(&pEngine->terrainManager))
	{
		DestroyEngine(pEngine);
		return (false);
	}

	pEngine->deltaTime = 0.0f;;
	pEngine->lastFrame = 0.0f;;

	pEngine->isRunning = true;

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

void HandleEngineInput(Engine pEngine)
{
	if (IsKeyDown(pEngine->Input, GLFW_KEY_W))
	{
		ProcessCameraKeboardInput(pEngine->camera, DIRECTION_FORWARD, pEngine->deltaTime);
	}
	if (IsKeyDown(pEngine->Input, GLFW_KEY_D))
	{
		ProcessCameraKeboardInput(pEngine->camera, DIRECTION_RIGHT, pEngine->deltaTime);
	}
	if (IsKeyDown(pEngine->Input, GLFW_KEY_S))
	{
		ProcessCameraKeboardInput(pEngine->camera, DIRECTION_BACKWARD, pEngine->deltaTime);
	}
	if (IsKeyDown(pEngine->Input, GLFW_KEY_A))
	{
		ProcessCameraKeboardInput(pEngine->camera, DIRECTION_LEFT, pEngine->deltaTime);
	}
}

void UpdateEngine(Engine pEngine)
{
	// 1. Time Update
	float currentFrame = (float)glfwGetTime();
	pEngine->deltaTime = currentFrame - pEngine->lastFrame;
	pEngine->lastFrame = currentFrame;

	// 2. Events & Input
	glfwPollEvents();
	UpdateInput(pEngine->Input);
	HandleEngineInput(pEngine); // Engine Handle Input
	UpdateCamera(pEngine->camera);

	// Update ImgUI
	ImGui_NewFrame();

	TerrainManager_Update(pEngine->terrainManager);

	// 3. Render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.2f, 0.2f, 0.2f, 0.5f);

	RenderEngine(pEngine);

	// Render on top of everything
	ImGui_Render();

	// 4. Swap Window Buffers
	glfwSwapBuffers(Window_GetGLWindow(pEngine->window));
}

void RenderEngine(Engine pEngine)
{
	// RenderRenderer(pEngine->renderer);
	RenderDebugRenderer(pEngine->debugRenderer);
	TerrainManager_Render(pEngine->terrainManager);
}

void DestroyEngine(Engine pEngine)
{
	syslog("Attemp to shutdown the engine...");

	ImGui_Shutdown();

	TerrainManager_Destroy(&pEngine->terrainManager);

	StateManager_Destroy(&pEngine->stateManager);

	DestroyCamera(&pEngine->camera);

	// DestroyRenderer(&pEngine->renderer);
	DestroyDebugRenderer(&pEngine->debugRenderer);

	Input_Destroy(&pEngine->Input);

	Window_Deallocate(&pEngine->window);

	glfwTerminate();
}

Engine GetEngine()
{
	return (s_Instance);
}

void OnEngineKeyButton(Engine pEngine, int key, int action)
{
	if (key > GLFW_KEY_LAST || key < 0)
	{
		return;
	}

	OnKeyButton(pEngine->Input, key, action);
}

void OnEngineMouseButton(Engine pEngine, int key, int action)
{
	if (key > GLFW_MOUSE_BUTTON_LAST || key < 0)
	{
		return;
	}

	OnMouseButton(pEngine->Input, key, action);
}

void framebuffer_size_callback(GLFWwindow* window, GLint iWidth, GLint iHeight)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	Window_UpdateDeminsions(pEngine->window, iWidth, iHeight);
	UpdateCameraDeminsions(pEngine->camera, (float)iWidth, (float)iHeight);

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

	OnMousePosition(pEngine->Input, (float)xpos, (float)ypos);
}

void cursorscroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	OnMouseScroll(pEngine->Input, (float)yoffset);
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	OnEngineKeyButton(pEngine, key, action);
}

void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
	Engine pEngine = (Engine)glfwGetWindowUserPointer(window);
	if (!pEngine)
	{
		return;
	}

	OnEngineMouseButton(pEngine, button, action);
}
