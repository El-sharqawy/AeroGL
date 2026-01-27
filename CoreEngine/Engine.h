#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "Core/Window.h"
#include "Core/Camera.h"
#include "Core/Input.h"
#include "Buffers/Buffer.h"
#include "Renderer/Shader.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/StateManager.h"
#include "Terrain/TerrainManager/TerrainManager.h"

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
bool InitializeEngine(Engine pEngine);
void HandleEngineInput(Engine pEngine);
void UpdateEngine(Engine pEngine);
void RenderEngine(Engine pEngine);
void DestroyEngine(Engine pEngine);

Engine GetEngine();

void OnEngineKeyButton(Engine pEngine, int key, int action);
void OnEngineMouseButton(Engine pEngine, int key, int action);

// set callbacks
void framebuffer_size_callback(GLFWwindow* window, GLint iWidth, GLint iHeight);
void cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
void cursorscroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mousebutton_callback(GLFWwindow* window, int button, int action, int mods);

#endif // __ENGINE_H__