#include "gui.h"
#include "embed.h"
#include "primitive.h"
#include "systime.h"

#include <glm/gtc/quaternion.hpp>

static gpu::Node *_createFrame(float width, float height, const glm::vec2 &frameDim,
                               gpu::Texture *texture) {
    gpu::Node *frame = gpu::createNode();
    frame->scale = {frameDim.x, frameDim.y, 1.0f};
    frame->mesh = gpu::createMesh();
    auto mat = gpu::createMaterial();
    mat->color = Color::white;
    mat->textures.emplace(GL_TEXTURE0, texture);
    frame->mesh->primitives.emplace_back(gpu::builtinPrimitives(gpu::SPRITE), mat);
    return frame;
}

void GUI::create(bdf::Font *font_, float width_, float height_, float em_, Options options) {
    font = font_;
    width = width_;
    height = height_;
    em = em_;
    projection = glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    view = glm::mat4{1.0f};
    if (options & TITLE) {
        titleText = gpu::createText(*font, "untitled*", true);
        titleText->node->material(0)->color = Color::black;
        titleText->node->translation = {width * 0.5f, height - 16.0f - font->ph * em, 0.0f};
        titleText->node->scale = {em, em, em};
    }
    if (options & COMMAND_LINE) {
        consoleText = gpu::createText(*font, "", false);
        consoleText->node->material(0)->color = Color::black;
        consoleText->node->translation = {48.0f, 16.0f, 0.0f};
        consoleText->node->scale = {em, em, em};
    }
    if (options & FPS) {
        fpsText = gpu::createText(*font, "fps: N/A", false);
        fpsText->node->material(0)->color = Color::black;
        fpsText->node->translation = {24.0f, height - 16.0f - font->ph * em, 0.0f};
        fpsText->node->scale = {em, em, em};
    }
    if (options & NODE_INFO) {
        for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
            nodeInfoRows[i] = gpu::createText(*font, "", false);
            nodeInfoRows[i]->node->material(0)->color = Color::black;
            float s = em * 0.75f;
            nodeInfoRows[i]->node->translation = {width - 256.0f - 24.0f,
                                                  height - 12.0f - (i + 1) * (font->ph * s), 0.0f};
            nodeInfoRows[i]->node->scale = {s, s, s};
        }

        nodeInfoFrame =
            _createFrame(width, height, {256, 128},
                         gpu::createTextureFromMem((uint8_t *)__png__ui, __png__ui_len, true));
        nodeInfoFrame->translation = {width - nodeInfoFrame->scale.x - 36.0f,
                                      height - nodeInfoFrame->scale.y - 6.0f, 0.0f};

        consoleFrame = _createFrame(
            width, height, {512, 48},
            gpu::createTextureFromMem((uint8_t *)__png__console, __png__console_len, true));
        consoleFrame->translation = {6.0f, 2.0f, 0.0f};
        consoleFrame->hidden = true;

        fpsFrame = _createFrame(
            width, height, {192, 48},
            gpu::createTextureFromMem((uint8_t *)__png__tframe, __png__tframe_len, true));
        fpsFrame->translation = {6.0f, height - fpsFrame->scale.y - 10.0f, 0.0f};
    }
}

void GUI::render(gpu::ShaderProgram *frameProgram, gpu::ShaderProgram *textProgram) {
    const float t = Time::seconds();
    textProgram->use();
    textProgram->uniforms.at("u_time") << t;
    if (titleText) {
        titleText->node->render(textProgram);
    }
    if (consoleText) {
        consoleText->node->render(textProgram);
    }
    if (fpsText) {
        fpsText->node->render(textProgram);
    }
    for (gpu::Text *text : nodeInfoRows) {
        if (text) {
            text->node->render(textProgram);
        }
    }
    frameProgram->use();
    if (consoleFrame) {
        consoleFrame->render(frameProgram);
    }
    if (nodeInfoFrame) {
        nodeInfoFrame->render(frameProgram);
    }
    if (fpsFrame) {
        fpsFrame->render(frameProgram);
    }
}

gpu::Text *GUI::setTitleText(const char *value) {
    if (titleText) {
        titleText->setText(value, true);
    }
    return titleText;
}

gpu::Text *GUI::setConsoleText(const char *value) {
    if (value) {
        if (consoleText) {
            consoleText->node->hidden = false;
            consoleFrame->hidden = false;
            consoleText->setText(value, false);
        }
    } else {
        consoleText->node->hidden = true;
        consoleFrame->hidden = true;
    }
    return consoleText;
}

gpu::Text *GUI::setFps(float fps) {
    static char buf[20] = {};
    snprintf(buf, sizeof(buf), "fps: %.1f", fps);
    if (fpsText) {
        fpsText->setText(buf, false);
    }
    return fpsText;
}

void GUI::setNodeInfo(const char *scene, const char *name, const char *mesh,
                      const char *componentInfo, const glm::vec3 &translation,
                      const glm::quat &rotation, const glm::vec3 &scale) {
    if (nodeInfoRows[0] == nullptr) {
        return;
    }
    static char buf[64] = {};
    // snprintf(buf, sizeof(buf), "%s", name);
    // nodeInfoRows[NODE_INFO_NAME]->setText(buf, false);
    snprintf(buf, sizeof(buf), "%s::%s::%s", scene, name, mesh);
    nodeInfoRows[NODE_INFO_TITLE]->setText(buf, false);
    snprintf(buf, sizeof(buf), "T=[%6.2f %6.2f %6.2f]", translation.x, translation.y,
             translation.z);

    nodeInfoRows[NODE_INFO_COMPONENTS]->setText(componentInfo, false);
    nodeInfoRows[NODE_INFO_TRANSLATION]->setText(buf, false);
    glm::vec3 euler = glm::eulerAngles(rotation) / glm::pi<float>() * 180.0f;
    snprintf(buf, sizeof(buf), "R=[%6.2f %6.2f %6.2f]", euler.x, euler.y, euler.z);
    nodeInfoRows[NODE_INFO_ROTATION]->setText(buf, false);
    snprintf(buf, sizeof(buf), "S=[%6.2f %6.2f %6.2f]", scale.x, scale.y, scale.z);
    nodeInfoRows[NODE_INFO_SCALE]->setText(buf, false);
}

void GUI::showNodeInfo(bool visible) {
    for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
        nodeInfoRows[i]->node->hidden = !visible;
        nodeInfoFrame->hidden = !visible;
    }
}