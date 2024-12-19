#include "persist.h"

#include "filesystem.h"
#include "gpu.h"
#include "logging.h"
#include <cassert>
#include <list>

void persist::storeSession(const SessionData &sessionData) {
    FILE *file = fopen("session.bin", "w");
    fwrite(&sessionData, sizeof(SessionData), 1, file);
    fflush(file);
    fclose(file);
}

bool persist::loadSession(SessionData &sessionData) {
    FILE *file = fopen("session.bin", "r");
    if (file) {
        fread(&sessionData, sizeof(SessionData), 1, file);
        fclose(file);
        return true;
    }
    return false;
}

persist::World *persist::load(const char *path) {
    std::string_view data = filesystem::loadFile(path);
    if (data.length() == 0) {
        return nullptr;
    }
    return (World *)(data.data());
}

static char _buffer[1024 * 1024];
static FILE *_file = nullptr;
void persist::open(const char *path) {
    assert(_file == nullptr);
    _file = fopen(path, "w");
    if (_file == NULL) {
        LOG_ERROR("file can't be opened (w): %s", path);
    }
    if (setvbuf(_file, _buffer, _IOFBF, sizeof(_buffer)) != 0) {
        LOG_ERROR("Failed to set buffer");
    }
}
void persist::write(const World &world) { fwrite(&world, sizeof(World), 1, _file); }
void persist::write(const Scene &scene) { fwrite(&scene, sizeof(Scene), 1, _file); }
void persist::write(const Node &node) { fwrite(&node, sizeof(Node), 1, _file); }
void persist::write(const Entity &entity) { fwrite(&entity, sizeof(Entity), 1, _file); }
void persist::write(const Entity *entities, size_t count) {
    fwrite(entities, sizeof(Entity), count, _file);
}
void persist::close() {
    assert(_file);
    // fflush(_file);
    fclose(_file);
}

void persist::saveWorld(const char *name, const SaveFile &saveFile) {
    std::vector<gpu::Scene *> scenes;
    std::vector<persist::Entity> entities(saveFile.nodes.size());
    size_t i{0};
    for (const auto &instance : saveFile.nodes) {
        gpu::Scene *scene = (gpu::Scene *)instance->gltfNode->scene->gpuInstance;
        assert(scene);
        auto it = std::find(scenes.begin(), scenes.end(), scene);
        if (it == scenes.end()) {
            scenes.emplace_back(scene);
            it = scenes.end() - 1;
        }
        entities[i].sceneId = std::distance(scenes.begin(), it);
        auto &nodes = (*it)->nodes;
        int nid = -1;
        for (size_t i{0}; i < nodes.size(); ++i) {
            gpu::Node *node = nodes.at(i);
            if (node->gltfNode == instance->gltfNode) {
                nid = i;
                break;
            }
        }
        assert(instance->gltfNode);
        assert(nid != -1);
        entities[i].nodeId = nid;
        entities[i].translation = instance->translation.data();
        entities[i].rotation = instance->rotation.data();
        entities[i].scale = instance->scale.data();
        entities[i].info = 0x0;
        entities[i].extra = 0x0;
        ++i;
    }
    persist::open(name);
    persist::World pw;
    strcpy(pw.name, name);
    pw.scenes = scenes.size();
    pw.entities = entities.size();
    pw.info = 0x0;
    pw.extra = 0x0;
    persist::write(pw);
    persist::Scene ps = {};
    persist::Node pn = {};
    for (gpu::Scene *scene : scenes) {
        if (scene->gltfScene) {
            strcpy(ps.name, scene->gltfScene->name.c_str());
        }
        ps.nodes = scene->nodes.size();
        ps.info = 0x0;
        ps.extra = 0x0;
        persist::write(ps);
        for (gpu::Node *node : scene->nodes) {
            strcpy(pn.name, node->gltfNode->name.c_str());
            pn.info = 0x0;
            pn.extra = 0x0;
            persist::write(pn);
        }
    }
    persist::write(entities.data(), entities.size());
    persist::close();
}

bool persist::loadWorld(const char *name, SaveFile &saveFile,
                        const std::function<gpu::Scene *(const char *name)> &callback) {
    if (persist::World *world = persist::load(name)) {
        persist::Entity *entities = world->firstEntity();
        for (size_t i{0}; i < world->entities; ++i) {
            auto &e = entities[i];
            persist::Scene *scene = world->scene(e.sceneId);
            persist::Node *node = scene->node(e.nodeId);
            gpu::Scene *gpuScene = callback(scene->name);
            auto *inst = saveFile.nodes.emplace_back(
                gpu::createNode(*gpuScene->nodeByName(node->name)->gltfNode));
            inst->translation = e.translation;
            inst->rotation = e.rotation;
            inst->scale = e.scale;
        }
    }
    return false;
}