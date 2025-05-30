#pragma once

#include "gpu.h"
#include "window.h"
#include <cstring>

struct IClickNPick {
    virtual void renderClickables(gpu::ShaderProgram *shaderProgram, uint8_t &id) = 0;
    virtual bool nodeClicked(size_t index) = 0;
};

struct ClickNPick : public window::IMouseListener, public window::IMouseMotionListener {

    ClickNPick(IClickNPick *iClickNPick_) : iClickNPick{iClickNPick_} {}

    void create(uint32_t width, uint32_t height, const glm::mat4 &projection, float scale,
                gpu::ShaderProgram *shaderProgram) {
        window::registerMouseListener(this);
        window::registerMouseMotionListener(this);
        _width = width;
        _height = height;
        _scale = scale;
        fbo = gpu::createFramebuffer();
        fbo->bind();
        fbo->createTexture(GL_COLOR_ATTACHMENT0, width * scale, height * scale,
                           gpu::ChannelSetting::RGB, GL_UNSIGNED_BYTE);
        fbo->createRenderBufferDS(width * scale, height * scale);
        // fbo->createDepthStencil(width * scale, height * scale);
        // unsigned int attachments[1] = {GL_COLOR_ATTACHMENT0};
        // glDrawBuffers(1, attachments);
        fbo->checkStatus("clicknpick.fbo");
        fbo->unbind();
        shaderPrograms.push_back(shaderProgram);
    }

    void update() {
        fbo->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        static int vp[4] = {};
        glGetIntegerv(GL_VIEWPORT, vp);
        glViewport(0, 0, _width * _scale, _height * _scale);
        uint8_t id = 1;
        for (auto *shaderProgram : shaderPrograms) {
            shaderProgram->use();
            assert(id < 255);
            iClickNPick->renderClickables(shaderProgram, id);
        }
        uint8_t pixel[4];
        glReadPixels((int)(mx * _scale), (int)(my * _scale), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixel);
        objectId = pixel[0];
        fbo->unbind();
        glViewport(vp[0], vp[1], vp[2], vp[3]);
    }

    bool mouseDown(int /*button*/, int /*x*/, int /*y*/) override {
        if (objectId > 0) {
            return iClickNPick->nodeClicked(objectId - 1);
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
    std::vector<gpu::ShaderProgram *> shaderPrograms;
    gpu::Framebuffer *fbo{nullptr};
    float mx;
    float my;
    uint8_t objectId{0};
};