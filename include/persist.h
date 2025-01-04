#pragma once

#include "cameraview.h"
#include "ecs.h"
#include "gpu.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace persist {

struct IPersist {
    virtual bool saveNodeInfo(gpu::Node *node, uint32_t &info) = 0;
    virtual bool saveNodeExtra(gpu::Node *node, uint32_t &extra) = 0;
    virtual gpu::Scene *loadedScene(const char *name) = 0;
    virtual bool loadedEntity(ecs::Entity *entity, gpu::Node *node, uint32_t info,
                              uint32_t extra) = 0;
};

void registerIPersist(IPersist *iPersist);

struct Entity {
    uint16_t sceneId;
    uint16_t nodeId;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    uint32_t info;
    uint32_t extra;
};
static_assert(sizeof(Entity) == sizeof(Entity::sceneId) + sizeof(Entity::nodeId) +
                                    sizeof(Entity::info) + sizeof(Entity::extra) +
                                    sizeof(glm::vec3) * 2 + sizeof(glm::quat));
struct Node {
    char name[24];
    uint32_t info;
    uint32_t extra;
};
static_assert(sizeof(Node) == sizeof(Node::name) + sizeof(Node::info) + sizeof(Node::extra));
struct Scene {
    char name[24];
    uint32_t nodes;
    uint32_t info;
    uint32_t extra;
    Node *node(uint16_t index) {
        assert(index <= nodes); // allow past the end
        return (Node *)(this + 1) + index;
    }
};
static_assert(sizeof(Scene) == sizeof(Scene::name) + sizeof(Scene::info) + sizeof(Scene::nodes) +
                                   sizeof(Scene::extra));
struct World {
    char name[24];
    uint16_t scenes;
    uint16_t entities;
    uint32_t info;
    uint32_t extra;
    Scene *scene(uint16_t index) {
        assert(index <= scenes); // allow past the end
        Scene *s = (Scene *)(this + 1);
        for (size_t i{0}; i < index; ++i) {
            s = (Scene *)s->node(s->nodes);
        }
        return s;
    }
    Entity *firstEntity() { return (Entity *)scene(scenes); }
};
static_assert(sizeof(World) == sizeof(World::name) + sizeof(World::scenes) +
                                   sizeof(World::entities) + sizeof(World::info) +
                                   sizeof(World::extra));

World *load(const char *path);

void open(const char *path);
void write(const World &world);
void write(const Scene &scene);
void write(const Node &node);
void write(const Entity &entity);
void write(const Entity *entities, size_t count);
void close();

struct SessionData {
    char recentFile[64];
    CameraView cameraView;
};
static_assert(sizeof(SessionData) == 64 + sizeof(glm::vec3) * 1 + sizeof(float) * 3);

void storeSession(const SessionData &sessionData);
bool loadSession(SessionData &sessionData);

struct SaveFile {
    char path[64];
    std::vector<gpu::Node *> nodes;
    bool dirty;
};
void saveWorld(const char *fpath, const SaveFile &saveFile);
bool loadWorld(const char *fpath, SaveFile &saveFile);

} // namespace persist