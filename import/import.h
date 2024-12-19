#pragma once

#include <cstddef>
#include <cstdint>

#if __has_include("import_assets.h")
#include "import_assets.h"

#ifndef __COLLECTIONS
#define __COLLECTIONS
#endif

#ifndef __IMAGES
#define __IMAGES
#endif

#ifndef __SHADERS
#define __SHADERS
#endif

#include "gpu.h"

namespace import {

enum class Collection {
#define __COLLECTION(handle, path) handle,
    __COLLECTIONS
#undef __COLLECTION
        COUNT
};

enum class Image {
#define __IMAGE(handle, path) handle,
    __IMAGES
#undef __IMAGE
        COUNT
};

enum class Shader {
#define __SHADER(handle, path, type) handle,
    __SHADERS
#undef __SHADER
        COUNT
};

// load collection from file
const char *get_path(import::Collection collection);
const uint8_t *load(import::Collection collection);

// get path to image file
const char *get_path(import::Image image);

// load all shaders from files
void loadShaders();

// access shaders
gpu::Shader *shader(Shader shader);

} // namespace import

#else
#error "Failed to find import_assets.h needed by bytesized_use_import."
#endif