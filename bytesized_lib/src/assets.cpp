#include "assets.h"

#include "logging.h"

assets::Collection::Collection() {}

assets::Collection::Collection(const library::Collection &collection)
    : scene{gpu::createScene(*collection.scene)} {
    LOG_INFO("Collection %s: #animations=%u #materials=%u"
             " #meshes=%u #nodes=%u #textures=%u",
             collection.scene->name.c_str(), collection.animations_count,
             collection.materials_count, collection.meshes_count, collection.nodes_count,
             collection.textures_count);
    for (size_t i{0}; i < collection.animations_count; ++i) {
        animations.emplace_back(gpu::createAnimation(collection.animations[i], nullptr));
    }
    if (!animations.empty()) {
        if (auto idleAnim = animationByName("Idle")) {
            idleAnim->start();
        } else {
            animations.front()->start();
        }
    }

    assert(scene->libraryScene != nullptr);
    assert(scene->libraryScene->gpuInstance == scene);
    for (gpu::Node *node : scene->nodes) {
        assert(node->libraryNode);
        assert(node->libraryNode->gpuInstance != nullptr);
        assert(node->libraryNode->scene == scene->libraryScene);
        assert(node->libraryNode->scene != nullptr);
    }
}

gpu::Animation *assets::Collection::animationByName(const char *name) {
    for (auto &animation : animations) {
        if (animation->name.compare(name) == 0) {
            return animation;
        }
    }
    return nullptr;
}

const std::string &assets::Collection::name() const {
    if (scene && scene->libraryScene) {
        return scene->libraryScene->name;
    }
    static const std::string noname{};
    return noname;
}