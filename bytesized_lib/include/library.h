#pragma once

#include "library_types.h"

namespace library {

void printAllocations();
Collection *loadGLB(const unsigned char *glb, bool copyBuffers = false);
Collection *createCollection(const char *name);
Collection *builtinCollection();
bool isBuiltin(const library::Mesh *mesh);
Node *createNode();
Mesh *createMesh();
Accessor *createAccessor(const void *data, size_t count, size_t elemSize,
                         library::Accessor::Type type, unsigned int componentType,
                         unsigned int target);

} // namespace library