#ifndef __AERO_PLATFORM_H__
#define __AERO_PLATFORM_H__

// --- OS Detection ---
#if defined(_WIN32) || defined(_WIN64)
    #define AERO_PLATFORM_WINDOWS
    #include <direct.h>   // For _mkdir
    #include <io.h>       // For _access
    #define MKDIR(path) _mkdir(path)
#elif defined(__linux__) || defined(__unix__)
    #define AERO_PLATFORM_LINUX
    #include <sys/stat.h> // For mkdir, stat
    #include <sys/types.h>
    #include <unistd.h>   // For access
    #define MKDIR(path) mkdir(path, 0755)

    #include <signal.h>
    #if defined(__x86_64__) || defined(__i386__)
        // For your 7800X3D (x86_64), this triggers a hardware breakpoint (INT 3)
        #define __debugbreak() __asm__ volatile("int $3")
    #else
        // Fallback for other architectures (like ARM)
        #define __debugbreak() raise(SIGTRAP)
    #endif

#endif

// --- Common Headers ---
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#endif
