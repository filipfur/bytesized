#include "sprite.h"
#include "gpu.h"
#include <algorithm>

static const char *_spriteVert = BYTESIZED_GLSL_VERSION R"(
layout (location=0) in vec4 aXYUV;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;
uniform ivec2 u_flip;

out vec2 UV;

void main()
{
    UV = aXYUV.zw;
    if(u_flip.x > 0) {
        UV.x = 1.0 - UV.x;
    }
    if(u_flip.y > 0) {
        UV.y = 1.0 - UV.y;
    }

    vec3 p = vec3(u_model * vec4(aXYUV.xy, 0.0, 1.0));
    gl_Position = u_projection * u_view * vec4(p, 1.0);
}
)";

static const char *_spriteFrag = BYTESIZED_GLSL_VERSION R"(
precision highp float;

out vec4 FragColor;
  
in vec2 UV;

uniform sampler2D u_texture;
uniform vec4 u_sprite;

void main()
{
    vec2 xy = u_sprite.xy;
    vec2 uv = u_sprite.zw;

    vec4 diffuse = texture(u_texture, UV * uv + xy);
    if(diffuse.a < 0.1) {
        discard;
    }
    FragColor = diffuse;
}
)";

// clang-format off
static const float _spriteVertices[] = {
    -0.5f, -0.5f, 0.0f, 0.0f,
    +0.5f, -0.5f, 1.0f, 0.0f,
    -0.5f, +0.5f, 0.0f, 1.0f,
    +0.5f, +0.5f, 1.0f, 1.0f,
    -0.5f, +0.5f, 0.0f, 1.0f,
    +0.5f, -0.5f, 1.0f, 0.0f,
};
// clang-format on

SpriteRenderer::SpriteRenderer(float width, float height)
    : projection{glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f)} {}

void SpriteRenderer::update(uint16_t dt) {
    for (auto &sprite : sprites) {
        auto &frames = sprite.animation_it->frames;
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
    static gpu::ShaderProgram *_shaderProgram = nullptr;
    static gpu::VertexArray *_planeVAO = nullptr;

    if (_shaderProgram == nullptr) {
        _shaderProgram =
            gpu::createShaderProgram(gpu::createShader(GL_VERTEX_SHADER, _spriteVert),
                                     gpu::createShader(GL_FRAGMENT_SHADER, _spriteFrag),
                                     {{"u_projection", projection},
                                      {"u_view", glm::mat4{1.0f}},
                                      {"u_diffuse", 0},
                                      {"u_model", glm::mat4{1.0f}},
                                      {"u_sprite", glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}},
                                      {"u_flip", glm::ivec2{0, 0}}});
    }
    if (_planeVAO == nullptr) {
        _planeVAO = gpu::createVertexArray();
        _planeVAO->bind();
        uint32_t *vbo = gpu::createVertexBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_spriteVertices), _spriteVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    }

    _shaderProgram->use();
    _shaderProgram->uniforms.at("u_projection") << projection;
    std::for_each(sprites.begin(), sprites.end(), [](Sprite &sprite) {
        if (sprite.hidden) {
            return;
        }
        _shaderProgram->uniforms.at("u_model") << sprite.model();
        _shaderProgram->uniforms.at("u_sprite")
            << glm::vec4(sprite.frame_it->region.bottomLeft, sprite.frame_it->region.size);
        _shaderProgram->uniforms.at("u_flip") << sprite.flip;
        sprite.animation_it->texture->bind();

        _planeVAO->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    });
}