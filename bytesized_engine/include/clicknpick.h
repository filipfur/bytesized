#pragma once

#include "gpu.h"
#include "window.h"
#include <cstring>

struct IClickNPick {
    virtual bool nodeClicked(gpu::Node *node) = 0;
};

#define CLICKNPICK_COUNT UCHAR_MAX

static const char *clickNPickFrag = R"(#version 330 core
out vec4 FragColor;
in vec2 UV;
uniform float u_object_id;
void main()
{ 
    FragColor.r = u_object_id / 255.0;
}
)";

struct ClickNPick : public window::IMouseListener, public window::IMouseMotionListener {

    ClickNPick(IClickNPick *iClickNPick_) : iClickNPick{iClickNPick_} {}

    void create(uint32_t width, uint32_t height, gpu::Shader *vertexShader,
                gpu::Shader *animVertexShader, const glm::mat4 &projection, float scale) {
        window::registerMouseListener(this);
        window::registerMouseMotionListener(this);
        _width = width;
        _height = height;
        _scale = scale;
        fbo = gpu::createFramebuffer();
        fbo->bind();
        fbo->createTexture(GL_COLOR_ATTACHMENT0, width * scale, height * scale,
                           gpu::ChannelSetting::R, GL_UNSIGNED_BYTE);
        fbo->createRenderBufferDS(width * scale, height * scale);
        fbo->checkStatus();
        fbo->unbind();
        gpu::Shader *clickNPickShader = gpu::createShader(GL_FRAGMENT_SHADER, clickNPickFrag);
        shaderProgram = gpu::createShaderProgram(
            vertexShader, clickNPickShader, {{"u_model", glm::mat4{1.0f}}, {"u_object_id", 0.0f}});
        gpu::builtinUBO(gpu::UBO_CAMERA)->bindShaders({shaderProgram});
        if (animVertexShader) {
            animProgram = gpu::createShaderProgram(animVertexShader, clickNPickShader,
                                                   {{"u_projection", projection},
                                                    {"u_view", glm::mat4{1.0f}},
                                                    {"u_model", glm::mat4{1.0f}},
                                                    {"u_object_id", 0.0f}});
            gpu::builtinUBO(gpu::UBO_SKINNING)->bindShaders({animProgram});
            gpu::builtinUBO(gpu::UBO_CAMERA)->bindShaders({animProgram});
        }
    }

    void registerNode(gpu::Node *node) {
        for (uint8_t i{0}; i < CLICKNPICK_COUNT; ++i) {
            if (nodes[i] == nullptr) {
                nodes[i] = node;
                return;
            }
        }
        assert(false); // full
    }

    void unregisterNode(gpu::Node *node) {
        for (uint8_t i{0}; i < CLICKNPICK_COUNT; ++i) {
            if (nodes[i] == node) {
                nodes[i] = nullptr;
                return;
            }
        }
        assert(false); // not registered
    }

    void clear() {
        for (size_t i{0}; i < CLICKNPICK_COUNT; ++i) {
            nodes[i] = nullptr;
        }
    }

    void update(const glm::mat4 &view) {
        fbo->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        static int vp[4] = {};
        glGetIntegerv(GL_VIEWPORT, vp);
        glViewport(0, 0, _width * _scale, _height * _scale);
        shaderProgram->use();
        for (uint8_t i{0}; i < CLICKNPICK_COUNT; ++i) {
            if (auto node = nodes[i]) {
                if (node->skin) {
                    continue;
                }
                shaderProgram->uniforms.at("u_object_id") << static_cast<float>(i + 1);
                node->render(shaderProgram);
            } else {
                break;
            }
        }
        if (animProgram) {
            animProgram->use();
            animProgram->uniforms.at("u_view") << view;
            for (uint8_t i{0}; i < CLICKNPICK_COUNT; ++i) {
                if (auto node = nodes[i]) {
                    if (node->skin) {
                        animProgram->uniforms.at("u_object_id") << static_cast<float>(i + 1);
                        node->render(animProgram);
                    }
                } else {
                    break;
                }
            }
        }
        glReadPixels((int)(mx * _scale), (int)(my * _scale), 1, 1, GL_RED, GL_UNSIGNED_BYTE,
                     &objectId);
        fbo->unbind();
        glViewport(vp[0], vp[1], vp[2], vp[3]);
    }

    bool mouseDown(int /*button*/, int /*x*/, int /*y*/) override {
        if (objectId > 0) {
            auto *clicked = nodes[objectId - 1];
            assert(clicked);
            iClickNPick->nodeClicked(clicked);
        }
        return false;
    }
    bool mouseUp(int /*button*/, int /*x*/, int /*y*/) override { return false; }
    bool mouseMoved(float x, float y, float /*xrel*/, float /*yrel*/) override {
        mx = x;
        my = _height - y;
        return false;
    }

    IClickNPick *iClickNPick;
    uint32_t _width;
    uint32_t _height;
    float _scale;
    gpu::ShaderProgram *shaderProgram{nullptr};
    gpu::ShaderProgram *animProgram{nullptr};
    gpu::Framebuffer *fbo{nullptr};
    gpu::Node *nodes[CLICKNPICK_COUNT] = {};
    float mx;
    float my;
    uint8_t objectId{0};
};