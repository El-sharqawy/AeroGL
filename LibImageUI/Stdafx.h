#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define IMGUI_DEFINE_MATH_OPERATORS

#ifndef _WIN32
    // Map MSVC secure functions to standard C functions for Linux
    #define sscanf_s sscanf
    #define strcpy_s(dest, src) strcpy(dest, src)
    #define strcat_s(dest, src) strcat(dest, src)
    // Note: If you encounter sprintf_s later:
    #define sprintf_s snprintf
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
