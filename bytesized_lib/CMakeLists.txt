add_library(bytesized_lib
    ${BYTESIZED_DIR}/stb/stb_image.cpp
    src/geom_obb.cpp
    src/assets.cpp
    src/geom_aabb.cpp
    src/text.cpp
    src/geom_sphere.cpp
    src/geom_plane.cpp
    src/gpu.cpp
    src/library.cpp
    src/uniform.cpp
    src/camera.cpp
    src/window.cpp
    src/timer.cpp
    src/texture.cpp
    src/color.cpp
    src/bdf.cpp
    src/ecs.cpp
    src/geom_collision.cpp
    src/filesystem.cpp
    src/geom_primitive.cpp
    src/simplex_noise.cpp
    src/trs.cpp
    src/polygonize.cpp
    src/geom_convexhull.cpp
    src/time.cpp
)

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

target_include_directories(bytesized_lib PUBLIC ${BYTESIZED_DIR}/glm ${BYTESIZED_DIR}/stb ${SDL2_INCLUDE_DIR} include)

set(BYTESIZED_EXT_LIBS ${SDL2_LIBRARY} GLEW::GLEW)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(BYTESIZED_EXT_LIBS ${LIBS} m GL)
endif()
target_link_libraries(bytesized_lib PUBLIC ${BYTESIZED_EXT_LIBS})
