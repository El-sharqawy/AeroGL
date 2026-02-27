cmake_minimum_required(VERSION 3.23)
project(Buffers)

# Set the C standard to C23 or latest
if (MSVC)
    set(CMAKE_C_STANDARD 23)
	add_compile_options(/W4 /permissive-) # High warnings and strict mode
else()
	set(CMAKE_C_STANDARD 23)
	set(CMAKE_C_STANDARD_REQUIRED ON)
	set(CMAKE_C_EXTENSIONS OFF) # Set to ON if you want 'gnu23' features
endif()

# Grab all sources recursively
file(GLOB_RECURSE Buffers_SOURCES 
     CONFIGURE_DEPENDS 
     "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Grab all headers recursively
file(GLOB_RECURSE Buffers_HEADERS
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Then filter out the build directory
list(FILTER Buffers_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}")
list(FILTER Buffers_HEADERS EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}")

add_library(Buffers STATIC)

target_sources(Buffers
	PRIVATE
		${Buffers_SOURCES}

	PUBLIC
	FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} # Use absolute path for safety
	FILES
		${Buffers_HEADERS}	        
)

# This ensures that library knows where its own headers are during its own compilation
target_include_directories(Buffers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Tells the compiler to optimize for the current CPU architecture (AVX2/FMA)
if(NOT MSVC)
    add_compile_options(
    -march=native
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3>
)
endif()
