#ifndef __INTERFACE_IMGUI_H__
#define __INTERFACE_IMGUI_H__

#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif
	void ImGui_Init(GLFWwindow* window);
	void ImGui_NewFrame();
	void ImGui_Render();
	void ImGui_Shutdown();
#ifdef __cplusplus
};
#endif

#endif // __INTERFACE_IMGUI_H__