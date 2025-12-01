cmake_minimum_required(VERSION 3.12)
project(kynetic_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include(FetchContent)

# Volk
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG 1.4.304
)
set(VOLK_STATIC_DEFINES ON)
set(VOLK_INSTALL OFF)
FetchContent_MakeAvailable(volk)

# SDL3
FetchContent_Declare(
        sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)

FetchContent_MakeAvailable(sdl3)

# vk-bootstrap
FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG v1.4.319
)
# set(VK_BOOTSTRAP_VULKAN_HEADER_DIR ${Vulkan_INCLUDE_DIRS})
FetchContent_MakeAvailable(vk-bootstrap)

# vma (Vulkan Memory Allocator)
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(vma)

target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE
        ${vma_SOURCE_DIR}/include
)

# ImGui (docking)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC
        IMGUI_DEFINE_MATH_OPERATORS
        VK_NO_PROTOTYPES
)

target_link_libraries(imgui PUBLIC
        SDL3::SDL3
        Vulkan::Headers
        volk
)

# glm
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# fastgltf
FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v0.9.0
)
FetchContent_MakeAvailable(fastgltf)

# fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
)
FetchContent_MakeAvailable(fmt)

# stb_image
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})

# flecs
FetchContent_Declare(
        flecs
        GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
        GIT_TAG v4.1.2
)

set(FLECS_STATIC ON)
set(FLECS_SHARED OFF)
set(FLECS_PIC ON)
set(FLECS_TESTS OFF)
set(FLECS_EXAMPLES OFF)

FetchContent_MakeAvailable(flecs)

# magic_enum
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

# MikkTSpace
FetchContent_Declare(
        mikktspace
        GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git
        GIT_TAG master
)
FetchContent_MakeAvailable(mikktspace)

add_library(mikktspace STATIC
        ${mikktspace_SOURCE_DIR}/mikktspace.c
)
target_include_directories(mikktspace PUBLIC ${mikktspace_SOURCE_DIR})

# meshoptimizer
FetchContent_Declare(
        meshoptimizer
        GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
        GIT_TAG v0.25
)
FetchContent_MakeAvailable(meshoptimizer)

# slang
if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SLANG_PLATFORM "windows-x86_64")
    else ()
        set(SLANG_PLATFORM "windows-x86")
    endif ()
elseif (UNIX AND NOT APPLE)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SLANG_PLATFORM "linux-x86_64")
    else ()
        set(SLANG_PLATFORM "linux-x86")
    endif ()
elseif (APPLE)
    # Check for Apple Silicon vs Intel
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(SLANG_PLATFORM "macos-aarch64")
    else ()
        set(SLANG_PLATFORM "macos-x86_64")
    endif ()
endif ()

message(STATUS "Detected Slang platform: ${SLANG_PLATFORM}")

FetchContent_Declare(
        slang
        URL https://github.com/shader-slang/slang/releases/download/v2025.22.1/slang-2025.22.1-${SLANG_PLATFORM}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(slang)

add_library(slang INTERFACE)
target_include_directories(slang INTERFACE ${slang_SOURCE_DIR}/include)

if (WIN32)
    set(SLANG_LIB_NAMES slang.lib slang.dll.lib)
    set(SLANG_LIB_DIR ${slang_SOURCE_DIR}/lib ${slang_SOURCE_DIR}/bin)
else ()
    set(SLANG_LIB_NAMES slang libslang libslang.so libslang.dylib)
    set(SLANG_LIB_DIR ${slang_SOURCE_DIR}/lib ${slang_SOURCE_DIR}/lib64)
endif ()

find_library(SLANG_LIBRARY
        NAMES ${SLANG_LIB_NAMES}
        PATHS ${SLANG_LIB_DIR}
        NO_DEFAULT_PATH
        REQUIRED
)

message(STATUS "Found Slang library: ${SLANG_LIBRARY}")

target_link_libraries(slang INTERFACE ${SLANG_LIBRARY})

if (WIN32)
    file(GLOB SLANG_DLLS "${slang_SOURCE_DIR}/bin/*.dll")
    if (SLANG_DLLS)
        foreach (DLL ${SLANG_DLLS})
            message(STATUS "Copying Slang DLL: ${DLL}")
            configure_file(${DLL} ${CMAKE_BINARY_DIR} COPYONLY)
        endforeach ()
    endif ()
endif ()

add_library(kynetic STATIC
        assets/shaders/shader_types.hpp
        src/core/components.hpp
        src/core/device.cpp
        src/core/device.hpp
        src/core/engine.cpp
        src/core/engine.hpp
        src/core/input.cpp
        src/core/input.hpp
        src/core/renderer.cpp
        src/core/renderer.hpp
        src/core/resource_manager.cpp
        src/core/resource_manager.hpp
        src/core/scene.cpp
        src/core/scene.hpp
        src/rendering/command_buffer.cpp
        src/rendering/command_buffer.hpp
        src/rendering/mesh.cpp
        src/rendering/mesh.hpp
        src/rendering/model.cpp
        src/rendering/model.hpp
        src/rendering/pipeline.cpp
        src/rendering/pipeline.hpp
        src/rendering/shader.cpp
        src/rendering/shader.hpp
        src/rendering/swapchain.cpp
        src/rendering/swapchain.hpp
        src/rendering/texture.cpp
        src/rendering/texture.hpp
        src/rendering/material.cpp
        src/rendering/material.hpp
        src/rendering/descriptor.cpp
        src/rendering/descriptor.hpp
        src/vma_usage.cpp
        src/vma_usage.hpp
        src/pch.cpp
        src/pch.hpp
)

target_link_libraries(kynetic PUBLIC
        SDL3::SDL3
        vk-bootstrap
        vk-bootstrap-compiler-warnings
        Vulkan::Headers
        volk
        VulkanMemoryAllocator
        imgui
        fastgltf
        glm::glm
        fmt::fmt
        stb_image
        slang
        flecs::flecs_static
        magic_enum::magic_enum
        mikktspace
        meshoptimizer
)

target_compile_features(kynetic PUBLIC cxx_std_20)

target_include_directories(kynetic PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
)

target_precompile_headers(kynetic PUBLIC
        src/pch.hpp
)

target_compile_definitions(kynetic PUBLIC
        VK_NO_PROTOTYPES
        VMA_DYNAMIC_VULKAN_FUNCTIONS
        VMA_STATIC_VULKAN_FUNCTIONS=0
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
)

if (WIN32)
    target_compile_definitions(kynetic PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    # Add Windows-specific runtime library settings
    set_property(TARGET kynetic PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif (UNIX AND NOT APPLE)
    target_link_libraries(kynetic PUBLIC ${CMAKE_DL_LIBS})
    target_compile_definitions(kynetic PUBLIC VK_USE_PLATFORM_XLIB_KHR)
elseif (APPLE)
    target_compile_definitions(kynetic PUBLIC VK_USE_PLATFORM_MACOS_MVK)
endif ()

if (MSVC)
    target_compile_options(kynetic PRIVATE /W4 /WX)
else ()
    target_compile_options(kynetic PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-missing-field-initializers)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(kynetic PUBLIC DEBUG)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(kynetic PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()