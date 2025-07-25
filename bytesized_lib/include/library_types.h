#pragma once

#include "bytesized_info.h"
#include "trs.h"
#include <string>
#include <vector>

namespace library {

struct Buffer {
    const unsigned char *data;
    size_t length;
};

struct Bufferview {
    Buffer *buffer;
    size_t length;
    size_t offset;
    unsigned int target;
    void *data() { return (void *)(buffer->data + offset); }
};

struct Accessor {
    enum Type { SCALAR, VEC2, VEC3, VEC4, MAT4 };
    Bufferview *bufferView;
    unsigned int componentType;
    unsigned int count;
    Type type;
    void *data() { return bufferView->data(); }
};

struct Image {
    std::string name;
    Bufferview *view;
};

struct TextureSampler {
    uint32_t magFilter;
    uint32_t minFilter;
};

struct Texture {
    Image *image;
    TextureSampler *sampler;
};

struct Material {
    enum Type { TEX_DIFFUSE, TEX_NORMAL, TEX_ARM, TEX_COUNT } type;
    std::string name;
    glm::vec4 baseColor;
    float metallic;
    float roughness;
    Texture *textures[TEX_COUNT];
};

struct Primitive {
    enum Attribute {
        POSITION,
        NORMAL,
        TEXCOORD_0,
        COLOR_0,
        COLOR_1,
#ifdef BYTESIZED_USE_SKINNING
        JOINTS_0,
        WEIGHTS_0,
#endif
        COUNT,
    };
    std::pair<const glm::vec3 *, size_t> positions() const {
        return {(glm::vec3 *)attributes[POSITION]->data(),
                attributes[POSITION]->bufferView->length / sizeof(glm::vec3)};
    }
    std::pair<const glm::vec3 *, size_t> normals() const {
        return {(glm::vec3 *)attributes[NORMAL]->data(),
                attributes[NORMAL]->bufferView->length / sizeof(glm::vec3)};
    }
    std::pair<const glm::vec2 *, size_t> texcoords() const {
        return {(glm::vec2 *)attributes[TEXCOORD_0]->data(),
                attributes[TEXCOORD_0]->bufferView->length / sizeof(glm::vec2)};
    }
    Accessor *attributes[COUNT];
    Accessor *indices;
    Material *material;
    void *gpuInstance;
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
    void *gpuInstance;
};

struct Node : public TRS {
    const struct Scene *scene;
    std::string name;
    Mesh *mesh;
    std::vector<Node *> children;
#ifdef BYTESIZED_USE_SKINNING
    struct Skin *skin;
#endif
    void *gpuInstance;
};

#ifdef BYTESIZED_USE_SKINNING
struct Skin {
    std::string name;
    std::vector<Node *> joints;
    Accessor *ibmData;
    std::vector<glm::mat4> inverseBindMatrices;
};
#endif

struct Scene {
    std::string name;
    std::vector<Node *> nodes;
    void *gpuInstance;
};

#ifdef BYTESIZED_USE_SKINNING
struct Sampler {
    enum Type { NONE, STEP, LINEAR } type;
    Accessor *input;
    Accessor *output;
};

struct Channel {
    enum Type { NONE, TRANSLATION, ROTATION, SCALE } type;
    Node *targetNode;
    Sampler *sampler;
};

struct Animation {
    std::string name;
    Channel channels[96];
    Sampler samplers[96];
};
#endif

struct Collection {
    Scene *scene;
    Node *nodes;
    uint32_t nodes_count;
    Mesh *meshes;
    uint32_t meshes_count;
    Material *materials;
    uint32_t materials_count;
    Texture *textures;
    uint32_t textures_count;
#ifdef BYTESIZED_USE_SKINNING
    Animation *animations;
    uint32_t animations_count;
#endif
};

} // namespace library