// External Libs
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN // Strips out rarely used stuff (like DDE, Shell, etc.)
	#endif
#endif

#include "Resources/MemoryManager.h" // new Malloc

#include "Engine.h"
#include "Core/Log.h"
#include "Core/Input.h"

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
