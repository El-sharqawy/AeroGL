#ifndef __INTERFACE_IMGUI_H__
#define __INTERFACE_IMGUI_H__

#ifdef __cplusplus
extern "C" {
#endif
	void ImGui_Init();
	void ImGui_NewFrame();
	void ImGui_Render();
	void ImGui_Shutdown();
#ifdef __cplusplus
};
#endif

#endif // __INTERFACE_IMGUI_H__