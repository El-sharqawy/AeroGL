#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include "../Math/Vectors/Vector2.h"

typedef struct SGLWindow* GLWindow;

typedef enum EWinowMode
{
	WINDOWED,
	FULLSCREEN,
} EWinowMode;

bool AllocateWindow(GLWindow* ppWindow);
void DeallocateWindow(GLWindow* pWindow);
void SetWindowTitle(GLWindow pWindow, const char* szTitle);
void SetWindowMode(GLWindow pWindow, EWinowMode windowMode);

bool InitializeWindow(GLWindow pWindow);
bool DestroyGLWindow(GLWindow pWindow);
GLFWwindow* GetGLWindow(GLWindow pWindow);

GLint GetWindowWidth(GLWindow pWindow);
GLint GetWindowHeight(GLWindow pWindow);
GLfloat GetWindowWidthF(GLWindow pWindow);
GLfloat GetWindowHeightF(GLWindow pWindow);

void UpdateWindowDeminsions(GLWindow pWindow, int width, int height);

#endif // __WINDOW_H__