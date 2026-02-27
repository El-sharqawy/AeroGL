cmake_minimum_required(VERSION 3.23)
project(LibImageUI LANGUAGES CXX)

# Set the C++ standard to C20 or latest
if (MSVC)
    # We use 'target' level settings instead of global 'add_compile_options'
    # to prevent flag clashing.
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
    # This prevents this library from trying to use C standards at all
    set(CMAKE_C_STANDARD OFF) 
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()

# Grab all sources recursively
file(GLOB_RECURSE LibImageUI_SOURCES 
     CONFIGURE_DEPENDS 
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Grab all headers recursively
file(GLOB_RECURSE LibImageUI_HEADERS
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Then filter out the build directory
list(FILTER LibImageUI_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}")
list(FILTER LibImageUI_HEADERS EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}")

add_library(LibImageUI STATIC)

target_sources(LibImageUI
	PRIVATE
		${LibImageUI_SOURCES}

	PUBLIC
	FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} # Use absolute path for safety
	FILES
		${LibImageUI_HEADERS}	        
)

# This ensures that library knows where its own headers are during its own compilation
target_include_directories(LibImageUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(LibImageUI PRIVATE "Extern/include")

# target_precompile_headers(LibImageUI PRIVATE Stdafx.h)

# Tells the compiler to optimize for the current CPU architecture (AVX2/FMA)
if(NOT MSVC)
    add_compile_options(
    -march=native
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3>
)
endif()
