#include "Window.h"
#include "CoreUtils.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>

typedef struct SGLWindow
{
	// OpenGL Data
	GLFWwindow* m_pGLWindow;
	const GLFWvidmode* m_pVidMode;
	GLFWmonitor* m_pMonitor;

	// Window Data
	char* m_szWindowTitle;
	EWinowMode windowMode;

	// Window Deminsions Data
	GLint m_iWidth;
	GLint m_iHeight;
	GLint m_iWindowedWidth;
	GLint m_iWindowedHeight;
	GLint m_iFullScreenWidth;
	GLint m_iFullScreenHeight;
} SGLWindow;

bool InitializeWindow(GLWindow pWindow)
{
	if (!glfwInit())
	{
		syserr("Failed to Initialize GLFW Library");
		return (GLFW_FALSE);
	}

	// Initialize OpenGL 4.6 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);							// OpenGL 4.x
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);							// OpenGL x.6
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);			// Enable Core Profile Only

	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);					// Enable Debugging

	glfwWindowHint(GLFW_SAMPLES, 0);										// Multi Sampling

	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);								// Decorated Window
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);								// Invisible Window, must show it at the end

	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);							// set focus on this window when we show it

	pWindow->m_pMonitor = glfwGetPrimaryMonitor();						// Get Monitor Data
	pWindow->m_pVidMode = glfwGetVideoMode(pWindow->m_pMonitor);		// Get Video Mode Settings

	// Setup GLFW Rendering Data
	glfwWindowHint(GLFW_RED_BITS, pWindow->m_pVidMode->redBits);			// Set red bits value for the window based on the monitor
	glfwWindowHint(GLFW_GREEN_BITS, pWindow->m_pVidMode->greenBits);		// Set Green bits value for the window based on the monitor
	glfwWindowHint(GLFW_BLUE_BITS, pWindow->m_pVidMode->blueBits);		// Set Blue bits value for the window based on the monitor
	glfwWindowHint(GLFW_REFRESH_RATE, pWindow->m_pVidMode->refreshRate);	// Set Refresh rate based on the monitor

	pWindow->m_iFullScreenWidth = pWindow->m_pVidMode->width;
	pWindow->m_iFullScreenHeight = pWindow->m_pVidMode->height;

	pWindow->m_iWindowedWidth = (pWindow->m_iFullScreenWidth * 75) / 100; // 75% of full screen width
	pWindow->m_iWindowedHeight = (pWindow->m_iFullScreenHeight * 75) / 100; // 75% of full screen height

	if (pWindow->windowMode == WINDOWED)
	{
		pWindow->m_iWidth = pWindow->m_iWindowedWidth;
		pWindow->m_iHeight = pWindow->m_iWindowedHeight;

		pWindow->m_pGLWindow = glfwCreateWindow(pWindow->m_iWidth, pWindow->m_iHeight, pWindow->m_szWindowTitle, NULL, NULL);

	}
	else if (pWindow->windowMode == FULLSCREEN)
	{
		pWindow->m_iWidth = pWindow->m_iFullScreenWidth;
		pWindow->m_iHeight = pWindow->m_iFullScreenHeight;

		pWindow->m_pGLWindow = glfwCreateWindow(pWindow->m_iWidth, pWindow->m_iHeight, pWindow->m_szWindowTitle, pWindow->m_pMonitor, NULL);
	}

	// check if glfwCreateWindow Fail
	if (pWindow->m_pGLWindow == nullptr)
	{
		syserr("failed to Initialize GLFW Library");
		return (GLFW_FALSE);
	}

	if (pWindow->windowMode == WINDOWED)
	{
		// Set Windowed window position
		glfwSetWindowPos(pWindow->m_pGLWindow, (pWindow->m_iFullScreenWidth - pWindow->m_iWidth) / 2, (pWindow->m_iFullScreenHeight - pWindow->m_iHeight) / 2);
	}

	// Set Current Context
	glfwMakeContextCurrent(pWindow->m_pGLWindow);

	// Initialize GLAD library, must be done after making context current
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == GL_FALSE)
	{
		syserr("failed to Initialize GLAD Library");
		return (GLFW_FALSE);
	}

	// Show Window
	glfwShowWindow(pWindow->m_pGLWindow);

	return (GLFW_TRUE);
}

bool AllocateWindow(GLWindow* ppWindow)
{
	*ppWindow = (GLWindow)tracked_malloc(sizeof(SGLWindow));

	GLWindow pWindow = *ppWindow;
	if (!pWindow)
	{
		syserr("Failed to Allocate Memory for window");
		return (false);
	}

	if (pWindow)
	{
		// Set all bytes to 0, ensuring m_szWindowTitle is NULL
		memset(pWindow, 0, sizeof(SGLWindow));
	}
	return (true);
}

void DeallocateWindow(GLWindow* ppWindow)
{
	if (!ppWindow || !*ppWindow)
	{
		return;
	}

	GLWindow pWindow = *ppWindow;

	if (pWindow->m_pGLWindow)
	{
		glfwDestroyWindow(pWindow->m_pGLWindow);
	}

	if (pWindow->m_szWindowTitle)
	{
		tracked_free(pWindow->m_szWindowTitle);
	}

	tracked_free(pWindow);

	*ppWindow = NULL;
}

void SetWindowTitle(GLWindow pWindow, const char* szTitle)
{
	// 1. If we already had a title, we must free it first to avoid a leak
	if (pWindow->m_szWindowTitle != NULL)
	{
		tracked_free((void*)pWindow->m_szWindowTitle);
	}

	// 2. Create the new copy
	pWindow->m_szWindowTitle = tracked_strdup(szTitle);

	// 3. Update the actual GLFW window if it exists
	if (pWindow->m_szWindowTitle)
	{
		glfwSetWindowTitle(pWindow->m_pGLWindow, pWindow->m_szWindowTitle);
	}
}		

void SetWindowMode(GLWindow pWindow, EWinowMode windowMode)
{
	pWindow->windowMode = windowMode;
}

bool DestroyGLWindow(GLWindow pWindow)
{
	glfwDestroyWindow(pWindow->m_pGLWindow);
	return (GLFW_TRUE);
}

GLFWwindow* GetGLWindow(GLWindow pWindow)
{
	return pWindow->m_pGLWindow;
}

GLint GetWindowWidth(GLWindow pWindow)
{
	return (pWindow->m_iWidth);
}

GLint GetWindowHeight(GLWindow pWindow)
{
	return (pWindow->m_iHeight);
}

GLfloat GetWindowWidthF(GLWindow pWindow)
{
	return (float)(pWindow->m_iWidth);
}

GLfloat GetWindowHeightF(GLWindow pWindow)
{
	return (float)(pWindow->m_iHeight);
}

void UpdateWindowDeminsions(GLWindow pWindow, int width, int height)
{
	pWindow->m_iWidth = width;
	pWindow->m_iHeight = height;
}

