cmake_minimum_required(VERSION 3.23)
project(AeroGL)

add_executable(AeroGL)

# Set the C standard to C23 or latest
if (MSVC)
# This ensures C flags ONLY go to .c files
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/std:clatest>)
    # This ensures CXX flags ONLY go to .cpp files
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++20>)
else()
	set(CMAKE_C_STANDARD 23)
	set(CMAKE_C_STANDARD_REQUIRED ON)
	set(CMAKE_C_EXTENSIONS OFF) # Set to ON if you want 'gnu23' features
endif()

set(CMAKE_BUILD_TYPE Debug)

# Grab all sources
set(AEROGL_SOURCES
	Stdafx.c
	Engine.c
	Main.c
)

set(AEROGL_HEADERS
	Stdafx.h
	AeroPlatform.h
	Engine.h
)

include_directories(BEFORE "${CMAKE_SOURCE_DIR}/Extern/include")

# 2. Tell CMake where the compiled .a or .so or .lib files are
link_directories("${CMAKE_SOURCE_DIR}/Extern/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# 1. Add the Libraries first so other things can link to it
add_subdirectory(MyLIB)
add_subdirectory(Math)
add_subdirectory(Buffers)
add_subdirectory(Core)
add_subdirectory(Meshes)
add_subdirectory(OpenGLUtils)
add_subdirectory(PipeLine)
add_subdirectory(Renderer)
add_subdirectory(Resources)
add_subdirectory(Terrain)
add_subdirectory(LibImageUI)
add_subdirectory(UserInterface)

target_precompile_headers(AeroGL PRIVATE Stdafx.h)

target_sources(AeroGL
	PRIVATE
		${AEROGL_SOURCES}

	PUBLIC
	FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} # Use absolute path for safety
	FILES
		${AEROGL_HEADERS}	        
)

# This ensures that library knows where its own headers are during its own compilation
target_include_directories(AeroGL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(AeroGL PRIVATE "Extern/include")

# Tells the compiler to optimize for the current CPU architecture (AVX2/FMA)
target_compile_options(AeroGL PRIVATE 
    -march=native
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3>
)

# using shared:
target_link_libraries(AeroGL PRIVATE 
    Terrain
    Renderer
    LibImageUI
    UserInterface
    Resources
    PipeLine
    OpenGLUtils
    Meshes
    Math
    Core
    Buffers
    MyLIB
)

if (WIN32)
target_link_libraries(AeroGL PRIVATE 
    # External libraries (Windows specific)
    $<$<CONFIG:Debug>:"${CMAKE_SOURCE_DIR}/Extern/lib/Windows/glfw3_d"> # .lib
    $<$<CONFIG:Release>:"${CMAKE_SOURCE_DIR}/Extern/lib/Windows/glfw3"> # .lib
    # Other standard Windows system libs
    opengl32.lib
    gdi32.lib 
    user32.lib 
    shell32.lib
)
else()
target_link_libraries(AeroGL PRIVATE 
    $<$<CONFIG:Debug>:"${CMAKE_SOURCE_DIR}/Extern/lib/Linux/libglfw_d.so">
    $<$<CONFIG:Release>:"${CMAKE_SOURCE_DIR}/Extern/lib/Linux/libglfw.so">
    GL
    dl
    m
    pthread
)
endif()
