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

add_library(bytesized_engine
    src/playercontroller.cpp
    src/persist.cpp
    src/console.cpp
    src/character.cpp
    src/panel.cpp
    src/playerinput.cpp
    src/gui.cpp
    src/gpu_primitive.cpp
    src/gpu_shader.cpp
    src/editor.cpp
    src/engine.cpp
    src/runtime.cpp
    src/skydome.cpp
    src/sprite.cpp
)
add_dependencies(bytesized_engine bytesized_assets)

target_include_directories(bytesized_engine PUBLIC ${BYTESIZED_DIR}/gcem/include include ${CMAKE_BINARY_DIR}/gen)
target_link_libraries(bytesized_engine PUBLIC bytesized_lib)

# TBD
# add_subdirectory(bytesized_import)
