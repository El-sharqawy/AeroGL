#ifndef __MEMORY_TAGS_H__
#define __MEMORY_TAGS_H__

#ifndef AERO_ALIGN
    #if defined(_MSC_VER)
        #define AERO_ALIGN(x) __declspec(align(x))
    #elif defined(__GNUC__) || defined(__clang__)
        #define AERO_ALIGN(x) __attribute__((aligned(x)))
    #else
        #include <stdalign.h>
        #define AERO_ALIGN(x) alignas(x)
    #endif
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define AERO_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define AERO_PLATFORM_LINUX
#endif

#include <stddef.h>

typedef enum EMemoryTag
{
    MEM_TAG_NONE = 0,
    MEM_TAG_ENGINE,      // General engine systems (Core, Input, etc.) The window is the engine's life.
    MEM_TAG_RENDERING,   // Renderer logic, command queues Purely related to drawing.
    MEM_TAG_GPU_BUFFER,  // Vertex/Index/Uniform buffer metadata
    MEM_TAG_SHADER,      // Shader source code and reflection data
    MEM_TAG_TEXTURE,     // CPU-side pixel data before upload
    MEM_TAG_PHYSICS,     // Collision data and transforms Related to the simulation.
    MEM_TAG_RESOURCES,   // Asset management (Models, Materials) These are loaded from disk and can be purged.
    MEM_TAG_STRINGS,     // Log messages and name handles Dedicated to text data.
    MEM_TAG_TERRAIN,     // Heightmaps, Grid data, Foliage maps
    MEM_TAG_COUNT
} EMemoryTag;

// Human-readable names for the tags
static const char* MemoryTagNames[] =
{
    "None", "Engine", "Rendering", "GPU_Buffer", "Shader",
    "Texture", "Physics", "Resources", "Strings", "Terrain"
};

#endif // __MEMORY_TAGS_H__
