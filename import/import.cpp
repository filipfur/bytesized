#include "import.h"

#include <stddef.h>
#include <stdio.h>

#if __has_include("import_assets.h")

#include "filesystem.h"
#include "library.h"
#include "opengl.h"

const char *import::get_path(import::Collection collection) {
    static const char *_paths[] = {
#define __COLLECTION(handle, path) path,
        __COLLECTIONS
#undef __COLLECTION
        "\0",
    };
    return _paths[static_cast<size_t>(collection)];
}
const uint8_t *import::load(import::Collection collection) {

    return (uint8_t *)filesystem::loadFile(get_path(collection)).data();
}

const char *import::get_path(import::Image image) {
    static const char *_paths[] = {
#define __IMAGE(handle, path) path,
        __IMAGES
#undef __IMAGE
        "\0",
    };
    return _paths[static_cast<size_t>(image)];
}

gpu::Shader **_shaders_start = nullptr;
void import::loadShaders() {
    static gpu::Shader *_shaders[] = {
#define __SHADER(handle, path, type) gpu::createShader(type, filesystem::loadFile(path).data()),
        __SHADERS
#undef __SHADER
        nullptr,
    };
    _shaders_start = _shaders;
}

gpu::Shader *import::shader(import::Shader shader) {
    return _shaders_start[static_cast<size_t>(shader)];
}

#endif