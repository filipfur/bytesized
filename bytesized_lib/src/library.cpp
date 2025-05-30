#include "library.h"

#include "glm/gtc/type_ptr.hpp"
#include "json.hpp"
#include "logging.h"
#include <list>

#define GLB_MAGIC 0x46546C67
#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

#define LIBRARY__BUFFER_COUNT BYTESIZED_MESH_COUNT + BYTESIZED_TEXTURE_COUNT
#define LIBRARY__BUFFERVIEW_COUNT BYTESIZED_VERTEXBUFFER_COUNT
#define LIBRARY__ACCESSOR_COUNT BYTESIZED_VERTEXBUFFER_COUNT
#define LIBRARY__TEXTURESAMPLER_COUNT BYTESIZED_TEXTURE_COUNT
#define LIBRARY__IMAGE_COUNT BYTESIZED_TEXTURE_COUNT
#define LIBRARY__TEXTURE_COUNT BYTESIZED_TEXTURE_COUNT
#define LIBRARY__MATERIAL_COUNT BYTESIZED_MATERIAL_COUNT
#define LIBRARY__MESH_COUNT BYTESIZED_MESH_COUNT
#define LIBRARY__NODE_COUNT BYTESIZED_NODE_COUNT
#define LIBRARY__SCENE_COUNT BYTESIZED_SCENE_COUNT
#ifdef BYTESIZED_USE_SKINNING
#define LIBRARY__SAMPLER_COUNT BYTESIZED_ANIMATION_COUNT
#define LIBRARY__CHANNEL_COUNT BYTESIZED_ANIMATION_COUNT * 3
#define LIBRARY__SKIN_COUNT BYTESIZED_SKIN_COUNT
#define LIBRARY__ANIMATION_COUNT BYTESIZED_ANIMATION_COUNT
#endif
#define LIBRARY__COLLECTION_COUNT 50

static library::Buffer BUFFERS[LIBRARY__BUFFER_COUNT] = {};
static library::Bufferview BUFFERVIEWS[LIBRARY__BUFFERVIEW_COUNT] = {};
static library::Accessor ACCESSORS[LIBRARY__ACCESSOR_COUNT] = {};
static library::TextureSampler TEXTURESAMPLERS[LIBRARY__TEXTURESAMPLER_COUNT] = {};
static library::Image IMAGES[LIBRARY__IMAGE_COUNT] = {};
static library::Material MATERIALS[LIBRARY__MATERIAL_COUNT] = {};
static library::Texture TEXTURES[LIBRARY__TEXTURE_COUNT] = {};
static library::Mesh MESHS[LIBRARY__MESH_COUNT] = {};
static library::Node NODES[LIBRARY__NODE_COUNT] = {};
static library::Scene SCENES[LIBRARY__SCENE_COUNT] = {};
#ifdef BYTESIZED_USE_SKINNING
static library::Skin SKINS[LIBRARY__SKIN_COUNT] = {};
static library::Animation ANIMATIONS[LIBRARY__ANIMATION_COUNT] = {};
#endif
static library::Collection COLLECTIONS[LIBRARY__COLLECTION_COUNT] = {};

static uint32_t ACTIVE_BUFFERS{0};
static uint32_t ACTIVE_BUFFERVIEWS{0};
static uint32_t ACTIVE_ACCESSORS{0};
static uint32_t ACTIVE_TEXTURESAMPLERS{0};
static uint32_t ACTIVE_IMAGES{0};
static uint32_t ACTIVE_MATERIALS{0};
static uint32_t ACTIVE_TEXTURES{0};
static uint32_t ACTIVE_MESHS{0};
static uint32_t ACTIVE_NODES{0};
static uint32_t ACTIVE_SCENES{0};
#ifdef BYTESIZED_USE_SKINNING
static uint32_t ACTIVE_SKINS{0};
static uint32_t ACTIVE_ANIMATIONS{0};
#endif
static uint32_t ACTIVE_COLLECTIONS{0};

#define PRINT_USAGE(var)                                                                           \
    do {                                                                                           \
        printf("%sS: %d / %d\n", #var, ACTIVE_##var##S, LIBRARY__##var##_COUNT);                   \
    } while (0);

void library::printAllocations() {
    printf("\nLibrary allocations:\n");
    PRINT_USAGE(BUFFER);
    PRINT_USAGE(BUFFERVIEW);
    PRINT_USAGE(ACCESSOR);
    PRINT_USAGE(TEXTURESAMPLER);
    PRINT_USAGE(IMAGE);
    PRINT_USAGE(MATERIAL);
    PRINT_USAGE(TEXTURE);
    PRINT_USAGE(MESH);
    PRINT_USAGE(NODE);
    PRINT_USAGE(SCENE);
#ifdef BYTESIZED_USE_SKINNING
    PRINT_USAGE(SKIN);
    PRINT_USAGE(ANIMATION);
#endif
    PRINT_USAGE(COLLECTION);
    printf("-------------------------\n\n");
}

static library::Collection *cCollection{nullptr};

static std::vector<float> floatArr;
static bool parseFloatVec3(glm::vec3 &v3, float val) {
    floatArr.emplace_back(val);
    if (floatArr.size() == 3) {
        v3.x = floatArr[0];
        v3.y = floatArr[1];
        v3.z = floatArr[2];
        floatArr.clear();
        return true;
    }
    return false;
}
static bool parseFloatVec4(glm::vec4 &v4, float val) {
    floatArr.emplace_back(val);
    if (floatArr.size() == 4) {
        v4.x = floatArr[0];
        v4.y = floatArr[1];
        v4.z = floatArr[2];
        v4.w = floatArr[3];
        floatArr.clear();
        return true;
    }
    return false;
}
static bool parseFloatQuat(glm::quat &q, float val) {
    floatArr.emplace_back(val);
    if (floatArr.size() == 4) {
        q.x = floatArr[0];
        q.y = floatArr[1];
        q.z = floatArr[2];
        q.w = floatArr[3];
        floatArr.clear();
        return true;
    }
    return false;
}

struct glb_header {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
};

struct glb_chunk {
    uint32_t length;
    uint32_t type;
};

enum JsonParseState {
    PARSE_IDLE,
    PARSE_SCENES,
    PARSE_SCENE_NODES,
    PARSE_NODES,
    PARSE_NODE_TRANSLATION,
    PARSE_NODE_ROTATION,
    PARSE_NODE_SCALE,
    PARSE_MESHES,
    PARSE_ACCESSORS,
    PARSE_BUFFERVIEWS,
    PARSE_BUFFERS,
    PARSE_MATERIALS,
    PARSE_MATERIALS_BASETEX,
    PARSE_MATERIALS_BASECOLOR,
    PARSE_TEXTURES,
    PARSE_IMAGES,
    PARSE_SAMPLERS,
#ifdef BYTESIZED_USE_SKINNING
    PARSE_SKINS,
    PARSE_ANIM_SAMPLERS,
    PARSE_ANIM_CHANNELS,
    PARSE_ANIMATIONS,
#endif
};

struct GLBJsonStream : public JsonStream {

    JsonParseState state{PARSE_IDLE};

    library::Buffer *sBuffer{BUFFERS};
    library::Bufferview *sBufferview{BUFFERVIEWS};
    library::Accessor *sAccessor{ACCESSORS};
    library::TextureSampler *sTextureSampler{TEXTURESAMPLERS};
    library::Image *sImage{IMAGES};
    library::Material *sMaterial{MATERIALS};
    library::Texture *sTexture{TEXTURES};
    library::Mesh *sMesh{MESHS};
    library::Node *sNode{NODES};
    library::Scene *sScene{SCENES};
#ifdef BYTESIZED_USE_SKINNING
    library::Sampler *sSampler{nullptr};
    library::Skin *sSkin{SKINS};
    library::Channel *sChannel{nullptr};
    library::Animation *sAnimation{ANIMATIONS};
#endif

    library::Buffer *cBuffer{nullptr};
    library::Bufferview *cBufferview{nullptr};
    library::Accessor *cAccessor{nullptr};
    library::TextureSampler *cTextureSampler{nullptr};
    library::Image *cImage{nullptr};
    library::Material *cMaterial{nullptr};
    library::Texture *cTexture{nullptr};
    library::Mesh *cMesh{nullptr};
    library::Node *cNode{nullptr};
    library::Scene *cScene{nullptr};
#ifdef BYTESIZED_USE_SKINNING
    library::Sampler *cSampler{nullptr};
    library::Skin *cSkin{nullptr};
    library::Channel *cChannel{nullptr};
    library::Animation *cAnimation{nullptr};
#endif

    virtual void object(const std::string_view &parent, const std::string_view &key) override {
        switch (state) {
        case PARSE_SCENE_NODES:
        case PARSE_SCENES:
            state = PARSE_SCENES;
            cScene = SCENES + ACTIVE_SCENES++;
            assert(ACTIVE_SCENES <= LIBRARY__SCENE_COUNT);
            break;
        case PARSE_NODES:
            cNode = NODES + ACTIVE_NODES++;
            cNode->scene = cScene;
            ++cCollection->nodes_count;
            assert(ACTIVE_NODES <= LIBRARY__NODE_COUNT);
            break;
        case PARSE_MESHES:
            if (parent == "meshes") {
                cMesh = MESHS + ACTIVE_MESHS++;
                ++cCollection->meshes_count;
                assert(ACTIVE_MESHS <= LIBRARY__MESH_COUNT);
            } else if (parent == "primitives") {
                if (key == "") {
                    cMesh->primitives.emplace_back();
                }
            }
            break;
        case PARSE_ACCESSORS:
            cAccessor = ACCESSORS + ACTIVE_ACCESSORS++;
            assert(ACTIVE_ACCESSORS <= LIBRARY__ACCESSOR_COUNT);
            break;
        case PARSE_BUFFERVIEWS:
            cBufferview = BUFFERVIEWS + ACTIVE_BUFFERVIEWS++;
            assert(ACTIVE_BUFFERVIEWS <= LIBRARY__BUFFERVIEW_COUNT);
            break;
        case PARSE_BUFFERS:
            cBuffer = BUFFERS + ACTIVE_BUFFERS++;
            assert(ACTIVE_BUFFERS <= LIBRARY__BUFFER_COUNT);
            break;
        case PARSE_SAMPLERS:
            cTextureSampler = TEXTURESAMPLERS + ACTIVE_TEXTURESAMPLERS++;
            break;
        case PARSE_IMAGES:
            cImage = IMAGES + ACTIVE_IMAGES++;
            break;
        case PARSE_TEXTURES:
            cTexture = TEXTURES + ACTIVE_TEXTURES++;
            ++cCollection->textures_count;
            break;
        case PARSE_MATERIALS:
            if (key == "baseColorTexture") {
                state = PARSE_MATERIALS_BASETEX;
            } else if (key.length() == 0) {
                cMaterial = MATERIALS + ACTIVE_MATERIALS++;
                cMaterial->baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
                ++cCollection->materials_count;
            }
            break;
#ifdef BYTESIZED_USE_SKINNING
        case PARSE_SKINS:
            cSkin = SKINS + ACTIVE_SKINS++;
            assert(ACTIVE_SKINS <= LIBRARY__SKIN_COUNT);
            break;
        case PARSE_ANIM_SAMPLERS:
        case PARSE_ANIM_CHANNELS:
        case PARSE_ANIMATIONS:
            if (key == "target") {
                // pass
            } else if (parent == "samplers") {
                if (cSampler == nullptr) {
                    cSampler = cAnimation->samplers;
                } else {
                    ++cSampler;
                }
                state = PARSE_ANIM_SAMPLERS;
            } else if (parent == "channels") {
                if (cChannel == nullptr) {
                    cChannel = cAnimation->channels;
                } else {
                    ++cChannel;
                }
                state = PARSE_ANIM_CHANNELS;
            } else {
                state = PARSE_ANIMATIONS;
                cAnimation = ANIMATIONS + ACTIVE_ANIMATIONS++;
                ++cCollection->animations_count;
                sSampler = cAnimation->samplers;
                sChannel = cAnimation->channels;
                cSampler = nullptr;
                cChannel = nullptr;
            }
            break;
#endif
        default:
            break;
        }
    }
    virtual void array(const std::string_view &parent, const std::string_view &key) override {
        if (key == "scenes") {
            state = PARSE_SCENES;
        } else if (key == "nodes") {
            switch (state) {
            case PARSE_SCENES:
                state = PARSE_SCENE_NODES;
                break;
            case PARSE_SCENE_NODES:
                state = PARSE_NODES;
                break;
            default:
                break;
            }
        } else if (key == "meshes") {
            state = PARSE_MESHES;
        } else if (key == "accessors") {
            state = PARSE_ACCESSORS;
        } else if (key == "bufferViews") {
            state = PARSE_BUFFERVIEWS;
        } else if (key == "buffers") {
            state = PARSE_BUFFERS;
        } else if (key == "materials") {
            state = PARSE_MATERIALS;
        } else if (key == "images") {
            state = PARSE_IMAGES;
        } else if (key == "textures") {
            state = PARSE_TEXTURES;
        }
#ifdef BYTESIZED_USE_SKINNING
        else if (key == "skins") {
            state = PARSE_SKINS;
        } else if (key == "animations") {
            state = PARSE_ANIMATIONS;
        } else if (key == "samplers") {
            if (parent == "animations") {
                state = PARSE_ANIM_SAMPLERS;
            } else {
                state = PARSE_SAMPLERS;
            }
        } else if (key == "channels") {
            state = PARSE_ANIM_CHANNELS;
        }
#endif
        switch (state) {
        case PARSE_NODES:
            if (key == "translation") {
                state = PARSE_NODE_TRANSLATION;
            } else if (key == "rotation") {
                state = PARSE_NODE_ROTATION;
            } else if (key == "scale") {
                state = PARSE_NODE_SCALE;
            }
            break;
        case PARSE_MATERIALS:
            if (key == "baseColorFactor") {
                state = PARSE_MATERIALS_BASECOLOR;
            }
            break;
        default:
            break;
        }
    }
    virtual void value(const std::string_view & /*parent*/, const std::string_view &key,
                       const std::string_view &val) override {
        if (key == "name") {
            switch (state) {
            case PARSE_SCENES:
                cScene->name = val;
                break;
            case PARSE_NODES:
                cNode->name = val;
                break;
            case PARSE_MESHES:
                cMesh->name = val;
                break;
            case PARSE_MATERIALS:
                cMaterial->name = val;
                break;
            case PARSE_IMAGES:
                cImage->name = val;
                break;
#ifdef BYTESIZED_USE_SKINNING
            case PARSE_SKINS:
                cSkin->name = val;
                break;
            case PARSE_ANIM_CHANNELS:
            case PARSE_ANIM_SAMPLERS:
            case PARSE_ANIMATIONS:
                cAnimation->name = val;
                break;
#endif
            default:
                break;
            }
        }
        switch (state) {
        case PARSE_ACCESSORS:
            if (key == "type") {
                if (val == "SCALAR") {
                    cAccessor->type = library::Accessor::SCALAR;
                } else if (val == "VEC2") {
                    cAccessor->type = library::Accessor::VEC2;
                } else if (val == "VEC3") {
                    cAccessor->type = library::Accessor::VEC3;
                } else if (val == "VEC4") {
                    cAccessor->type = library::Accessor::VEC4;
                } else if (val == "MAT4") {
                    cAccessor->type = library::Accessor::MAT4;
                } else {
                    assert(false); // unsupported type
                }
            }
            break;
#ifdef BYTESIZED_USE_SKINNING
        case PARSE_ANIM_CHANNELS:
            if (key == "path") {
                if (val == "translation") {
                    cChannel->type = library::Channel::TRANSLATION;
                } else if (val == "rotation") {
                    cChannel->type = library::Channel::ROTATION;
                } else if (val == "scale") {
                    cChannel->type = library::Channel::SCALE;
                }
            }
            break;
        case PARSE_ANIM_SAMPLERS:
            if (key == "interpolation") {
                if (val == "STEP") {
                    cSampler->type = library::Sampler::STEP;
                } else if (val == "LINEAR") {
                    cSampler->type = library::Sampler::LINEAR;
                }
            }
            break;
#endif
        default:
            break;
        }
    }
    virtual void value(const std::string_view &parent, const std::string_view &key,
                       int val) override {
        switch (state) {
        case PARSE_NODES:
            if (key == "mesh") {
                cNode->mesh = sMesh + val;
            } else if (parent == "children") {
                cNode->children.emplace_back(sNode + val);
            }
#ifdef BYTESIZED_USE_SKINNING
            else if (key == "skin") {
                cNode->skin = sSkin + val;
            }
#endif
            break;
        case PARSE_SCENE_NODES:
            cScene->nodes.emplace_back(sNode + val);
            break;
        case PARSE_MESHES:
            if (key == "POSITION") {
                cMesh->primitives.back().attributes[library::Primitive::POSITION] = sAccessor + val;
            } else if (key == "NORMAL") {
                cMesh->primitives.back().attributes[library::Primitive::NORMAL] = sAccessor + val;
            } else if (key == "TEXCOORD_0") {
                cMesh->primitives.back().attributes[library::Primitive::TEXCOORD_0] =
                    sAccessor + val;
            } else if (key == "COLOR_0") {
                cMesh->primitives.back().attributes[library::Primitive::COLOR_0] = sAccessor + val;
            } else if (key == "COLOR_1") {
                cMesh->primitives.back().attributes[library::Primitive::COLOR_1] = sAccessor + val;
            }
#ifdef BYTESIZED_USE_SKINNING
            else if (key == "JOINTS_0") {
                cMesh->primitives.back().attributes[library::Primitive::JOINTS_0] = sAccessor + val;
            } else if (key == "WEIGHTS_0") {
                cMesh->primitives.back().attributes[library::Primitive::WEIGHTS_0] =
                    sAccessor + val;
            }
#endif
            else if (key == "indices") {
                cMesh->primitives.back().indices = sAccessor + val;
            } else if (key == "material") {
                cMesh->primitives.back().material = sMaterial + val;
            }
            break;
        case PARSE_ACCESSORS:
            if (key == "bufferView") {
                cAccessor->bufferView = sBufferview + val;
            } else if (key == "componentType") {
                cAccessor->componentType = val;
            } else if (key == "count") {
                cAccessor->count = val;
            } else if (key == "normalized") {
                printf("Normalized!\n");
                // pass
            }
            break;
        case PARSE_BUFFERVIEWS:
            if (key == "buffer") {
                cBufferview->buffer = sBuffer + val;
            } else if (key == "byteLength") {
                cBufferview->length = val;
            } else if (key == "byteOffset") {
                cBufferview->offset = val;
            } else if (key == "target") {
                cBufferview->target = val;
            }
            break;
        case PARSE_BUFFERS:
            if (key == "byteLength") {
                cBuffer->length = val;
            }
            break;
        case PARSE_NODE_TRANSLATION:
            if (parseFloatVec3(cNode->translation.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_NODE_SCALE:
            if (parseFloatVec3(cNode->scale.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_NODE_ROTATION:
            if (parseFloatQuat(cNode->rotation.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_MATERIALS_BASECOLOR:
            if (parseFloatVec4(cMaterial->baseColor, val)) {
                state = PARSE_MATERIALS;
            }
            break;
        case PARSE_MATERIALS_BASETEX:
            if (key == "index") {
                cMaterial->textures[library::Material::TEX_DIFFUSE] = sTexture + val;
                state = PARSE_MATERIALS;
            }
            break;
        case PARSE_MATERIALS:
            if (key == "metallicFactor") {
                cMaterial->metallic = val;
            } else if (key == "roughnessFactor") {
                cMaterial->roughness = val;
            }
            break;
        case PARSE_TEXTURES:
            if (key == "source") {
                cTexture->image = sImage + val;
            } else if (key == "sampler") {
                cTexture->sampler = sTextureSampler + val;
            }
            break;
        case PARSE_IMAGES:
            if (key == "bufferView") {
                cImage->view = sBufferview + val;
            }
            break;
#ifdef BYTESIZED_USE_SKINNING
        case PARSE_SKINS:
            if (parent == "joints") {
                cSkin->joints.emplace_back(sNode + val);
            } else if (key == "inverseBindMatrices") {
                cSkin->ibmData = sAccessor + val;
            }
        case PARSE_ANIM_CHANNELS:
            if (key == "sampler") {
                cChannel->sampler = sSampler + val;
            } else if (key == "node") {
                cChannel->targetNode = sNode + val;
            }
            break;
        case PARSE_ANIM_SAMPLERS:
            if (key == "input") {
                cSampler->input = sAccessor + val;
            } else if (key == "output") {
                cSampler->output = sAccessor + val;
            }
            break;
#endif
        default:
            break;
        }
    }

    virtual void value(const std::string_view &parent, const std::string_view &key,
                       double val) override {
        switch (state) {
        case PARSE_ACCESSORS:
            if (parent == "max") {
                // pass
            } else if (parent == "min") {
                // pass
            }
            break;
        case PARSE_NODE_TRANSLATION:
            if (parseFloatVec3(cNode->translation.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_NODE_SCALE:
            if (parseFloatVec3(cNode->scale.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_NODE_ROTATION:
            if (parseFloatQuat(cNode->rotation.data(), val)) {
                state = PARSE_NODES;
            }
            break;
        case PARSE_MATERIALS_BASECOLOR:
            if (parseFloatVec4(cMaterial->baseColor, val)) {
                state = PARSE_MATERIALS;
            }
            break;
        case PARSE_MATERIALS:
            if (key == "metallicFactor") {
                cMaterial->metallic = val;
            } else if (key == "roughnessFactor") {
                cMaterial->roughness = val;
            }
            break;
        default:
            break;
        }
    }
};

static GLBJsonStream js;

std::list<std::vector<uint8_t>> _copiedBuffers;

inline static void _parseChunk(glb_chunk *chunk, bool copyBuffers) {
    switch (chunk->type) {
    case GLB_CHUNK_TYPE_JSON: {
        printf("%.*s\n", (int)chunk->length, (const char *)(chunk + 1));
        js.state = PARSE_IDLE;
        js.sScene = SCENES + ACTIVE_SCENES;
        js.sNode = NODES + ACTIVE_NODES;
        js.sMesh = MESHS + ACTIVE_MESHS;
        js.sAccessor = ACCESSORS + ACTIVE_ACCESSORS;
        js.sBufferview = BUFFERVIEWS + ACTIVE_BUFFERVIEWS;
        js.sBuffer = BUFFERS + ACTIVE_BUFFERS;
        js.sTextureSampler = TEXTURESAMPLERS + ACTIVE_TEXTURESAMPLERS;
        js.sImage = IMAGES + ACTIVE_IMAGES;
        js.sTexture = TEXTURES + ACTIVE_TEXTURES;
        js.sMaterial = MATERIALS + ACTIVE_MATERIALS;
#ifdef BYTESIZED_USE_SKINNING
        js.sSkin = SKINS + ACTIVE_SKINS;
        js.sAnimation = ANIMATIONS + ACTIVE_ANIMATIONS;
#endif
        cCollection = COLLECTIONS + ACTIVE_COLLECTIONS++;
        cCollection->scene = js.sScene;
        cCollection->nodes = js.sNode;
        cCollection->meshes = js.sMesh;
        cCollection->materials = js.sMaterial;
        cCollection->textures = js.sTexture;
#ifdef BYTESIZED_USE_SKINNING
        cCollection->animations = js.sAnimation;
#endif
        loadJson(js, (char *)(chunk + 1), chunk->length);
        break;
    }
    case GLB_CHUNK_TYPE_BIN:
        assert(ACTIVE_BUFFERS > 0);
        if (copyBuffers) {
            library::Buffer *buf = (BUFFERS + ACTIVE_BUFFERS - 1);
            uint8_t *data = (uint8_t *)(chunk + 1);
            buf->data = _copiedBuffers.emplace_back(data, data + chunk->length).data();
            assert(buf->length == chunk->length);
        } else {
            library::Buffer *buf = (BUFFERS + ACTIVE_BUFFERS - 1);
            buf->data = (uint8_t *)(chunk + 1);
            assert(buf->length == chunk->length);
        }
        break;
    default:
        break;
    }
}

library::Collection *library::loadGLB(const unsigned char *glb, bool copyBuffers) {
    glb_header *header = (glb_header *)glb;
    assert(header->magic == GLB_MAGIC);
    glb_chunk *chunk = (glb_chunk *)(header + 1);
    size_t i{header->length - sizeof(glb_header)};
    while (i > 0) {
        _parseChunk(chunk, copyBuffers);
        i -= chunk->length + sizeof(glb_chunk);
        chunk = (glb_chunk *)((unsigned char *)chunk + chunk->length + sizeof(glb_chunk));
    }
#ifdef BYTESIZED_USE_SKINNING
    for (size_t k{0}; k < cCollection->nodes_count; ++k) {
        Node &node = cCollection->nodes[k];
        if (node.skin) {
            const float *fp = (const float *)node.skin->ibmData->data();
            for (size_t j{0}; j < node.skin->joints.size(); ++j) {
                node.skin->inverseBindMatrices.push_back(glm::make_mat4(fp + j * 16));
            }
        }
    }
#endif
    return cCollection;
}

library::Collection *library::createCollection(const char *name) {
    auto collection = COLLECTIONS + ACTIVE_COLLECTIONS++;
    collection->scene = SCENES + ACTIVE_SCENES++;
    collection->scene->name = name;
    return collection;
}

library::Collection *library::builtinCollection() { return COLLECTIONS; }

bool library::isBuiltin(const library::Mesh *mesh) {
    auto *collection = builtinCollection();
    for (size_t i{0}; i < collection->meshes_count; ++i) {
        if ((collection->meshes + i) == mesh) {
            return true;
        }
    }
    return false;
}

library::Node *library::createNode() {
    auto node = NODES + ACTIVE_NODES++;
    return node;
}

library::Mesh *library::createMesh() {
    auto mesh = MESHS + ACTIVE_MESHS++;
    return mesh;
}

library::Accessor *library::createAccessor(const void *data, size_t count, size_t elemSize,
                                           library::Accessor::Type type, unsigned int componentType,
                                           unsigned int target) {
    auto accessor = ACCESSORS + ACTIVE_ACCESSORS++;
    accessor->type = type;
    accessor->componentType = componentType;
    accessor->count = count;
    accessor->bufferView = BUFFERVIEWS + ACTIVE_BUFFERVIEWS++;
    accessor->bufferView->length = count * elemSize;
    accessor->bufferView->offset = 0;
    accessor->bufferView->target = target;
    accessor->bufferView->buffer = BUFFERS + ACTIVE_BUFFERS++;
    accessor->bufferView->buffer->data = (unsigned char *)data;
    accessor->bufferView->buffer->length = accessor->bufferView->length;
    return accessor;
}
