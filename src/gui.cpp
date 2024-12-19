#include "gui.h"
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
        titleText = gpu::createText(*font, "untitled*", true);
        titleText->node->material(0)->color = Color::black;
        titleText->node->translation = {width * 0.5f, height - 16.0f - font->ph * em, 0.0f};
        titleText->node->scale = {em, em, em};
    }
    if (options & COMMAND_LINE) {
        consoleText = gpu::createText(*font, "", false);
        consoleText->node->material(0)->color = Color::black;
        consoleText->node->translation = {16.0f, 16.0f, 0.0f};
        consoleText->node->scale = {em, em, em};
    }
    if (options & FPS) {
        fpsText = gpu::createText(*font, "fps: N/A", false);
        fpsText->node->material(0)->color = Color::black;
        fpsText->node->translation = {16.0f, height - 16.0f - font->ph * em, 0.0f};
        fpsText->node->scale = {em, em, em};
    }
    if (options & NODE_INFO) {
        for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
            nodeInfoRows[i] = gpu::createText(*font, "", false);
            nodeInfoRows[i]->node->material(0)->color = Color::black;
            float s = em * 0.8f;
            nodeInfoRows[i]->node->translation = {width - 256.0f - 16.0f,
                                                  height - 16.0f - (i + 1) * (font->ph * s), 0.0f};
            nodeInfoRows[i]->node->scale = {s, s, s};
        }
    }
}

void GUI::render(gpu::ShaderProgram *shaderProgram) {
    const float t = Time::seconds();
    shaderProgram->use();
    shaderProgram->uniforms.at("u_time") << t;
    if (titleText) {
        titleText->node->render(shaderProgram);
    }
    if (consoleText) {
        consoleText->node->render(shaderProgram);
    }
    if (fpsText) {
        fpsText->node->render(shaderProgram);
    }
    for (gpu::Text *text : nodeInfoRows) {
        if (text) {
            text->node->render(shaderProgram);
        }
    }
}

gpu::Text *GUI::setTitleText(const char *value) {
    if (titleText) {
        titleText->setText(value, true);
    }
    return titleText;
}

gpu::Text *GUI::setConsoleText(const char *value) {
    if (consoleText) {
        consoleText->setText(value, false);
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

void GUI::setNodeInfo(const char *name, const char *mesh, const glm::vec3 &translation,
                      const glm::quat &rotation, const glm::vec3 &scale) {
    if (nodeInfoRows[NODE_INFO_NAME] == nullptr) {
        return;
    }
    static char buf[64] = {};
    // snprintf(buf, sizeof(buf), "Node: %s", name);
    // nodeInfoRows[NODE_INFO_NAME]->setText(buf, false);
    snprintf(buf, sizeof(buf), "%s::%s", name, mesh);
    nodeInfoRows[NODE_INFO_MESH]->setText(buf, false);
    snprintf(buf, sizeof(buf), "T: [%6.2f %6.2f %6.2f]", translation.x, translation.y,
             translation.z);
    nodeInfoRows[NODE_INFO_TRANSLATION]->setText(buf, false);
    glm::vec3 euler = glm::eulerAngles(rotation) / glm::pi<float>() * 180.0f;
    snprintf(buf, sizeof(buf), "R: [%6.2f %6.2f %6.2f]", euler.x, euler.y, euler.z);
    nodeInfoRows[NODE_INFO_ROTATION]->setText(buf, false);
    snprintf(buf, sizeof(buf), "S: [%6.2f %6.2f %6.2f]", scale.x, scale.y, scale.z);
    nodeInfoRows[NODE_INFO_SCALE]->setText(buf, false);
}

void GUI::showNodeInfo(bool visible) {
    for (size_t i{0}; i < NODE_INFO_COUNT; ++i) {
        nodeInfoRows[i]->node->visible = visible;
    }
}