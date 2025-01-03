#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char __bdf__boxxy[];
extern const uint32_t __bdf__boxxy_len;

extern const char __shaders__vert_screen[];
extern const char __shaders__vert_object[];
extern const char __shaders__vert_anim[];
extern const char __shaders__vert_text[];
extern const char __shaders__frag_object[];
extern const char __shaders__frag_anim[];
extern const char __shaders__frag_text[];
extern const char __shaders__frag_texture[];

#ifdef __cplusplus
}
#endif
