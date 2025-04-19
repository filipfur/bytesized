#include "gui.h"
#include "embed/console_png.hpp"
#include "embed/tframe_png.hpp"
#include "embed/ui_png.hpp"
#include "gpu_primitive.h"
#include "systime.h"
#include <glm/gtc/quaternion.hpp>

void GUI::create(bdf::Font *font_, float width_, float height_, float em_, Options options) {
    font = font_;
    width = width_;
    height = height_;
    em = em_;
    projection = glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    view = glm::mat4{1.0f};
    if (options & TITLE) {
        auto &titleFrame = frames[FRAME_TITLE];
        titleFrame.createText(*font, width - 192.0f, font->ph * 0.5f * em, em, "untitled*", false,
                              Color::black);
    }
    if (options & COMMAND_LINE) {
        auto &consoleFrame = frames[FRAME_CONSOLE];
        consoleFrame.createPanel(512, 48,
                                 gpu::createTextureFromMem((uint8_t *)_embed_console_png,
                                                           sizeof(_embed_console_png), true));
        consoleFrame.setPosition(6.0f, 2.0f);
        consoleFrame.setHidden(true);
        auto &textFrame = consoleFrame.children.emplace_back();
        textFrame.createText(*font, 48, 16, em, "", false, Color::black);
    }
    if (options & FPS) {
        auto &fpsFrame = frames[FRAME_FPS];
        fpsFrame.createPanel(192, 48,
                             gpu::createTextureFromMem((uint8_t *)_embed_tframe_png,
                                                       sizeof(_embed_tframe_png), true));
        fpsFrame.setPosition(6.0f, height - fpsFrame.height() - 10.0f);
        auto &textFrame = fpsFrame.children.emplace_back();
        textFrame.createText(*font, 24.0f, height - 16.0f - font->ph * em, em, "fps: N/A", false,
                             Color::black);
    }
    if (options & NODE_INFO) {
        for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
            nodeInfoRows[i] = gpu::createText(*font, "", false);
            nodeInfoRows[i]->node->material(0)->color = Color::black;
            float s = em * 0.75f;
            nodeInfoRows[i]->node->translation = {width - 256.0f * 1.33f - 16.0f,
                                                  height - 24.0f - (i + 1) * (font->ph * s), 0.0f};
            nodeInfoRows[i]->node->scale = {s, s, s};
        }
        auto &nodeInfoFrame = frames[FRAME_NODE_INFO];
        nodeInfoFrame.createPanel(
            256 * 1.33f, 128 * 1.33f,
            gpu::createTextureFromMem((uint8_t *)_embed_ui_png, sizeof(_embed_ui_png), true));
        nodeInfoFrame.setPosition(width - nodeInfoFrame.width() - 36.0f,
                                  height - nodeInfoFrame.height() - 6.0f);
        nodeInfoFrame.setHidden(true);
    }
}

void GUI::render(gpu::ShaderProgram *frameProgram, gpu::ShaderProgram *textProgram) {
    const float t = Time::seconds();
    textProgram->use();
    textProgram->uniforms.at("u_time") << t;
    for (auto &frame : frames) {
        frame.renderTexts(textProgram);
    }

    for (gpu::Text *text : nodeInfoRows) {
        if (text) {
            text->node->render(textProgram);
        }
    }
    frameProgram->use();
    for (auto &frame : frames) {
        frame.renderPanels(frameProgram);
    }
}

gpu::Text *GUI::setTitleText(const char *value) {
    if (frames[FRAME_TITLE]) {
        frames[FRAME_TITLE].text->setText(value, true);
    }
    return frames[FRAME_TITLE].text;
}

gpu::Text *GUI::setConsoleText(const char *value) {
    if (value) {
        if (frames[FRAME_CONSOLE]) {
            frames[FRAME_CONSOLE].setHidden(false);
            frames[FRAME_CONSOLE].children[0].text->setText(value, false);
        }
    } else {
        frames[FRAME_CONSOLE].setHidden(true);
    }
    return frames[FRAME_CONSOLE].children[0].text;
}

gpu::Text *GUI::setFps(float fps) {
    static char buf[20] = {};
    snprintf(buf, sizeof(buf), "fps: %.1f", fps);
    if (frames[FRAME_FPS]) {
        frames[FRAME_FPS].children[0].text->setText(buf, false);
    }
    return frames[FRAME_FPS].children[0].text;
}

void GUI::setNodeInfo(const char *scene, const char *name, uint32_t id, const char *mesh,
                      const char *componentInfo, const glm::vec3 &translation,
                      const glm::vec3 &euler, const glm::vec3 &scale) {
    if (nodeInfoRows[0] == nullptr) {
        return;
    }
    static char buf[64] = {};
    snprintf(buf, sizeof(buf), "%s<%d>", name, id);
    nodeInfoRows[NODE_INFO_TITLE]->setText(buf, false);
    nodeInfoRows[NODE_INFO_COMPONENTS]->setText(componentInfo, false);
    snprintf(buf, sizeof(buf), "%s::%s", scene, mesh);
    nodeInfoRows[NODE_INFO_MESH]->setText(buf, false);
    snprintf(buf, sizeof(buf), "T=[%6.2f %6.2f %6.2f]", translation.x, translation.y,
             translation.z);
    nodeInfoRows[NODE_INFO_TRANSLATION]->setText(buf, false);
    snprintf(buf, sizeof(buf), "R=[%6.2f %6.2f %6.2f]", euler.x, euler.y, euler.z);
    nodeInfoRows[NODE_INFO_ROTATION]->setText(buf, false);
    snprintf(buf, sizeof(buf), "S=[%6.2f %6.2f %6.2f]", scale.x, scale.y, scale.z);
    nodeInfoRows[NODE_INFO_SCALE]->setText(buf, false);
}

void GUI::showNodeInfo(bool visible) {
    for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
        nodeInfoRows[i]->node->hidden = !visible;
        frames[FRAME_NODE_INFO].setHidden(!visible);
    }
}