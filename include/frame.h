#pragma once

#include "gpu.h"
#include "gpu_primitive.h"

struct Frame {
    void createPanel(float width, float height, gpu::Texture *texture) {
        node = gpu::createNode();
        node->scale = {width, height, 1.0f};
        node->mesh = gpu::createMesh();
        auto mat = gpu::createMaterial();
        mat->color = Color::white;
        mat->textures.emplace(GL_TEXTURE0, texture);
        node->mesh->primitives.emplace_back(gpu::builtinPrimitives(gpu::SPRITE), mat);
    }

    void createText(const bdf::Font &font, float x, float y, float scale, const char *value,
                    bool center, const Color &textColor) {
        text = gpu::createText(font, value, center);
        node = text->node;
        text->node->material(0)->color = textColor;
        text->node->translation = {x, y, 0.0f};
        text->node->scale = {scale, scale, 1.0f};
    }

    operator bool() { return node != nullptr; }

    Frame *setPosition(float x, float y) {
        node->translation = {x, y, 0.0f};
        return this;
    }

    Frame *setScale(float x, float y) {
        node->scale = {x, y, 1.0f};
        return this;
    }

    Frame *setHidden(bool hidden) {
        node->hidden = hidden;
        return this;
    }

    float width() { return node->scale.x; }

    float height() { return node->scale.y; }

    void renderPanels(gpu::ShaderProgram *shaderProgram) {
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

    void renderTexts(gpu::ShaderProgram *shaderProgram) {
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

    gpu::Node *node;
    gpu::Text *text;
    std::vector<Frame> children;
};