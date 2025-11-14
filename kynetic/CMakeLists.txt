cmake_minimum_required(VERSION 3.12)
project(kynetic_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include(FetchContent)

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
)

target_link_libraries(imgui PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
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
        GIT_TAG v0.8.0
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

# slang
if (WIN32)
    set(SLANG_PLATFORM "windows-x86_64")
elseif (UNIX AND NOT APPLE)
    set(SLANG_PLATFORM "linux-x86_64")
elseif (APPLE)
    set(SLANG_PLATFORM "macos-aarch64")
endif ()

FetchContent_Declare(
        slang
        URL https://github.com/shader-slang/slang/releases/download/v2025.21.1/slang-2025.21.1-${SLANG_PLATFORM}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(slang)

add_library(slang INTERFACE)
target_include_directories(slang INTERFACE ${slang_SOURCE_DIR}/include)

find_library(SLANG_LIBRARY
        NAMES slang libslang
        PATHS ${slang_SOURCE_DIR}/lib
        NO_DEFAULT_PATH
        REQUIRED
)

target_link_libraries(slang INTERFACE ${SLANG_LIBRARY})

file(GLOB_RECURSE KYNETIC_SOURCES "src/*.cpp")
file(GLOB_RECURSE KYNETIC_HEADERS "src/*.hpp")

add_library(kynetic STATIC
        ${KYNETIC_SOURCES}
        ${KYNETIC_HEADERS}
)

target_link_libraries(kynetic PUBLIC
        SDL3::SDL3
        vk-bootstrap
        vk-bootstrap-compiler-warnings
        Vulkan::Headers
        VulkanMemoryAllocator
        imgui
        fastgltf
        glm::glm
        fmt::fmt
        stb_image
        slang
)

target_compile_features(kynetic PUBLIC cxx_std_20)

target_include_directories(kynetic PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
)

target_precompile_headers(kynetic PUBLIC
        src/pch.hpp
)

if (WIN32)
    target_compile_definitions(kynetic PUBLIC VK_USE_PLATFORM_WIN32_KHR)
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