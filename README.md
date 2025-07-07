# Requirements
- **Cmake 3.12+**
- **C++23 compiler**
  - GCC 15+ / Clang 20+ (Linux)
  - MSVC 17+ (Windows)
- Vulkan SDK: Download from [LunarG](https://vulkan.lunarg.com/)

# Project Structure
```
kynetic/
├── src/
│   ├── Engine/
│   │   ├── Core/          # Application and window management
│   │   ├── Renderer/      # Vulkan Renderer
│   │   └── UI/            # ImGui
│   ├── main.cpp           # Entry point
│   └── pch.h              # Precompiled headers
├── CMakeLists.txt         # Build configuration
└── README.md
```

# Dependencies
All dependencies are automatically fetched via CMake FetchContent:
- `glfw`
- `vk-bootstrap`
- `imgui`
- `imguizmo`
- `fastgltf`
