#include "persist.h"

#include "component.h"
#include "filesystem.h"
#include "logging.h"
#include <algorithm>
#include <cassert>
#include <list>

static persist::IPersist *_iPersist{nullptr};

void persist::registerIPersist(IPersist *iPersist) { _iPersist = iPersist; }

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
    const std::string_view _fileData = filesystem::loadFile(path);
    if (_fileData.length() == 0) {
        return nullptr;
    }
    return (World *)(_fileData.data());
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
    _file = nullptr;
}

void persist::saveWorld(const char *fpath, const SaveFile &saveFile) {
    std::vector<gpu::Scene *> scenes;
    std::vector<persist::Entity> entities(saveFile.nodes.size());
    size_t i{0};
    for (const auto &instance : saveFile.nodes) {
        gpu::Scene *scene = nullptr;
        assert(instance->libraryNode);
        scene = (gpu::Scene *)instance->libraryNode->scene->gpuInstance;
        auto it = std::find(scenes.begin(), scenes.end(), scene);
        if (it == scenes.end()) {
            scenes.emplace_back(scene);
            it = scenes.end() - 1;
        }
        entities[i].sceneId = std::distance(scenes.begin(), it);
        auto &nodes = (*it)->nodes;
        int nid = -1;
        for (size_t j{0}; j < nodes.size(); ++j) {
            gpu::Node *node = nodes.at(j);
            if (node->libraryNode == instance->libraryNode) {
                nid = j;
                break;
            }
        }
        // assert(instance->libraryNode);
        assert(nid != -1);
        entities[i].nodeId = nid;
        entities[i].translation = instance->translation.data();
        entities[i].rotation = instance->euler.data();
        entities[i].scale = instance->scale.data();
        entities[i].info = 0x0;
        entities[i].extra = 0x0;
        _iPersist->saveNodeInfo(instance, entities[i].info);
        _iPersist->saveNodeExtra(instance, entities[i].extra);
        ++i;
    }
    persist::open(fpath);
    persist::World pw;
    strcpy(pw.name, fpath);
    pw.scenes = scenes.size();
    pw.entities = entities.size();
    pw.info = 0x0;
    pw.extra = 0x0;
    persist::write(pw);
    persist::Scene ps = {};
    persist::Node pn = {};
    for (gpu::Scene *scene : scenes) {
        if (scene->libraryScene) {
            strcpy(ps.name, scene->libraryScene->name.c_str());
        }
        ps.nodes = scene->nodes.size();
        ps.info = 0x0;
        ps.extra = 0x0;
        persist::write(ps);
        for (gpu::Node *node : scene->nodes) {
            strcpy(pn.name, node->libraryNode->name.c_str());
            pn.info = 0x0;
            pn.extra = 0x0;
            persist::write(pn);
        }
    }
    persist::write(entities.data(), entities.size());
    persist::close();
}

bool persist::loadWorld(const char *fpath, SaveFile &saveFile) {
    if (persist::World *world = persist::load(fpath)) {
        persist::Entity *entities = world->firstEntity();
        for (size_t i{0}; i < world->entities; ++i) {
            auto &e = entities[i];
            persist::Scene *scene = world->scene(e.sceneId);
            persist::Node *node = scene->node(e.nodeId);
            if (gpu::Scene *gpuScene = _iPersist->loadScene(scene->name)) {
                if (auto gpuNode = gpuScene->nodeByName(node->name)) {
                    gpu::Node *inst =
                        saveFile.nodes.emplace_back(gpu::createNode(*gpuNode->libraryNode));
                    inst->translation = e.translation;
                    inst->euler = e.rotation;
                    inst->scale = e.scale;
                    if (e.info != 0 || e.extra != 0 || inst->skin) {
                        ecs::Entity *entity = ecs::create_entity();
                        inst->entity = entity;
                        _iPersist->loadEntity(entity, inst, e.info, e.extra);
                    }
                } else {
                    LOG_WARN("Failed to load node: %s\n", node->name);
                }
            }
        }
    }
    return false;
}