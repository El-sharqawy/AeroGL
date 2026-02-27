#ifndef __STDAFX_H__
#define __STDAFX_H__

// some common stuff
#include "AeroPlatform.h"

// External Libs
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>

#if defined(AERO_PLATFORM_WINDOWS)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN // Strips out rarely used stuff (like DDE, Shell, etc.)
	#endif
#endif

#include "Resources/MemoryManager.h" // new Malloc

#include "Engine.h"
#include "Core/Window.h"
#include "Core/Camera.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Buffers/Buffer.h"
#include "Renderer/DebugRenderer.h"
#include "PipeLine/StateManager.h"
#include "Terrain/TerrainManager/TerrainManager.h"

// C External Libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <xmmintrin.h> // SSE

#endif
