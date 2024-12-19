#pragma once

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#include <GLES3/gl32.h>
#else
#include <GL/glew.h>
#endif