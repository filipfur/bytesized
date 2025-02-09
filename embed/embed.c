#include "embed.h"

// clang-format off

const char __bdf__boxxy[] = {
#embed "assets/fonts/boxxy.bdf"
, '\0'
};
const uint32_t __bdf__boxxy_len = sizeof __bdf__boxxy;

const char __png__ui[] = {
#embed "assets/ui.png"
, '\0'
};
const uint32_t __png__ui_len = sizeof __png__ui;

const char __png__console[] = {
#embed "assets/console.png"
, '\0'
};
const uint32_t __png__console_len = sizeof __png__console;

const char __png__tframe[] = {
#embed "assets/tframe.png"
, '\0'
};
const uint32_t __png__tframe_len = sizeof __png__tframe;

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
const char __shaders__vert_ui[] = {
#embed "assets/shaders/ui.vert"
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
