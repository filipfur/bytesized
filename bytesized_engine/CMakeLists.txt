set(assets
    ${BYTESIZED_ASSETS}/shaders/anim.frag
    ${BYTESIZED_ASSETS}/shaders/anim.vert
    ${BYTESIZED_ASSETS}/shaders/object.frag
    ${BYTESIZED_ASSETS}/shaders/object.vert
    ${BYTESIZED_ASSETS}/shaders/screen.vert
    ${BYTESIZED_ASSETS}/shaders/text.frag
    ${BYTESIZED_ASSETS}/shaders/text.vert
    ${BYTESIZED_ASSETS}/shaders/texture.frag
    ${BYTESIZED_ASSETS}/shaders/ui.vert
    ${BYTESIZED_ASSETS}/shaders/billboard.vert
    ${BYTESIZED_ASSETS}/icons/console.png
    ${BYTESIZED_ASSETS}/icons/tframe.png
    ${BYTESIZED_ASSETS}/icons/ui.png
    ${BYTESIZED_ASSETS}/fonts/boxxy.bdf
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
    src/builtin_shader.cpp
    src/editor.cpp
    src/engine.cpp
    src/runtime.cpp
)
add_dependencies(bytesized_engine bytesized_assets)

target_include_directories(bytesized_engine PUBLIC include ${CMAKE_BINARY_DIR}/gen)
target_link_libraries(bytesized_engine PUBLIC bytesized_lib)

# TBD
# add_subdirectory(bytesized_import)
