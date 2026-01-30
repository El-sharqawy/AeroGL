#include "Interface_imgui.h"

#include "../../LibImageUI/Stdafx.h"

void ImGui_Init(GLFWwindow* window)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark(); // Could be Light Too

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui::GetStyle().FramePadding = ImVec2(8, 6); // Padding for menu items
	style.FramePadding = ImVec2(8, 6); // permanent global change, no push/pop

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

void ImGui_NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGui_Render()
{
	static bool bShowDemo = false;
	ImGui::ShowDemoWindow(&bShowDemo);

	// Place Rendering objects here
	ImGui_RenderEngineMainUI();

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	/*if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}*/
}

void ImGui_Shutdown()
{
	// Check if the ImGui context exists before attempting to destroy it.
	// If the context is null, it means we have already shut down.
	if (ImGui::GetCurrentContext() == nullptr)
	{
		return;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGui_RenderEngineMainUI()
{
	ImGui::Begin("Terrain Tools");
	if (ImGui::BeginTabBar("##MainEditorTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Engine"))
		{
			ImGui_RenderEngineDataUI();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Maps"))
		{
			ImGui_RenderMapsUI();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	// Show FPS
	ImGui::NewLine();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

/**
 * @brief Renders the engine data UI panel.
 *
 * Displays information about the engine version,
 * build date, and build time.
 */
void ImGui_RenderEngineDataUI()
{
	ImGui::Text("Engine Version: %s", ENGINE_VERSION);
	ImGui::Text("Build Date: %s", __DATE__);
	ImGui::Text("Build Time: %s", __TIME__);
	ImGui::Separator();

	ImGui::Separator();
}

void ImGui_RenderMapsUI()
{
	static const TerrainManager pTerrainManager = GetTerrainManager();
	static bool showCreateMapPopup = false;
	static bool showLoadMapPopup = false;
	static bool showSaveMapPopup = false;

	ImVec2 buttonSize(125, 25);

	// "New Map" button to open the popup
	if (ImGui::Button("New Map", buttonSize))
	{
		showCreateMapPopup = true;
		ImGui::OpenPopup("Create New Map");
	}
	ImGui_RenderCreateNewMapPopUP(&showCreateMapPopup);

	ImGui::SameLine();
	// "Load Map" button to load map with popup
	if (ImGui::Button("Load Map", buttonSize))
	{
		showLoadMapPopup = true;
		ImGui::OpenPopup("Load New Map");
	}
	ImGui_RenderLoadMapPopUP(&showLoadMapPopup);

}

void ImGui_RenderCreateNewMapPopUP(bool* showPopup)
{
	static const TerrainManager pTerrainManager = GetTerrainManager();
	static char szMapName[256] = "map_new";
	static GLint iMapSizeX = 1;
	static GLint iMapSizeZ = 1;

	// Always center the popup when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create New Map", &*showPopup,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Create a new terrain map. Map name must be unique.");


		ImGui::InputText("Map Name", szMapName, IM_ARRAYSIZE(szMapName));
		ImGui::InputInt("Map X Size", &iMapSizeX);
		ImGui::InputInt("Map Z Size", &iMapSizeZ);

		// Clamp values to valid range
		//iMapSizeX = clampi(iMapSizeX, 1, 256);
		//iMapSizeZ = clampi(iMapSizeZ, 1, 256);

		ImGui::Separator();

		bool isInvalid = (strlen(szMapName) == 0);
		if (isInvalid)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Map name cannot be empty!");
		}

		// Proper scoped disabled region
		ImGui::BeginDisabled(isInvalid);

		// Create button - no need for && isValid here
		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			TerrainManager_SetMapName(pTerrainManager, szMapName);
			TerrainManager_SetMapDeminsions(pTerrainManager, iMapSizeX, iMapSizeZ);

			if (TerrainManager_CreateMap(pTerrainManager))
			{
				*showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				syserr("Failed to Create Map %s", szMapName);
			}
		}

		// End disabled region if we started it
		ImGui::EndDisabled();

		// Handle Enter key - only when valid
		if (ImGui::IsKeyPressed(ImGuiKey_Enter))
		{
			TerrainManager_SetMapName(pTerrainManager, szMapName);
			TerrainManager_SetMapDeminsions(pTerrainManager, iMapSizeX, iMapSizeZ);

			if (TerrainManager_CreateMap(pTerrainManager))
			{
				*showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				syserr("Failed to Create Map %s", szMapName);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			*showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		// Handle Escape Key
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			*showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void ImGui_RenderLoadMapPopUP(bool* showPopup)
{
	static const TerrainManager pTerrainManager = GetTerrainManager();
	static char szMapName[256] = "map_new";

	// Always center the popup when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Load New Map", &*showPopup,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Load Map. write it's name in the text box");
		ImGui::InputText("Map Name", szMapName, IM_ARRAYSIZE(szMapName));

		ImGui::Separator();

		bool isInvalid = (strlen(szMapName) == 0);
		if (isInvalid)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Map name cannot be empty!");
		}

		// Disable load button if invalid
		ImGui::BeginDisabled(isInvalid);

		// load and Cancel buttons
		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			if (TerrainManager_LoadMap(pTerrainManager, szMapName))
			{
				*showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				syserr("Failed to Load Map %s", szMapName);
			}
		}
		ImGui::EndDisabled();

		// Handle Enter key - only when valid
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !isInvalid)
		{
			if (TerrainManager_LoadMap(pTerrainManager, szMapName))
			{
				*showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				syserr("Failed to Load Map %s", szMapName);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			*showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		// Handle Escape key
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !isInvalid)
		{
			*showPopup = false;
			ImGui::CloseCurrentPopup();
		}


		ImGui::EndPopup();

	}
}
