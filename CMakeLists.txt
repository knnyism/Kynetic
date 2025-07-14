cmake_minimum_required(VERSION 3.12)
project(kynetic)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_program(GLSLC glslc HINTS
        ${Vulkan_GLSLC_EXECUTABLE}
        $ENV{VULKAN_SDK}/bin
        $ENV{VULKAN_SDK}/Bin
        $ENV{VULKAN_SDK}/x86_64/bin
        $ENV{VULKAN_SDK}/Bin32
)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK.")
endif()

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

# glm
FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# vk-bootstrap
FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG v1.4.319
)
FetchContent_MakeAvailable(vk-bootstrap)

# VMA (Vulkan Memory Allocator)
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

function(compile_shader target shader_file)
    get_filename_component(shader_name ${shader_file} NAME)
    set(output_file "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.spv")

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

    add_custom_command(
            OUTPUT ${output_file}
            COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file} -o ${output_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file}
            COMMENT "Compiling shader ${shader_name}"
    )

    target_sources(${target} PRIVATE ${output_file})
endfunction()

file(GLOB_RECURSE SHADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.comp"
)

add_custom_target(shaders)

foreach(shader ${SHADER_FILES})
    file(RELATIVE_PATH shader_rel ${CMAKE_CURRENT_SOURCE_DIR} ${shader})
    compile_shader(shaders ${shader_rel})
endforeach()

set(ENGINE_SOURCES
        src/pch.cpp
        src/pch.h
        src/Engine/Core/App.cpp
        src/Engine/Core/App.h
        src/Engine/Core/Window.cpp
        src/Engine/Core/Window.h
        src/Engine/Renderer/Renderer.cpp
        src/Engine/Renderer/Renderer.h
        src/Engine/Renderer/Swapchain.cpp
        src/Engine/Renderer/Swapchain.h
        src/Engine/Renderer/Device.cpp
        src/Engine/Renderer/Device.h
        src/Engine/Renderer/Shader.cpp
        src/Engine/Renderer/Shader.h
        src/Engine/Renderer/CommandPool.cpp
        src/Engine/Renderer/CommandPool.h
        src/Engine/UI/ImGuiRenderer.cpp
        src/Engine/UI/ImGuiRenderer.h
        src/Engine/Renderer/Buffer.cpp
        src/Engine/Renderer/Buffer.h
        src/Engine/Renderer/Common.h
)

add_executable(kynetic
        src/main.cpp
        ${ENGINE_SOURCES}
)

add_dependencies(kynetic shaders)

target_link_libraries(kynetic PRIVATE
        glfw
        vk-bootstrap
        vk-bootstrap-compiler-warnings
        Vulkan::Headers
        VulkanMemoryAllocator
        imgui
        imguizmo
        fastgltf
        glm::glm
)

target_compile_features(kynetic PRIVATE cxx_std_23)

target_include_directories(kynetic PRIVATE
        src
)

target_precompile_headers(kynetic PRIVATE
        "src/pch.h"
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