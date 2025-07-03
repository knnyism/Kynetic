cmake_minimum_required(VERSION 3.12)
project(kynetic)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include(FetchContent)

# glfw
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.3.10
)
FetchContent_MakeAvailable(glfw)

# vk-bootstrap
FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG v1.4.319
)
FetchContent_MakeAvailable(vk-bootstrap)


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
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
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
        glfw
        Vulkan::Vulkan
)

# ImGuizmo
FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
)
FetchContent_MakeAvailable(imguizmo)

add_library(imguizmo STATIC
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)

target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
)

target_link_libraries(imguizmo PUBLIC
        imgui
)

# fastgltf
FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(fastgltf)

file(GLOB_RECURSE ENGINE_SOURCES "Source/**/*.cpp")

add_executable(kynetic Source/main.cpp
        Include/pch.h
        ${ENGINE_SOURCES})

target_link_libraries(kynetic PRIVATE
        glfw
        vk-bootstrap
        vk-bootstrap-compiler-warnings
        Vulkan::Headers
        imgui
        imguizmo
        fastgltf
)

target_compile_features(kynetic PRIVATE cxx_std_26)

target_include_directories(kynetic PRIVATE
        Include
)

target_precompile_headers(kynetic PRIVATE
        <pch.h>
)

target_compile_definitions(kynetic PRIVATE
        KYNETIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if (WIN32)
    target_compile_definitions(kynetic PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(kynetic PUBLIC ${CMAKE_DL_LIBS})
    target_compile_definitions(kynetic PRIVATE VK_USE_PLATFORM_XLIB_KHR)
elseif (APPLE)
    target_compile_definitions(kynetic PRIVATE VK_USE_PLATFORM_MACOS_MVK)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(kynetic PRIVATE DEBUG)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(kynetic PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif ()