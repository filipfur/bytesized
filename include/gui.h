#pragma once

#include "gpu.h"
#include <glm/glm.hpp>

struct GUI {
    enum Options {
        NONE = 0,
        COMMAND_LINE = (1 << 0),
        FPS = (1 << 1),
        NODE_INFO = (1 << 2),
        TITLE = (1 << 3),
        EVERYTHING = 0xFFFFFFFF,
    };
    void create(bdf::Font *font, float width, float height, float em, Options options);

    void render(gpu::ShaderProgram *shaderProgram);

    gpu::Text *setTitleText(const char *value);
    gpu::Text *setConsoleText(const char *value);
    gpu::Text *setFps(float fps);
    void setNodeInfo(const char *name, const char *mesh, const glm::vec3 &translation,
                     const glm::quat &rotation, const glm::vec3 &scale);
    void showNodeInfo(bool visible);

    bdf::Font *font{nullptr};
    float width;
    float height;
    float em;
    glm::mat4 projection;
    glm::mat4 view;

    gpu::Text *titleText{nullptr};
    gpu::Text *consoleText{nullptr};
    gpu::Text *fpsText{nullptr};
    enum NodeInfoDetail {
        NODE_INFO_NAME,
        NODE_INFO_MESH,
        NODE_INFO_TRANSLATION,
        NODE_INFO_ROTATION,
        NODE_INFO_SCALE,
        NODE_INFO_COUNT,
    };
    gpu::Text *nodeInfoRows[NODE_INFO_COUNT];
};