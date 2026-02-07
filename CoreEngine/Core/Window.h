#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct SGLWindow* GLWindow;

typedef enum EWinowMode
{
	WINDOWED,
	FULLSCREEN,
} EWinowMode;

bool Window_Initialize(GLWindow* ppWindow);
void Window_Deallocate(GLWindow* pWindow);

bool Window_InitializeGLWindow(GLWindow pWindow);

void Window_SetTitle(GLWindow pWindow, const char* szTitle);
void Window_SetMode(GLWindow pWindow, EWinowMode windowMode);
bool Window_DestroyGLWindow(GLWindow pWindow);
GLFWwindow* Window_GetGLWindow(GLWindow pWindow);

GLint Window_GetWidth(GLWindow pWindow);
GLint Window_GetHeight(GLWindow pWindow);
GLfloat Window_GetWidthF(GLWindow pWindow);
GLfloat Window_GetHeightF(GLWindow pWindow);

void Window_UpdateDeminsions(GLWindow pWindow, int width, int height);

#endif // __WINDOW_H__