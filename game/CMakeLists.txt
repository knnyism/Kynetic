cmake_minimum_required(VERSION 3.12)
project(game)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../kynetic ${CMAKE_CURRENT_BINARY_DIR}/kynetic)

add_executable(game
        src/main.cpp
)

target_link_libraries(game PRIVATE
        kynetic
)

target_include_directories(game PRIVATE
        src
)

set(KYNETIC_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../kynetic/assets)
set(GAME_SHARED_ASSETS_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/shared_assets)

file(MAKE_DIRECTORY ${GAME_SHARED_ASSETS_DIR})

file(GLOB_RECURSE KYNETIC_ASSET_FILES
        "${KYNETIC_ASSETS_DIR}/*"
)

add_custom_target(copy_kynetic_assets ALL
        COMMENT "Copying kynetic assets to game/assets/shared_assets"
)

add_custom_command(
        TARGET copy_kynetic_assets POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${KYNETIC_ASSETS_DIR}
        ${GAME_SHARED_ASSETS_DIR}
        COMMENT "Copying assets from kynetic to shared_assets"
)

add_dependencies(game copy_kynetic_assets)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(game PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()