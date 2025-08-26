#include "frame.h"

struct VertexData {
    constexpr VertexData()
        : positions{
              {0.0f, 0.0f, 0.0f},
              {1.0f, 0.0f, 0.0f},
              {0.0f, 1.0f, 0.0f},
              {1.0f, 1.0f, 0.0f},
          },
          normals{
              {0.0f, 0.0f, 1.0f},
              {0.0f, 0.0f, 1.0f},
              {0.0f, 0.0f, 1.0f},
              {0.0f, 0.0f, 1.0f},
          },
          uvs{
              {0.0f, 0.0f},
              {1.0f, 0.0f},
              {0.0f, 1.0f},
              {1.0f, 1.0f},
          },
          indices{0, 1, 2, 1, 3, 2} {}
    glm::vec3 positions[4];
    glm::vec3 normals[4];
    glm::vec2 uvs[4];
    uint16_t indices[6];
};
static constexpr VertexData _vertexData;

static gpu::Primitive *_framePrimitive() {
    static gpu::Primitive *primitive = nullptr;
    if (primitive == nullptr) {
        primitive = gpu::createPrimitive(_vertexData.positions, _vertexData.normals,
                                         _vertexData.uvs, 4, _vertexData.indices, 6);
    }
    return primitive;
}

void Frame::createPanel(float width, float height, gpu::Texture *texture) {
    node = gpu::createNode();
    node->scale = {width, height, 1.0f};
    node->mesh = gpu::createMesh();
    auto mat = gpu::createMaterial();
    mat->color = Color::white;
    mat->textures.emplace(GL_TEXTURE0, texture);
    node->mesh->primitives.emplace_back(_framePrimitive(), mat);
}

void Frame::createText(const bdf::Font &font, float x, float y, float scale, const char *value,
                       bool center, const Color &textColor) {
    text = gpu::createText(font, value, center);
    node = text->node;
    text->node->material(0)->color = textColor;
    text->node->translation = {x, y, 0.0f};
    text->node->scale = {scale, scale, 1.0f};
}

Frame *Frame::setPosition(float x, float y) {
    node->translation = {x, y, 0.0f};
    return this;
}

Frame *Frame::setScale(float x, float y) {
    node->scale = {x, y, 1.0f};
    return this;
}

Frame *Frame::setHidden(bool hidden) {
    node->hidden = hidden;
    return this;
}

float Frame::x() const { return node->translation.x; }

float Frame::y() const { return node->translation.y; }

float Frame::width() const { return node->scale.x; }

float Frame::height() const { return node->scale.y; }

void Frame::renderPanels(gpu::ShaderProgram *shaderProgram) {
    if (node->hidden) {
        return;
    }
    if (text) {
        return;
    }
    for (auto &child : children) {
        if (child.text == nullptr) {
            child.renderPanels(shaderProgram);
        }
    }
    node->render(shaderProgram);
}

void Frame::renderTexts(gpu::ShaderProgram *shaderProgram) {
    if (node->hidden) {
        return;
    }
    if (text) {
        text->node->render(shaderProgram);
    } else {
        for (auto &child : children) {
            child.renderTexts(shaderProgram);
        }
    }
}