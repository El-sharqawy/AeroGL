#include "Engine.h"
#include "Core/CoreUtils.h"
#include "Lib/Vector.h"
#include "UserInterface/Interface_imgui.h"
#include <assert.h>

static Engine s_Instance = NULL; // Hidden from other files

bool InitializeEngine(Engine pEngine)
{
	s_Instance = pEngine;

	// Validation Check
	if (!AllocateWindow(&pEngine->window))
	{
		return (false);
	}

	SetWindowTitle(pEngine->window, "AeroGL");
	SetWindowMode(pEngine->window, WINDOWED);
	if (!InitializeWindow(pEngine->window))
	{
		return (false);
	}

	// Validation Check
	if (!InitializeCamera(&pEngine->camera, GetWindowWidthF(pEngine->window), GetWindowHeightF(pEngine->window)))
	{
		return (false);
	}

	// CreateRenderer(&pEngine->renderer, pEngine->camera, "MainRenderer");
	CreateDebugRenderer(&pEngine->debugRenderer, pEngine->camera, "DebugRenderer");

	if (!InitializeInput(&pEngine->Input))
	{
		return (false);
	}

	if (!InitializeStateManager(&pEngine->stateManager))
	{
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
	glfwSetWindowUserPointer(GetGLWindow(pEngine->window), pEngine);

	// set GLFW callbacks
	glfwSetFramebufferSizeCallback(GetGLWindow(pEngine->window), framebuffer_size_callback);
	glfwSetCursorPosCallback(GetGLWindow(pEngine->window), cursorpos_callback);
	glfwSetScrollCallback(GetGLWindow(pEngine->window), cursorscroll_callback);

	glfwSetKeyCallback(GetGLWindow(pEngine->window), keyboard_callback);
	glfwSetMouseButtonCallback(GetGLWindow(pEngine->window), mousebutton_callback);

	ImGui_Init(GetGLWindow(pEngine->window));

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

	// 3. Render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.2f, 0.2f, 0.2f, 0.5f);

	RenderEngine(pEngine);

	// Render on top of everything
	ImGui_Render();

	// 4. Swap Window Buffers
	glfwSwapBuffers(GetGLWindow(pEngine->window));

	assert(GetStateDepth(GetStateManager()) == 0);
}

void RenderEngine(Engine pEngine)
{
	// RenderRenderer(pEngine->renderer);
	RenderDebugRenderer(pEngine->debugRenderer);
}

void DestroyEngine(Engine pEngine)
{
	syslog("Attemp to shutdown the engine...");

	ImGui_Shutdown();

	DestroyStateManager(&pEngine->stateManager);

	DestroyCamera(&pEngine->camera);

	// DestroyRenderer(&pEngine->renderer);
	DestroyDebugRenderer(&pEngine->debugRenderer);

	DestroyInput(&pEngine->Input);

	DeallocateWindow(&pEngine->window);

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

	UpdateWindowDeminsions(pEngine->window, iWidth, iHeight);
	UpdateCameraDeminsions(pEngine->camera, (float)iWidth, (float)iHeight);

	syslog("Window Resized: (%d, %d)", GetWindowWidth(pEngine->window), GetWindowHeight(pEngine->window));

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
