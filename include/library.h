#pragma once

#include "library_types.h"

namespace library {

struct Collection {
    Scene *scene;
    Animation *animations;
    uint32_t animations_count;
    Node *nodes;
    uint32_t nodes_count;
    Mesh *meshes;
    uint32_t meshes_count;
    Material *materials;
    uint32_t materials_count;
    Texture *textures;
    uint32_t textures_count;
};

Collection *loadGLB(const unsigned char *glb, bool copyBuffers = false);
Collection *createCollection(const char *name);
Node *createNode();
Mesh *createMesh();
Accessor *createAccessor(const void *data, size_t count, size_t elemSize,
                         library::Accessor::Type type, unsigned int componentType,
                         unsigned int target);

void print();

} // namespace library