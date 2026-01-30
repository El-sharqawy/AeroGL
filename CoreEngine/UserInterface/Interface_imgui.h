#ifndef __INTERFACE_IMGUI_H__
#define __INTERFACE_IMGUI_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define ENGINE_VERSION "AeroGL-Engine v1.0.0"

// Include C Files Here
#if defined(__cplusplus)
extern "C" {
#endif

#include "../Terrain/TerrainManager/TerrainManager.h"
#include "../Math/MathUtils.h"
#include "../Core/CoreUtils.h"

#if defined(__cplusplus)
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
	void ImGui_Init(GLFWwindow* window);
	void ImGui_NewFrame();
	void ImGui_Render();
	void ImGui_Shutdown();

	void ImGui_RenderEngineMainUI();

	// Sub Windows
	void ImGui_RenderEngineDataUI();
	void ImGui_RenderMapsUI();
	void ImGui_RenderCreateNewMapPopUP(bool* showPopup);

	void ImGui_RenderLoadMapPopUP(bool* showPopup);

#ifdef __cplusplus
};
#endif

#endif // __INTERFACE_IMGUI_H__