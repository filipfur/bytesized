cmake_minimum_required(VERSION 3.15)
project(bytesized)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    src/collision_primitive.cpp
)

add_subdirectory(embed)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

target_include_directories(bytesized PUBLIC glm gcem/include stb ${SDL2_INCLUDE_DIR} include)
set(LIBS ${SDL2_LIBRARY} GLEW::GLEW bytesized_embed)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LIBS ${LIBS} m GL)
endif()
target_link_libraries(bytesized PUBLIC ${LIBS})


include(cmake/API.cmake)
