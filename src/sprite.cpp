#include "sprite.h"
#include "gpu_primitive.h"
#include <algorithm>

void SpriteRenderer::create(float width, float height) {
    shaderProgram = gpu::createShaderProgram(
        gpu::builtinShader(gpu::SPRITE_VERT), gpu::builtinShader(gpu::SPRITE_FRAG),
        {{"u_projection", glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f)},
         {"u_view", glm::mat4{1.0f}},
         {"u_diffuse", 0},
         {"u_transform", glm::vec4{0.0f, 0.0f, 32.0f, 32.0f}},
         {"u_sprite", glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}},
         {"u_flip", glm::ivec2{0, 0}}});
}

void SpriteRenderer::update(uint16_t dt) {
    for (auto &sprite : sprites) {
        const auto &frames = sprite.animation_it->frames;
        if (sprite.frame_ms <= dt) {
            ++sprite.frame_it;
            if (sprite.frame_it == frames.end()) {
                sprite.frame_it = frames.begin();
            }
            sprite.frame_ms += sprite.frame_it->duration - dt;
        } else {
            sprite.frame_ms -= dt;
        }
    }
}

void SpriteRenderer::render() {
    shaderProgram->use();
    auto *plane = gpu::builtinPrimitives(gpu::BuiltinPrimitives::PLANE);
    std::for_each(sprites.begin(), sprites.end(), [this, plane](Sprite &sprite) {
        shaderProgram->uniforms.at("u_transform") << glm::vec4{sprite.translation, sprite.scale};
        shaderProgram->uniforms.at("u_sprite")
            << glm::vec4(sprite.frame_it->region.bottomLeft, sprite.frame_it->region.size);
        shaderProgram->uniforms.at("u_flip") << sprite.flip;
        sprite.animation_it->texture->bind();
        plane->render();
    });
}