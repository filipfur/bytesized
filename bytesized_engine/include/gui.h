#pragma once

#include "frame.h"
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

    void render(gpu::ShaderProgram *frameProgram, gpu::ShaderProgram *textProgram);

    gpu::Text *setTitleText(const char *value);
    gpu::Text *setConsoleText(const char *value);
    gpu::Text *setFps(float fps);
    void setNodeInfo(const char *scene, const char *name, uint32_t id, const char *mesh,
                     const char *componentInfo, const glm::vec3 &translation,
                     const glm::vec3 &euler, const glm::vec3 &scale);
    void showNodeInfo(bool visible);

    bdf::Font *font{nullptr};
    float width;
    float height;
    float em;
    glm::mat4 projection;
    glm::mat4 view;

    enum NodeInfoDetail {
        NODE_INFO_TITLE,
        NODE_INFO_COMPONENTS,
        NODE_INFO_MESH,
        NODE_INFO_TRANSLATION,
        NODE_INFO_ROTATION,
        NODE_INFO_SCALE,
        NODE_INFO_COUNT,
    };
    gpu::Text *nodeInfoRows[NODE_INFO_COUNT];

    enum Frames { FRAME_FPS, FRAME_CONSOLE, FRAME_NODE_INFO, FRAME_TITLE, FRAME_COUNT };
    Frame frames[FRAME_COUNT];
};
