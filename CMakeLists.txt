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
add_compile_options(-Wall -Wshadow -Wextra -pedantic -Werror -DLOG_LEVEL=${BYTESIZED_LOG_LEVEL} -DBYTESIZED_DIR=${BYTESIZED_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

add_subdirectory(embed)
set(assets
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/anim.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/anim.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/object.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/object.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/screen.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/text.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/text.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/texture.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/sprite.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/sprite.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/ui.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/billboard.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/console.png
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/tframe.png
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/ui.png
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/boxxy.bdf
)
embed_assets("${assets}" _bytesized_assets_headers)
add_custom_target(bytesized_assets DEPENDS ${_bytesized_assets_headers})

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
    src/gpu_primitive.cpp
    src/geom_primitive.cpp
    src/geom_plane.cpp
    src/geom_sphere.cpp
    src/geom_aabb.cpp
    src/geom_obb.cpp
    src/polygonize.cpp
    src/simplex_noise.cpp
    src/geom_convexhull.cpp
    src/geom_collision.cpp
    src/camera.cpp
    src/character.cpp
    src/playercontroller.cpp
    src/skydome.cpp
    src/playerinput.cpp
    src/runtime.cpp
    src/panel.cpp
    src/sprite.cpp
)
add_dependencies(bytesized bytesized_assets)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

target_include_directories(bytesized PUBLIC glm gcem/include stb ${SDL2_INCLUDE_DIR} include ${CMAKE_BINARY_DIR}/gen)
set(LIBS ${SDL2_LIBRARY} GLEW::GLEW)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LIBS ${LIBS} m GL)
endif()
target_link_libraries(bytesized PUBLIC ${LIBS})

add_subdirectory(import)
