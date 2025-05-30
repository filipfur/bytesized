#pragma once

#include "gpu.h"

struct BlurRenderer {
    void create(const glm::ivec2 &resolution, gpu::ShaderProgram *shaderProgram_ = nullptr);

    gpu::Texture *blur(gpu::Texture *inputTexture);

    gpu::Texture *outputTexture();

    gpu::Framebuffer *blurFBO[2];
    gpu::ShaderProgram *shaderProgram;
};