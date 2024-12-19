#include "assets.h"

#include "logging.h"

assets::Collection::Collection() {}

assets::Collection::Collection(const library::Collection &collection)
    : scene{gpu::createScene(*collection.scene)} {
    LOG_INFO("Collection: #animations=%u #materials=%u"
             " #meshes=%u #nodes=%u #textures=%u",
             collection.animations_count, collection.materials_count, collection.meshes_count,
             collection.nodes_count, collection.textures_count);
    for (size_t i{0}; i < collection.animations_count; ++i) {
        animations.emplace_back(animation::createAnimation(collection.animations[i]));
    }
    if (!animations.empty()) {
        animations.front()->start();
    }
}

animation::Animation *assets::Collection::animationByName(const char *name) {
    for (auto &animation : animations) {
        if (animation->name.compare(name) == 0) {
            return animation;
        }
    }
    return nullptr;
}

const std::string &assets::Collection::name() const {
    if (scene && scene->gltfScene) {
        return scene->gltfScene->name;
    }
    static const std::string noname{};
    return noname;
}