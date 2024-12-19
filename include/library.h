#pragma once

#include "gltf.hpp"

namespace library {

struct Collection {
    gltf::Scene *scene;
    gltf::Animation *animations;
    uint32_t animations_count;
    gltf::Node *nodes;
    uint32_t nodes_count;
    gltf::Mesh *meshes;
    uint32_t meshes_count;
    gltf::Material *materials;
    uint32_t materials_count;
    gltf::Texture *textures;
    uint32_t textures_count;
};

Collection *loadGLB(const unsigned char *glb);

void print();

} // namespace library