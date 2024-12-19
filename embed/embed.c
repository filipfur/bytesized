#include "embed.h"

// clang-format off

const char __bdf__boxxy[] = {
#embed "assets/fonts/boxxy.bdf"
, '\0'
};
const uint32_t __bdf__boxxy_len = sizeof __bdf__boxxy;

const char __shaders__vert_screen[] = {
#embed "assets/shaders/screen.vert"
, '\0'
};
const char __shaders__vert_object[] = {
#embed "assets/shaders/object.vert"
, '\0'
};
const char __shaders__vert_anim[] = {
#embed "assets/shaders/anim.vert"
, '\0'
};
const char __shaders__vert_text[] = {
#embed "assets/shaders/text.vert"
, '\0'
};
const char __shaders__frag_object[] = {
#embed "assets/shaders/object.frag"
, '\0'
};
const char __shaders__frag_anim[] = {
#embed "assets/shaders/anim.frag"
, '\0'
};
const char __shaders__frag_text[] = {
#embed "assets/shaders/text.frag"
, '\0'
};
const char __shaders__frag_texture[] = {
#embed "assets/shaders/texture.frag"
, '\0'
};

// clang-format on
