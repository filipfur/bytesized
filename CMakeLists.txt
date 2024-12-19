cmake_minimum_required(VERSION 3.15)
project(bytesized)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(BYTESIZED_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(BYTESIZED_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT BYTESIZED_LOG_LEVEL)
    set(BYTESIZED_LOG_LEVEL 4)
endif()

# -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=
add_compile_options(-Wall -Wextra -pedantic -Werror -DLOG_LEVEL=${BYTESIZED_LOG_LEVEL} -DBYTESIZED_DIR=${BYTESIZED_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

add_library(bytesized
    stb/stb_image.cpp
    src/library.cpp
    src/window.cpp
    src/gpu.cpp
    src/uniform.cpp
    src/shader.cpp
    src/trs.cpp
    src/color.cpp
    src/time.cpp
    src/timer.cpp
    src/texture.cpp
    src/animation.cpp
    src/assets.cpp
    src/ecs.cpp
    src/filesystem.cpp
    src/persist.cpp
    src/bdf.cpp
    src/text.cpp
    src/console.cpp
    src/editor.cpp
    src/gui.cpp
    src/engine.cpp
    src/primitive.cpp
)

add_subdirectory(embed)

target_include_directories(bytesized PUBLIC glm gcem/include stb ${SDL2_INCLUDE_DIR} include)
target_link_libraries(bytesized PUBLIC ${SDL2_LIBRARY} GLEW::GLEW bytesized_embed)

include(cmake/API.cmake)
