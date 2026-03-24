# AeroGL

A custom real-time 3D graphics engine written in **C (ISO C23)**, built on top of **OpenGL 4.5+**. AeroGL is a from-scratch engine focused on low-level GPU control, efficient terrain rendering, and clean engine architecture — without relying on any third-party engine or abstraction framework.

## Features

- **Engine Core** — Lifecycle management (`Init`, `Update`, `Render`, `Destroy`), delta time tracking, wireframe toggle, and a global engine singleton
- **Camera System** — First-person camera with mouse look, scroll zoom, and keyboard navigation
- **Input System** — Abstracted keyboard and mouse button callbacks via GLFW
- **Buffer Abstraction** — GPU buffer management (VBO, EBO, UBO, SSBO) using OpenGL DSA
- **Pipeline / State Manager** — Centralized OpenGL state management to minimize redundant state changes
- **Renderer** — Main render loop with a built-in **Debug Renderer** for visualizing normals, bounding boxes, and geometry
- **Terrain System** — Patch-based terrain manager supporting heightmap-driven terrain with LOD
- **Mesh System** — Mesh loading and geometry management
- **Math Library** — Custom SIMD-optimized vector and matrix math (vec2, vec3, vec4, mat3, mat4), quaternions, and ray utilities
- **Resource Manager** — Shader and texture resource loading and caching
- **UI System** — ImGui-based user interface integration via `UserInterface` and `LibImageUI` modules
- **Cross-platform** — Builds on both **Windows** (MSVC) and **Linux** (GCC/Clang)


## Project Structure

```markdown
AeroGL/
├── Assets/          # Raw asset files (textures, heightmaps, etc.)
├── Buffers/         # GPU buffer wrappers (VBO, EBO, UBO, SSBO)
├── Core/            # Window, Camera, and Input subsystems
├── Extern/          # Third-party headers and precompiled libs (GLFW, GLAD)
├── LibImageUI/      # Image-based UI rendering library
├── Math/            # Custom math library (vectors, matrices, quaternions)
├── Meshes/          # Mesh loading and geometry management
├── AeroLib/         # Internal shared utility library
├── OpenGLUtils/     # OpenGL helpers, error checking, and debug utilities
├── PipeLine/        # StateManager and render pipeline abstractions
├── Renderer/        # Main renderer and DebugRenderer
├── Resources/       # Shader and texture resource management
├── Terrain/         # Terrain manager, heightmap processing, and LOD
├── UserInterface/   # ImGui integration and UI panels
├── Engine.h/.c      # Top-level engine struct and lifecycle
├── Main.c           # Application entry point
├── Stdafx.h/.c      # Precompiled header
├── AeroPlatform.h   # Platform detection macros
├── CMakeLists.txt   # CMake build configuration
└── Makefile         # Alternative Makefile build
```

---

## Dependencies

| Library | Purpose |
|---|---|
| [GLFW](https://www.glfw.org/) | Window creation and input handling |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader |
| [ImGui](https://github.com/ocornut/imgui) | Immediate-mode debug UI |
| OpenGL 4.5+ | Graphics API |

All external libraries are vendored under `Extern/` — no package manager needed.

---

## Building

### Prerequisites

- CMake 3.23+
- A C23-capable compiler (MSVC on Windows, GCC 13+ or Clang 16+ on Linux)
- OpenGL 4.5+ capable GPU and drivers

### Windows (Visual Studio)

```bat
make_vs_solution.bat
```

This generates a Visual Studio solution in a `build/` directory. Open `AeroGL.sln` and build from the IDE.

Alternatively with CMake directly:

```bat
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

### Linux

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Or using the provided shell script:

```sh
bash run.sh
```

---

## Compiler Flags

| Flag | Purpose |
|---|---|
| `-march=native` | Enables SIMD (AVX2/FMA) for the host CPU |
| `-O0 -g` | Debug build — no optimization, full debug symbols |
| `-O3` | Release build — full optimization |

---

## Engine Lifecycle

The engine follows a simple explicit lifecycle:

```c
Engine engine = GetEngine();

InitializeEngine(engine);       // Create window, init subsystems

while (engine->isRunning) {
    HandleEngineInput(engine);  // Poll input
    UpdateEngine(engine);       // Update state, delta time
    RenderEngine(engine);       // Submit draw calls
}

DestroyEngine(engine);          // Free all GPU and CPU resources
```

---

## License

This project is licensed under the [MIT License](LICENSE.txt).