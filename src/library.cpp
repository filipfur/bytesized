#include "library.h"

#include "json.hpp"
#include "logging.h"

#define GLB_MAGIC 0x46546C67
#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

#define LIBRARY__BUFFER_COUNT 10
#define LIBRARY__BUFFER_VIEW_COUNT 1000
#define LIBRARY__ACCESSOR_COUNT 1000
#define LIBRARY__TEXTURESAMPLER_COUNT 100
#define LIBRARY__IMAGE_COUNT 100
#define LIBRARY__TEXTURE_COUNT 100
#define LIBRARY__MATERIAL_COUNT 100
#define LIBRARY__SKIN_COUNT 10
#define LIBRARY__MESH_COUNT 100
#define LIBRARY__NODE_COUNT 200
#define LIBRARY__SCENE_COUNT 10
#define LIBRARY__SAMPLER_COUNT 20
#define LIBRARY__CHANNEL_COUNT 20
#define LIBRARY__ANIMATION_COUNT 20
#define LIBRARY__COLLECTION_COUNT 10

static gltf::Buffer BUFFERS[LIBRARY__BUFFER_COUNT] = {};
static gltf::Bufferview BUFFERVIEWS[LIBRARY__BUFFER_VIEW_COUNT] = {};
static gltf::Accessor ACCESSORS[LIBRARY__ACCESSOR_COUNT] = {};
static gltf::TextureSampler TEXTURESAMPLERS[LIBRARY__TEXTURESAMPLER_COUNT] = {};
static gltf::Image IMAGES[LIBRARY__IMAGE_COUNT] = {};
static gltf::Material MATERIALS[LIBRARY__MATERIAL_COUNT] = {};
static gltf::Texture TEXTURES[LIBRARY__TEXTURE_COUNT] = {};
static gltf::Skin SKINS[LIBRARY__SKIN_COUNT] = {};
static gltf::Mesh MESHES[LIBRARY__MESH_COUNT] = {};
static gltf::Node NODES[LIBRARY__NODE_COUNT] = {};
static gltf::Scene SCENES[LIBRARY__SCENE_COUNT] = {};
static gltf::Animation ANIMATIONS[LIBRARY__ANIMATION_COUNT] = {};
static library::Collection COLLECTIONS[LIBRARY__COLLECTION_COUNT] = {};

static uint32_t ACTIVE_BUFFERS{0};
static uint32_t ACTIVE_BUFFERVIEWS{0};
static uint32_t ACTIVE_ACCESSORS{0};
static uint32_t ACTIVE_TEXTURESAMPLERS{0};
static uint32_t ACTIVE_IMAGES{0};
static uint32_t ACTIVE_MATERIALS{0};
static uint32_t ACTIVE_TEXTURES{0};
static uint32_t ACTIVE_SKINS{0};
static uint32_t ACTIVE_MESHES{0};
static uint32_t ACTIVE_NODES{0};
static uint32_t ACTIVE_SCENES{0};
static uint32_t ACTIVE_ANIMATIONS{0};
static uint32_t ACTIVE_COLLECTIONS{0};

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
    PARSE_SKINS,
    PARSE_ACCESSORS,
    PARSE_BUFFERVIEWS,
    PARSE_BUFFERS,
    PARSE_ANIM_SAMPLERS,
    PARSE_ANIM_CHANNELS,
    PARSE_ANIMATIONS,
    PARSE_MATERIALS,
    PARSE_MATERIALS_BASETEX,
    PARSE_MATERIALS_BASECOLOR,
    PARSE_TEXTURES,
    PARSE_IMAGES,
    PARSE_SAMPLERS,
};

struct GLBJsonStream : public JsonStream {

    JsonParseState state{PARSE_IDLE};

    gltf::Buffer *sBuffer{BUFFERS};
    gltf::Bufferview *sBufferview{BUFFERVIEWS};
    gltf::Accessor *sAccessor{ACCESSORS};
    gltf::TextureSampler *sTextureSampler{TEXTURESAMPLERS};
    gltf::Image *sImage{IMAGES};
    gltf::Material *sMaterial{MATERIALS};
    gltf::Texture *sTexture{TEXTURES};
    gltf::Skin *sSkin{SKINS};
    gltf::Mesh *sMesh{MESHES};
    gltf::Node *sNode{NODES};
    gltf::Scene *sScene{SCENES};
    gltf::Sampler *sSampler{nullptr};
    gltf::Channel *sChannel{nullptr};
    gltf::Animation *sAnimation{ANIMATIONS};

    gltf::Buffer *cBuffer{nullptr};
    gltf::Bufferview *cBufferview{nullptr};
    gltf::Accessor *cAccessor{nullptr};
    gltf::TextureSampler *cTextureSampler{nullptr};
    gltf::Image *cImage{nullptr};
    gltf::Material *cMaterial{nullptr};
    gltf::Texture *cTexture{nullptr};
    gltf::Skin *cSkin{nullptr};
    gltf::Mesh *cMesh{nullptr};
    gltf::Node *cNode{nullptr};
    gltf::Scene *cScene{nullptr};
    gltf::Sampler *cSampler{nullptr};
    gltf::Channel *cChannel{nullptr};
    gltf::Animation *cAnimation{nullptr};

    virtual void object(const std::string_view &parent, const std::string_view &key) override {
        switch (state) {
        case PARSE_SCENE_NODES:
        case PARSE_SCENES:
            state = PARSE_SCENES;
            cScene = SCENES + ACTIVE_SCENES++;
            assert(ACTIVE_SCENES < LIBRARY__SCENE_COUNT);
            break;
        case PARSE_NODES:
            cNode = NODES + ACTIVE_NODES++;
            cNode->scene = cScene;
            ++cCollection->nodes_count;
            assert(ACTIVE_NODES < LIBRARY__NODE_COUNT);
            break;
        case PARSE_MESHES:
            if (parent == "meshes") {
                cMesh = MESHES + ACTIVE_MESHES++;
                ++cCollection->meshes_count;
                assert(ACTIVE_MESHES < LIBRARY__MESH_COUNT);
            } else if (parent == "primitives") {
                if (key == "") {
                    cMesh->primitives.emplace_back();
                }
            }
            break;
        case PARSE_SKINS:
            cSkin = SKINS + ACTIVE_SKINS++;
            assert(ACTIVE_SKINS < LIBRARY__SKIN_COUNT);
            break;
        case PARSE_ACCESSORS:
            cAccessor = ACCESSORS + ACTIVE_ACCESSORS++;
            assert(ACTIVE_ACCESSORS < LIBRARY__ACCESSOR_COUNT);
            break;
        case PARSE_BUFFERVIEWS:
            cBufferview = BUFFERVIEWS + ACTIVE_BUFFERVIEWS++;
            assert(ACTIVE_BUFFERVIEWS < LIBRARY__BUFFER_VIEW_COUNT);
            break;
        case PARSE_BUFFERS:
            cBuffer = BUFFERS + ACTIVE_BUFFERS++;
            assert(ACTIVE_BUFFERS < LIBRARY__BUFFER_COUNT);
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
        } else if (key == "skins") {
            state = PARSE_SKINS;
        } else if (key == "accessors") {
            state = PARSE_ACCESSORS;
        } else if (key == "bufferViews") {
            state = PARSE_BUFFERVIEWS;
        } else if (key == "buffers") {
            state = PARSE_BUFFERS;
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
        } else if (key == "materials") {
            state = PARSE_MATERIALS;
        } else if (key == "images") {
            state = PARSE_IMAGES;
        } else if (key == "textures") {
            state = PARSE_TEXTURES;
        }
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
    virtual void value(const std::string_view &parent, const std::string_view &key,
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
            case PARSE_SKINS:
                cSkin->name = val;
                break;
            case PARSE_ANIM_CHANNELS:
            case PARSE_ANIM_SAMPLERS:
            case PARSE_ANIMATIONS:
                cAnimation->name = val;
                break;
            case PARSE_MATERIALS:
                cMaterial->name = val;
                break;
            case PARSE_IMAGES:
                cImage->name = val;
                break;
            default:
                break;
            }
        }
        switch (state) {
        case PARSE_ACCESSORS:
            if (key == "type") {
                if (val == "SCALAR") {
                    cAccessor->type = gltf::Accessor::SCALAR;
                } else if (val == "VEC2") {
                    cAccessor->type = gltf::Accessor::VEC2;
                } else if (val == "VEC3") {
                    cAccessor->type = gltf::Accessor::VEC3;
                } else if (val == "VEC4") {
                    cAccessor->type = gltf::Accessor::VEC4;
                } else if (val == "MAT4") {
                    cAccessor->type = gltf::Accessor::MAT4;
                } else {
                    assert(false); // unsupported type
                }
            }
            break;
        case PARSE_ANIM_CHANNELS:
            if (key == "path") {
                if (val == "translation") {
                    cChannel->type = gltf::Channel::TRANSLATION;
                } else if (val == "rotation") {
                    cChannel->type = gltf::Channel::ROTATION;
                } else if (val == "scale") {
                    cChannel->type = gltf::Channel::SCALE;
                }
            }
            break;
        case PARSE_ANIM_SAMPLERS:
            if (key == "interpolation") {
                if (val == "STEP") {
                    cSampler->type = gltf::Sampler::STEP;
                } else if (val == "LINEAR") {
                    cSampler->type = gltf::Sampler::LINEAR;
                }
            }
            break;
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
            } else if (key == "skin") {
                cNode->skin = sSkin + val;
            }
            break;
        case PARSE_SCENE_NODES:
            cScene->nodes.emplace_back(sNode + val);
            break;
        case PARSE_MESHES:
            if (key == "POSITION") {
                cMesh->primitives.back().attributes[gltf::Primitive::POSITION] = sAccessor + val;
            } else if (key == "NORMAL") {
                cMesh->primitives.back().attributes[gltf::Primitive::NORMAL] = sAccessor + val;
            } else if (key == "TEXCOORD_0") {
                cMesh->primitives.back().attributes[gltf::Primitive::TEXCOORD_0] = sAccessor + val;
            } else if (key == "JOINTS_0") {
                cMesh->primitives.back().attributes[gltf::Primitive::JOINTS_0] = sAccessor + val;
            } else if (key == "WEIGHTS_0") {
                cMesh->primitives.back().attributes[gltf::Primitive::WEIGHTS_0] = sAccessor + val;
            } else if (key == "indices") {
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
        case PARSE_SKINS:
            if (parent == "joints") {
                cSkin->joints.emplace_back(sNode + val);
            } else if (key == "inverseBindMatrices") {
                cSkin->inverseBindMatrices = sAccessor + val;
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
        case PARSE_MATERIALS_BASETEX:
            if (key == "index") {
                cMaterial->textures[gltf::Material::TEX_DIFFUSE] = sTexture + val;
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

inline static void parseChunk(glb_chunk *chunk) {
    static gltf::Buffer *curBuf{BUFFERS};
    switch (chunk->type) {
    case GLB_CHUNK_TYPE_JSON: {
        js.state = PARSE_IDLE;
        js.sScene = SCENES + ACTIVE_SCENES;
        js.sNode = NODES + ACTIVE_NODES;
        js.sMesh = MESHES + ACTIVE_MESHES;
        js.sSkin = SKINS + ACTIVE_SKINS;
        js.sAccessor = ACCESSORS + ACTIVE_ACCESSORS;
        js.sBufferview = BUFFERVIEWS + ACTIVE_BUFFERVIEWS;
        js.sBuffer = BUFFERS + ACTIVE_BUFFERS;
        js.sTextureSampler = TEXTURESAMPLERS + ACTIVE_TEXTURESAMPLERS;
        js.sAnimation = ANIMATIONS + ACTIVE_ANIMATIONS;
        js.sImage = IMAGES + ACTIVE_IMAGES;
        js.sTexture = TEXTURES + ACTIVE_TEXTURES;
        js.sMaterial = MATERIALS + ACTIVE_MATERIALS;
        if (cCollection == nullptr) {
            cCollection = COLLECTIONS;
        } else {
            ++cCollection;
        }
        ++ACTIVE_COLLECTIONS;
        cCollection->scene = js.sScene;
        cCollection->animations = js.sAnimation;
        cCollection->nodes = js.sNode;
        cCollection->meshes = js.sMesh;
        cCollection->materials = js.sMaterial;
        cCollection->textures = js.sTexture;
        loadJson(js, (char *)(chunk + 1), chunk->length);
        break;
    }
    case GLB_CHUNK_TYPE_BIN:
        curBuf->data = (const unsigned char *)(chunk + 1);
        assert(curBuf->length == chunk->length);
        ++curBuf;
        break;
    default:
        break;
    }
}

library::Collection *library::loadGLB(const unsigned char *glb) {
    glb_header *header = (glb_header *)glb;
    assert(header->magic == GLB_MAGIC);
    glb_chunk *chunk = (glb_chunk *)(header + 1);
    size_t i{header->length - sizeof(glb_header)};
    while (i > 0) {
        parseChunk(chunk);
        i -= chunk->length + sizeof(glb_chunk);
        chunk = (glb_chunk *)((unsigned char *)chunk + chunk->length + sizeof(glb_chunk));
    }
    return cCollection;
}

void library::print() {
    LOG_INFO("# SCENES: %u NODES: %u MESHES: %u ACCESSORS: %u BUFFERVIEWS: %u", ACTIVE_SCENES,
             ACTIVE_NODES, ACTIVE_MESHES, ACTIVE_ACCESSORS, ACTIVE_BUFFERVIEWS);
    for (size_t i{0}; i < ACTIVE_SCENES; ++i) {
        LOG_TRACE("SCENE: name=%s", SCENES[i].name.c_str());
    }
    for (size_t i{0}; i < ACTIVE_ANIMATIONS; ++i) {
        LOG_TRACE("ANIMATIONS: name=%s", ANIMATIONS[i].name.c_str());
    }
    for (size_t i{0}; i < ACTIVE_NODES; ++i) {
        LOG_TRACE("NODE: name=%s [%.1f %.1f %.1f]", NODES[i].name.c_str(), NODES[i].translation.x,
                  NODES[i].translation.y, NODES[i].translation.z);
    }
}