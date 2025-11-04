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

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(game PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()