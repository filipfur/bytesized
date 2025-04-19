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

static const char *next_val(const std::string_view &fileData, size_t &i) {
    bool foundSpace = false;
    while (i < fileData.length() && (foundSpace && fileData[i] != ' ') == false &&
           fileData[i] != '\n') {
        if (fileData[i] == ' ') {
            foundSpace = true;
        }
        ++i;
    }
    return (i < fileData.length() && fileData[i] != '\n') ? (fileData.data() + i) : nullptr;
}

static void strcpy_todelim(char *dst, const char *src, char delim) {
    for (size_t i{0}; i < 1024; ++i) {
        if (src[i] == delim || src[i] == '\0') {
            dst[i] = '\0';
            break;
        }
        dst[i] = src[i];
    }
}

persist::World *persist::load(const char *path) {
    const std::string_view _fileData = filesystem::loadFile(path);
    if (_fileData.length() == 0) {
        return nullptr;
    }
    if (strcmp(path + strlen(path) - 4, ".txt") == 0) {
        static unsigned char _mem[1024 * 1024];
        unsigned char *addr = _mem;
        persist::World *world;
        persist::Scene *scene;
        persist::Node *node;
        persist::Entity *entity;
        for (size_t i{0}; i < _fileData.length(); ++i) {
            switch (_fileData[i]) {
            case 'w':
                world = (World *)addr;
                world->scenes = 0;
                world->entities = 0;
                strcpy_todelim(world->name, next_val(_fileData, i), ' ');
                world->info = std::atoi(next_val(_fileData, i));
                world->extra = std::atoi(next_val(_fileData, i));
                assert(next_val(_fileData, i) == nullptr);
                addr += sizeof(World);
                break;
            case 's':
                scene = (Scene *)addr;
                scene->nodes = 0;
                strcpy_todelim(scene->name, next_val(_fileData, i), ' ');
                scene->info = std::atoi(next_val(_fileData, i));
                scene->extra = std::atoi(next_val(_fileData, i));
                assert(next_val(_fileData, i) == nullptr);
                addr += sizeof(Scene);
                ++world->scenes;
                break;
            case 'n':
                node = (Node *)addr;
                strcpy_todelim(node->name, next_val(_fileData, i), ' ');
                node->info = std::atoi(next_val(_fileData, i));
                node->extra = std::atoi(next_val(_fileData, i));
                assert(next_val(_fileData, i) == nullptr);
                addr += sizeof(Node);
                ++scene->nodes;
                break;
            case 'e':
                entity = (Entity *)addr;
                entity->info = std::atoi(next_val(_fileData, i));
                entity->extra = std::atoi(next_val(_fileData, i));
                assert(next_val(_fileData, i)[0] == '/');
                entity->sceneId = std::atoi(next_val(_fileData, i));
                entity->nodeId = std::atoi(next_val(_fileData, i));
                assert(next_val(_fileData, i)[0] == '/');
                entity->translation.x = std::atof(next_val(_fileData, i));
                entity->translation.y = std::atof(next_val(_fileData, i));
                entity->translation.z = std::atof(next_val(_fileData, i));
                assert(next_val(_fileData, i)[0] == '/');
                entity->rotation.x = std::atof(next_val(_fileData, i));
                entity->rotation.y = std::atof(next_val(_fileData, i));
                entity->rotation.z = std::atof(next_val(_fileData, i));
                assert(next_val(_fileData, i)[0] == '/');
                entity->scale.x = std::atof(next_val(_fileData, i));
                entity->scale.y = std::atof(next_val(_fileData, i));
                entity->scale.z = std::atof(next_val(_fileData, i));
                assert(next_val(_fileData, i) == nullptr);
                addr += sizeof(Entity);
                ++world->entities;
                break;
            default:
                assert(false);
                break;
            }
        }
        return world;
    }
    return (World *)(_fileData.data());
}

static char _buffer[1024 * 1024];
static FILE *_file = nullptr;
enum FileMode {
    MODE_BIN,
    MODE_PLAIN,
};
FileMode _fileMode = MODE_PLAIN;
void persist::open(const char *path) {
    assert(_file == nullptr);
    _file = fopen(path, "w");
    if (_file == NULL) {
        LOG_ERROR("file can't be opened (w): %s", path);
    }
    if (setvbuf(_file, _buffer, _IOFBF, sizeof(_buffer)) != 0) {
        LOG_ERROR("Failed to set buffer");
    }
    if (strcmp(path + strlen(path) - 4, ".txt") == 0) {
        _fileMode = MODE_PLAIN;
    } else {
        _fileMode = MODE_BIN;
    }
}
void persist::write(const World &world) {
    switch (_fileMode) {
    case MODE_BIN:
        fwrite(&world, sizeof(World), 1, _file);
        break;
    case MODE_PLAIN:
        fprintf(_file, "w %s %d %d\n", world.name, world.info, world.extra);
        break;
    }
}
void persist::write(const Scene &scene) {
    switch (_fileMode) {
    case MODE_BIN:
        fwrite(&scene, sizeof(Scene), 1, _file);
        break;
    case MODE_PLAIN:
        fprintf(_file, "s %s %d %d\n", scene.name, scene.info, scene.extra);
        break;
    }
}
void persist::write(const Node &node) {
    switch (_fileMode) {
    case MODE_BIN:
        fwrite(&node, sizeof(Node), 1, _file);
        break;
    case MODE_PLAIN:
        fprintf(_file, "n %s %d %d\n", node.name, node.info, node.extra);
        break;
    }
}
void persist::write(const Entity &entity) {
    switch (_fileMode) {
    case MODE_BIN:
        fwrite(&entity, sizeof(Entity), 1, _file);
        break;
    case MODE_PLAIN:
        fprintf(_file, "e %d %d / %d %d / %f %f %f / %f %f %f / %f %f %f\n", entity.info,
                entity.extra, entity.sceneId, entity.nodeId, entity.translation.x,
                entity.translation.y, entity.translation.z, entity.rotation.x, entity.rotation.y,
                entity.rotation.z, entity.scale.x, entity.scale.y, entity.scale.z);
        break;
    }
}
void persist::write(const Entity *entities, size_t count) {
    switch (_fileMode) {
    case MODE_BIN:
        fwrite(entities, sizeof(Entity), count, _file);
        break;
    case MODE_PLAIN:
        for (size_t i{0}; i < count; ++i) {
            write(entities[i]);
        }
        break;
    }
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
            if (gpu::Scene *gpuScene = _iPersist->getSceneByName(scene->name)) {
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
