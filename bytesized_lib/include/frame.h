#pragma once

#include "gpu.h"

struct Frame {
    void createPanel(float width, float height, gpu::Texture *texture);
    void createText(const bdf::Font &font, float x, float y, float scale, const char *value,
                    bool center, const Color &textColor);

    operator bool() { return node != nullptr; }

    Frame *setPosition(float x, float y);
    Frame *setScale(float x, float y);
    Frame *setHidden(bool hidden);

    float x() const;
    float y() const;

    float width() const;
    float height() const;

    void renderPanels(gpu::ShaderProgram *shaderProgram);
    void renderTexts(gpu::ShaderProgram *shaderProgram);

    gpu::Node *node;
    gpu::Text *text;
    std::vector<Frame> children;
};