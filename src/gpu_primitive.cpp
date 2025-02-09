#include "gpu_primitive.h"

constexpr gpu::Sprite sprite;
constexpr gpu::Plane<1, 1> plane;
constexpr gpu::Cube cube;
constexpr gpu::Sphere<16, 8> sphere;
constexpr gpu::Sphere<16, 8> cylinder(gpu::Sphere<16, 8>::OPT_CYLINDER);
constexpr gpu::Sphere<16, 8> cone(gpu::Sphere<16, 8>::OPT_CONE_A);

template <typename T>
inline static library::Primitive &_populatePrimitive(const T &geometry,
                                                     library::Primitive &primitive) {
    primitive.attributes[library::Primitive::POSITION] = library::createAccessor(
        geometry.positions, sizeof(geometry.positions) / sizeof(glm::vec3), sizeof(glm::vec3),
        library::Accessor::VEC3, GL_FLOAT, GL_ARRAY_BUFFER);
    primitive.attributes[library::Primitive::NORMAL] = library::createAccessor(
        geometry.normals, sizeof(geometry.normals) / sizeof(glm::vec3), sizeof(glm::vec3),
        library::Accessor::VEC3, GL_FLOAT, GL_ARRAY_BUFFER);
    primitive.attributes[library::Primitive::TEXCOORD_0] = library::createAccessor(
        geometry.uvs, sizeof(geometry.uvs) / sizeof(glm::vec2), sizeof(glm::vec2),
        library::Accessor::VEC2, GL_FLOAT, GL_ARRAY_BUFFER);
    primitive.indices = library::createAccessor(
        geometry.indices, sizeof(geometry.indices) / sizeof(uint16_t), sizeof(uint16_t),
        library::Accessor::SCALAR, GL_UNSIGNED_SHORT, GL_ELEMENT_ARRAY_BUFFER);
    return primitive;
}

static library::Primitive *_builtinPrimitives[gpu::BuiltinPrimitives::PRIMITIVE_COUNT];

library::Collection *gpu::createBuiltinPrimitives() {
    library::Collection *collection = library::createCollection("builtin");
    for (size_t i{0}; i < gpu::BuiltinPrimitives::PRIMITIVE_COUNT; ++i) {
        library::Node *node = library::createNode();
        library::Mesh *mesh = library::createMesh();
        if (collection->nodes == nullptr) {
            collection->nodes = node;
        }
        if (collection->meshes == nullptr) {
            collection->meshes = mesh;
        }
        collection->scene->nodes.emplace_back(node);
        node->mesh = mesh;
        node->translation = {2.5f * i, 0.0f, 0.0f};
        node->rotation = {1.0f, 0.0f, 0.0f, 0.0f};
        node->scale = {1.0f, 1.0f, 1.0f};
        node->scene = collection->scene;
        switch (i) {
        case gpu::BuiltinPrimitives::SPRITE:
            node->name = "Sprite";
            _builtinPrimitives[i] = &_populatePrimitive(sprite, mesh->primitives.emplace_back());
            break;
        case gpu::BuiltinPrimitives::PLANE:
            node->name = "Plane";
            _builtinPrimitives[i] = &_populatePrimitive(plane, mesh->primitives.emplace_back());
            break;
        case gpu::BuiltinPrimitives::CUBE:
            node->name = "Cube";
            _builtinPrimitives[i] = &_populatePrimitive(cube, mesh->primitives.emplace_back());
            break;
        case gpu::BuiltinPrimitives::SPHERE:
            node->name = "Sphere";
            _builtinPrimitives[i] = &_populatePrimitive(sphere, mesh->primitives.emplace_back());
            break;
        case gpu::BuiltinPrimitives::CYLINDER:
            node->name = "Cylinder";
            _builtinPrimitives[i] = &_populatePrimitive(cylinder, mesh->primitives.emplace_back());
            break;
        case gpu::BuiltinPrimitives::CONE:
            node->name = "Cone";
            _builtinPrimitives[i] = &_populatePrimitive(cone, mesh->primitives.emplace_back());
            break;
        }
        mesh->name = node->name;
        ++collection->nodes_count;
        ++collection->meshes_count;
    }

    return collection;
}

gpu::Primitive *gpu::builtinPrimitives(BuiltinPrimitives builtinPrimitives) {
    return (gpu::Primitive *)_builtinPrimitives[builtinPrimitives]->gpuInstance;
}
