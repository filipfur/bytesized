#include "gpu.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "debug.hpp"
#include "logging.h"
#include "opengl.h"
#include "primitive.h"
#include "shader.h"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>

#define GPU__VERTEXARRAY_COUNT 100
#define GPU__VERTEXBUFFER_COUNT 1000
#define GPU__TEXTURE_COUNT 100
#define GPU__MATERIAL_COUNT 100
#define GPU__UBO_COUNT 10
#define GPU__MESH_COUNT 100
#define GPU__PRIMITIVE_COUNT 100
#define GPU__NODE_COUNT 200
#define GPU__SCENE_COUNT 10
#define GPU__SKIN_COUNT 10
#define GPU__SHADER_COUNT 20
#define GPU__SHADERPROGRAM_COUNT 10
#define GPU__TEXT_COUNT 10
#define GPU__FRAMEBUFFER_COUNT 10

static recycler<uint32_t, GPU__VERTEXARRAY_COUNT> VAOS = {};
static recycler<uint32_t, GPU__VERTEXBUFFER_COUNT> VBOS = {};
static recycler<gpu::Texture, GPU__TEXTURE_COUNT> TEXTURES = {};
static recycler<gpu::Material, GPU__MATERIAL_COUNT> MATERIALS = {};
static recycler<gpu::UniformBuffer, GPU__UBO_COUNT> UBOS = {};
static recycler<gpu::Mesh, GPU__MESH_COUNT> MESHES = {};
static recycler<gpu::Primitive, GPU__PRIMITIVE_COUNT> PRIMITIVES = {};
static recycler<gpu::Node, GPU__NODE_COUNT> NODES = {};
static recycler<gpu::Scene, GPU__SCENE_COUNT> SCENES = {};
static recycler<gpu::Skin, GPU__SKIN_COUNT> SKINS = {};
static recycler<gpu::Shader, GPU__SHADER_COUNT> SHADERS = {};
static recycler<gpu::ShaderProgram, GPU__SHADERPROGRAM_COUNT> SHADERPROGRAMS = {};
static recycler<gpu::Text, GPU__TEXT_COUNT> TEXTS = {};
static recycler<gpu::Framebuffer, GPU__FRAMEBUFFER_COUNT> FRAMEBUFFERS = {};

static gpu::Material *_builtinMaterials[gpu::MATERIAL_COUNT];
static gpu::Material *_overrideMaterial{nullptr};

template <std::size_t W, std::size_t H, std::size_t C> struct StaticTexture {
    constexpr StaticTexture(glm::vec4 (*func)(uint32_t x, uint32_t y)) : buf{} {
        size_t i{0};
        for (size_t y{0}; y < height; ++y) {
            for (size_t x{0}; x < width; ++x) {
                glm::vec4 pixel = func(x, y);
                for (size_t b{0}; b < C; ++b) {
                    buf[i * channels + b] = static_cast<uint8_t>(pixel[b] * 255);
                }
                ++i;
            }
        }
    }
    const size_t width = W;
    const size_t height = H;
    const size_t channels = C;
    uint8_t buf[W * H * C];
};

void gpu::allocate() {
    glGenVertexArrays(GPU__VERTEXARRAY_COUNT, VAOS.data());
    glGenBuffers(GPU__VERTEXBUFFER_COUNT, VBOS.data());
    glGenTextures(GPU__TEXTURE_COUNT, (GLuint *)TEXTURES.data());
    LOG_INFO("GPU recyclers RAM usage: %.1f KB",
             (sizeof(VAOS) + sizeof(VBOS) + sizeof(TEXTURES) + sizeof(MATERIALS) + sizeof(UBOS) +
              sizeof(MESHES) + sizeof(PRIMITIVES) + sizeof(NODES) + sizeof(SCENES) + sizeof(SKINS) +
              sizeof(SHADERS) + sizeof(SHADERPROGRAMS)) *
                 1e-3f);

    _builtinMaterials[BuiltinMaterial::CHECKERS] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::CHECKERS]->color = 0xFF00FF;
    constexpr StaticTexture<8, 8, 3> checkers_tex{[](uint32_t x, uint32_t y) {
        return glm::vec4(glm::vec3(0.75f + ((y % 2 + x) % 2) * 0.25f), 1.0f);
    }};
    _builtinMaterials[BuiltinMaterial::CHECKERS]->textures.emplace(
        GL_TEXTURE0,
        createTexture(checkers_tex.buf, checkers_tex.width, checkers_tex.height,
                      static_cast<ChannelSetting>(checkers_tex.channels), GL_UNSIGNED_BYTE));

    _builtinMaterials[BuiltinMaterial::GRID_TILE] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::GRID_TILE]->color = 0x88FF88;
    constexpr size_t quad_width = 64;
    constexpr size_t quad_height = 64;
    constexpr StaticTexture<quad_width, quad_height, 4> quadratic_tex{[](uint32_t x, uint32_t y) {
        // glm::vec2 st = glm::vec2(x, y) / glm::vec2(8, 8);
        // st -= 0.5f;
        bool paint = (x <= 1 || x == (quad_width / 2 + 1) || y <= 1 || y == (quad_width / 2 + 1));
        return glm::vec4(paint ? 1.0f : 0.0f);
    }};
    _builtinMaterials[BuiltinMaterial::GRID_TILE]->textures.emplace(
        GL_TEXTURE0,
        createTexture(quadratic_tex.buf, quadratic_tex.width, quadratic_tex.height,
                      static_cast<ChannelSetting>(quadratic_tex.channels), GL_UNSIGNED_BYTE));

    _builtinMaterials[BuiltinMaterial::WHITE] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::WHITE]->color = 0xFFFFFF;
    uint8_t onePixel[]{0xFF, 0xFF, 0xFF};
    _builtinMaterials[BuiltinMaterial::WHITE]->textures.emplace(
        GL_TEXTURE0, createTexture(onePixel, 1, 1, ChannelSetting::RGB, GL_UNSIGNED_BYTE));

    createBuiltinPrimitives();
}

void gpu::dispose() {
    glDeleteVertexArrays(GPU__VERTEXARRAY_COUNT, VAOS.data());
    glDeleteBuffers(GPU__VERTEXBUFFER_COUNT, VBOS.data());
    glDeleteTextures(GPU__TEXTURE_COUNT, (GLuint *)TEXTURES.data());
    for (size_t i{0}; i < SHADERS.count(); ++i) {
        glDeleteShader(SHADERS[i].id);
    }
    for (size_t i{0}; i < SHADERPROGRAMS.count(); ++i) {
        SHADERPROGRAMS[i].vertex = nullptr;
        SHADERPROGRAMS[i].fragment = nullptr;
        glDeleteProgram(SHADERPROGRAMS[i].id);
    }
}

void gpu::setOverrideMaterial(gpu::Material *material) { _overrideMaterial = material; }

void gpu::Framebuffer::bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }
void gpu::Framebuffer::attach(uint32_t attachment, Texture *texture) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->id, 0);
    textures.emplace(attachment, texture);
}
void gpu::Framebuffer::createTexture(uint32_t attachment, uint32_t width, uint32_t height,
                                     ChannelSetting channels, uint32_t type) {
    attach(attachment, gpu::createTexture(nullptr, width, height, channels, type));
}
void gpu::Framebuffer::createDepthStencil(uint32_t width, uint32_t height) {
    this->createTexture(GL_DEPTH_STENCIL_ATTACHMENT, width, height, ChannelSetting::DS,
                        GL_UNSIGNED_INT_24_8);
}
void gpu::Framebuffer::createRenderBufferDS(uint32_t width, uint32_t height) {
    // TODO: Dispose the render buffer at some point
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
}
void gpu::Framebuffer::checkStatus() {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer is not complete: %d", id);
    }
}
void gpu::Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

gpu::Mesh *gpu::createMesh() {
    gpu::Mesh *mesh = MESHES.acquire();
    return mesh;
}

gpu::Primitive *gpu::createPrimitive(const void *vertices, size_t vertices_len,
                                     const uint16_t *indices, size_t indices_len) {
    Primitive *prim = PRIMITIVES.acquire();
    prim->vao = VAOS.acquire();
    glBindVertexArray(*prim->vao);
    uint32_t *vbo = prim->vbos.emplace_back(VBOS.acquire());
    prim->ebo = VBOS.acquire();
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_len, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *prim->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_len, indices, GL_STATIC_DRAW);
    prim->count = indices_len / sizeof(uint16_t);
    return prim;
}

gpu::Material *gpu::builtinMaterial(BuiltinMaterial builtinMaterial) {
    return _builtinMaterials[builtinMaterial];
}

gpu::Primitive *gpu::builtinPrimitive(BuiltinPrimitive builtinPrimitive) {
    return gpu::getBuiltinPrimitive(builtinPrimitive);
}

gpu::Framebuffer *gpu::createFramebuffer() {
    gpu::Framebuffer *fbo = FRAMEBUFFERS.acquire();
    glGenFramebuffers(1, &fbo->id);
    return fbo;
}

void gpu::freeFramebuffer(gpu::Framebuffer *fbo) {
    glDeleteFramebuffers(1, &fbo->id);
    fbo->textures.clear();
    FRAMEBUFFERS.free(fbo);
}

gpu::Texture *gpu::createTexture(const uint8_t *data, uint32_t width, uint32_t height,
                                 ChannelSetting channels, uint32_t type) {
    gpu::Texture *texture = TEXTURES.acquire();
    Texture_create(*texture, data, width, height, channels, type);
    return texture;
}

gpu::Texture *gpu::createTextureFromFile(const char *path) {
    int iw, ih, ic;
    const uint8_t *idata = stbi_load(path, &iw, &ih, &ic, 0);
    return createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
}

gpu::Texture *gpu::createTextureFromMem(const uint8_t *addr, uint32_t len) {
    int iw, ih, ic;
    const uint8_t *idata = stbi_load_from_memory((uint8_t *)addr, len, &iw, &ih, &ic, 0);
    return createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
}
gpu::Texture *gpu::createTexture(const gltf::Texture &texture) {
    return createTextureFromMem((uint8_t *)texture.image->view->data(),
                                texture.image->view->length);
}

gpu::Texture *gpu::createTexture(const bdf::Font &font) {
    int iw = font.pw * 16;
    int ih = font.ph * 8;
    int ic = 1;
    static uint8_t _buf[128 * 128];
    uint8_t c{0};
    for (uint8_t y{0}; y < 8; ++y) {
        for (uint8_t x{0}; x < 16; ++x) {
            for (int16_t i{0}; i < font.ph; ++i) {
                for (int16_t j{0}; j < font.pw; ++j) {
                    /*_buf[i + (int)(c / 16) * font.ph][j + (c % 16) * font.pw] =
                        (font.bitmap[c][i] & (0b10000000 >> j)) ? 0xFF : 0x00;*/
                    _buf[(ih - (y * font.ph + i)) * iw + x * font.pw + j] =
                        (font.bitmap[c][i] & (0b10000000 >> j)) ? 0xFF : 0x00;
                }
            }
            ++c;
        }
    }
    return gpu::createTexture((uint8_t *)_buf, iw, ih, static_cast<ChannelSetting>(ic),
                              GL_UNSIGNED_BYTE);
}

void gpu::freeTexture(gpu::Texture *texture) { TEXTURES.free(texture); }

gpu::Text *gpu::createText(const bdf::Font &font, const char *txt, bool center) {
    gpu::Text *text = TEXTS.acquire();
    text->node = NODES.acquire();
    if (font.gpuInstance == nullptr) {
        const_cast<bdf::Font &>(font).gpuInstance = createTexture(font);
    }
    gpu::Material *mat = MATERIALS.acquire();
    mat->textures.emplace(GL_TEXTURE0, (gpu::Texture *)font.gpuInstance);
    mat->color = Color::white;
    text->node->mesh = MESHES.acquire();
    text->node->visible = true;
    gpu::Primitive *primitive = PRIMITIVES.acquire();
    text->node->mesh->primitives.emplace_back(primitive, mat);
    primitive->vao = VAOS.acquire();
    primitive->vbos.emplace_back(VBOS.acquire());
    primitive->ebo = VBOS.acquire();
    text->bdfFont = &font;
    text->init();
    text->setText(txt, center);
    return text;
}

void gpu::freeText(gpu::Text *text) {
    text->setText("", false);
    text->bdfFont = nullptr;
    freeNode(text->node);
    text->node = nullptr;
}

gpu::Material *gpu::createMaterial() { return MATERIALS.acquire(); }

gpu::Material *gpu::createMaterial(const gltf::Material &material) {
    gpu::Material *mat = MATERIALS.acquire();
    bool noTex = true;
    for (size_t i{0}; i < gltf::Material::TEX_COUNT; ++i) {
        if (material.textures[i]) {
            mat->textures.emplace(GL_TEXTURE0 + i, createTexture(*material.textures[i]));
            noTex = false;
        }
        // Add white texture to empty ones for convenience. (to reduce n.o shaders)
        // Not minimalistic
        if (noTex) {
            mat->textures.emplace(GL_TEXTURE0,
                                  builtinMaterial(gpu::WHITE)->textures.at(GL_TEXTURE0));
        }
        mat->color = material.baseColor;
        mat->metallic = material.metallic;
        mat->roughness = material.roughness;
    }
    return mat;
}

void gpu::freeMaterial(gpu::Material *material) {
    for (const auto &it : material->textures) {
        bool onlyUserTexture{true};
        for (size_t i{0}; i < MATERIALS.count(); ++i) {
            if (material == &MATERIALS[i]) {
                continue;
            }
            for (const auto &it2 : MATERIALS[i].textures) {
                if (it.second == it2.second) {
                    onlyUserTexture = false;
                    break;
                }
            }
            if (!onlyUserTexture) {
                break;
            }
        }
        if (onlyUserTexture) {
            freeTexture(it.second);
        }
    }
    material->textures.clear();
    MATERIALS.free(material);
}

static gpu::Mesh *_createMesh(const gltf::Mesh &gltfMesh) {
    gpu::Mesh *mesh = MESHES.acquire();
    for (size_t i{0}; i < gltfMesh.primitives.size(); ++i) {
        const auto &gltfPrimitive = gltfMesh.primitives[i];
        auto &[primitive, material] = mesh->primitives.emplace_back(
            PRIMITIVES.acquire(),
            gltfPrimitive.material ? gpu::createMaterial(*gltfPrimitive.material) : &MATERIALS[0]);
        primitive->vao = VAOS.acquire();
        glBindVertexArray(*primitive->vao);
        for (size_t j{0}; j < gltf::Primitive::Attribute::COUNT; ++j) {
            const gltf::Accessor *gltfAttr = gltfPrimitive.attributes[j];
            if (gltfAttr == nullptr) {
                continue;
            }
            uint32_t vbo = *primitive->vbos.emplace_back(VBOS.acquire());
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, gltfAttr->bufferView->length,
                         gltfAttr->bufferView->buffer->data + gltfAttr->bufferView->offset,
                         GL_STATIC_DRAW);
            int size;
            if (gltfAttr->type == gltf::Accessor::MAT4) {
                assert(false);
            } else {
                size = gltfAttr->type + 1;
            }
            int baseSize = gltfAttr->componentType == GL_FLOAT ? sizeof(float) : 1;
            glVertexAttribPointer(j, size, gltfAttr->componentType, GL_FALSE, size * baseSize,
                                  (void *)0);
            glEnableVertexAttribArray(j);
        }
        primitive->ebo = VBOS.acquire();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *primitive->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, gltfPrimitive.indices->bufferView->length,
                     gltfPrimitive.indices->bufferView->buffer->data +
                         gltfPrimitive.indices->bufferView->offset,
                     GL_STATIC_DRAW);
        primitive->count = gltfPrimitive.indices->count;
    }
    mesh->gltfMesh = &gltfMesh;
    const_cast<gltf::Mesh *>(mesh->gltfMesh)->gpuInstance = mesh;
    return mesh;
}

gpu::Node *gpu::createNode() { return NODES.acquire(); }

gpu::Node *gpu::createNode(const gpu::Node &other) {
    gpu::Node *node = createNode();
    node->mesh = other.mesh;
    for (gpu::Node *child : other.children) {
        node->children.emplace_back(createNode(*child));
    }
    node->gltfNode = other.gltfNode;
    node->translation = other.translation.data();
    node->rotation = other.rotation.data();
    node->scale = other.scale.data();
    node->visible = other.visible;
    assert(other.skin == nullptr);
    return node;
}

gpu::Node *gpu::createNode(const gltf::Node &gltfNode) {
    Node *node = createNode();
    node->gltfNode = &gltfNode;
    const_cast<gltf::Node &>(gltfNode).gpuInstance = (void *)node;
    if (gltfNode.mesh) {
        gpu::Mesh *loadedMesh = (gpu::Mesh *)gltfNode.mesh->gpuInstance;
        if (loadedMesh) {
            node->mesh = loadedMesh;
            LOG_TRACE("Reusing mesh: %s", gltfNode.mesh->name.c_str());
        } else {
            node->mesh = _createMesh(*gltfNode.mesh);
            LOG_TRACE("Loading mesh: %s", gltfNode.mesh->name.c_str());
        }
    }
    glBindVertexArray(0);
    node->translation = gltfNode.translation.data();
    node->rotation = gltfNode.rotation.data();
    node->scale = gltfNode.scale.data();
    node->visible = true;
    for (gltf::Node *gltfChild : gltfNode.children) {
        node->children.emplace_back(createNode(*gltfChild))->setParent(node);
        if (gltfChild->skin) {
            node->skin = SKINS.acquire();
            node->skin->gltfSkin = gltfChild->skin;
        }
    }
    if (node->skin) {
        for (gltf::Node *joint : node->skin->gltfSkin->joints) {
            node->skin->joints.emplace_back(node->childByName(joint->name.c_str()));
        }
        const GLfloat *fp = (const GLfloat *)node->skin->gltfSkin->inverseBindMatrices->data();
        for (size_t j{0}; j < node->skin->gltfSkin->joints.size(); ++j) {
            node->skin->invBindMatrices.push_back(glm::make_mat4(fp + j * 16));
        }
    }
    return node;
};

static void freeMesh(gpu::Mesh *mesh) {
    for (auto &[primitive, material] : mesh->primitives) {

        VAOS.free(primitive->vao);
        for (uint32_t *vbo : primitive->vbos) {
            VBOS.free(vbo);
        }
        VBOS.free(primitive->ebo);
        primitive->vbos.clear();
        /*if (material) {
            bool onlyUserMaterial{true};
            for (size_t i{0}; i < PRIMITIVES.count(); ++i) {
                if (&PRIMITIVES[i] == primitive) {
                    continue;
                }
                if (PRIMITIVES[i].material == material) {
                    onlyUserMaterial = false;
                    break;
                }
            }
            if (onlyUserMaterial) {
                MATERIALS.free(material);
            }
            material = nullptr;
        }*/
    }
    const_cast<gltf::Mesh *>(mesh->gltfMesh)->gpuInstance = nullptr;
    mesh->primitives.clear();
    MESHES.free(mesh);
}

void gpu::freeNode(gpu::Node *node) {
    if (node->mesh) {
        bool onlyMeshUser = true;
        for (size_t i{0}; i < NODES.count(); ++i) {
            if (&NODES[i] == node) {
                continue;
            }
            if (NODES[i].mesh == node->mesh) {
                onlyMeshUser = false;
                break;
            }
        }
        if (onlyMeshUser) {
            freeMesh(node->mesh);
        }
    }
    node->mesh = nullptr;
    node->gltfNode = nullptr;
    if (node->skin) {
        node->skin->joints.clear();
        node->skin->gltfSkin = nullptr;
        SKINS.free(node->skin);
        node->skin = nullptr;
    }
    for (Node *child : node->children) {
        freeNode(child);
    }
    const_cast<gltf::Node *>(node->gltfNode)->gpuInstance = nullptr;
    node->children.clear();
    NODES.free(node);
}

gpu::Scene *gpu::createScene() {
    gpu::Scene *scene = SCENES.acquire();
    assert(scene->gltfScene == nullptr);
    return scene;
}

gpu::Scene *gpu::createScene(const gltf::Scene &gltfScene) {
    gpu::Scene *scene = createScene();
    for (gltf::Node *gltfNode : gltfScene.nodes) {
        scene->nodes.emplace_back(createNode(*gltfNode));
    }
    scene->gltfScene = &gltfScene;
    const_cast<gltf::Scene &>(gltfScene).gpuInstance = (void *)scene;
    return scene;
}

void gpu::freeScene(gpu::Scene *scene) {
    SCENES.free(scene);
    for (gpu::Node *node : scene->nodes) {
        if (node) {
            freeNode(node);
        }
    }
    scene->nodes.clear();
    const_cast<gltf::Scene *>(scene->gltfScene)->gpuInstance = nullptr;
    scene->gltfScene = nullptr;
}

static void invalidateRecursive(gpu::Node *node) {
    node->invalidate();
    for (gpu::Node *child : node->children) {
        invalidateRecursive(child);
    }
}

void gpu::bindMaterial(gpu::ShaderProgram *shaderProgram, gpu::Material *material) {
    if (auto color = shaderProgram->uniform("u_color")) {
        *color << material->color.vec4();
    }
    if (auto metallic = shaderProgram->uniform("u_metallic")) {
        *metallic << material->metallic;
    }
    if (auto metallic = shaderProgram->uniform("u_roughness")) {
        *metallic << material->metallic;
    }
    for (const auto &it : material->textures) {
        // printf("%s: %u\n", this->gltfNode->name.c_str(), *it.second);
        glActiveTexture(it.first);
        glBindTexture(GL_TEXTURE_2D, it.second->id);
    }
}

void gpu::Primitive::render(ShaderProgram *shaderProgram) {
    glBindVertexArray(*vao);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, NULL);
    glBindVertexArray(0);
}

void gpu::Node::render(ShaderProgram *shaderProgram) {
    if (parent() == nullptr && !valid()) {
        invalidateRecursive(this);
    }
    if (skin) {
        static UniformBuffer *skinUBO = nullptr;
        static const int MAX_BONES = 32;
        static glm::mat4 bones[MAX_BONES] = {};

        if (skinUBO == nullptr) {
            skinUBO = createUniformBuffer(1, sizeof(bones), bones);
        }
        static std::unordered_set<uint32_t> skinningShaders;
        auto it = skinningShaders.find(shaderProgram->id);
        if (it == skinningShaders.end()) {
            skinUBO->bindBlock(shaderProgram, "SkinBlock");
            skinUBO->bindBufferBase();
            skinningShaders.emplace(shaderProgram->id);
        }
        const glm::mat4 globalWorldInverse = glm::inverse(model());
        for (size_t j{0}; j < skin->joints.size() && j < MAX_BONES; ++j) {
            bones[j] =
                globalWorldInverse * skin->joints.at(j)->model() * skin->invBindMatrices.at(j);
        }
        skinUBO->bind();
        skinUBO->bufferData(sizeof(bones), bones);
        skinUBO->bufferSubData(64 * 30, sizeof(bones) - 64 * 30, bones + 30);
    }
    for (gpu::Node *child : children) {
        child->render(shaderProgram);
    }
    if (visible && mesh && !mesh->primitives.empty()) {
        shaderProgram->uniforms.at("u_model") << model();
        for (auto &[primitive, material] : mesh->primitives) {
            bindMaterial(shaderProgram, _overrideMaterial ? _overrideMaterial : material);
            primitive->render(shaderProgram);
        }
    }
}

gpu::Node *gpu::Node::childByName(const char *name) {
    if (strcmp(this->gltfNode->name.c_str(), name) == 0) {
        return this;
    }
    for (Node *child : children) {
        Node *rval = child->childByName(name);
        if (rval) {
            return rval;
        }
    }
    return nullptr;
}
const std::string &gpu::Node::name() const {
    static const std::string noname{"noname"};
    return gltfNode ? gltfNode->name : noname;
}
const std::string &gpu::Node::meshName() const {
    static const std::string noname{"noname"};
    return (mesh && mesh->gltfMesh) ? mesh->gltfMesh->name : noname;
}

gpu::Primitive *gpu::Node::primitive(size_t primitiveIndex) {
    if (mesh && primitiveIndex < mesh->primitives.size()) {
        return mesh->primitives.at(primitiveIndex).first;
    }
    return nullptr;
}

gpu::Material *gpu::Node::material(size_t primitiveIndex) {
    if (mesh && primitiveIndex < mesh->primitives.size()) {
        return mesh->primitives.at(primitiveIndex).second;
    }
    return nullptr;
}

gpu::Node *gpu::Scene::nodeByName(const char *name) {
    for (Node *node : nodes) {
        if (node->gltfNode->name.compare(name) == 0) {
            return node;
        }
    }
    return nullptr;
}

gpu::UniformBuffer *gpu::createUniformBuffer(uint32_t bindingPoint, uint32_t length, void *data) {
    gpu::UniformBuffer *ubo = UBOS.acquire();
    ubo->id = VBOS.acquire();
    ubo->length = length;
    ubo->data = nullptr;
    ubo->length = 0;
    ubo->bindingPoint = bindingPoint;
    ubo->bind();
    ubo->bufferData(length, data);
    ubo->unbind();
    return ubo;
}

void gpu::freeUniformBuffer(gpu::UniformBuffer *ubo) {
    ubo->bind();
    glBufferData(GL_UNIFORM_BUFFER, 0, NULL, GL_STATIC_DRAW);
    ubo->unbind();
    VBOS.free(ubo->id);
    ubo->bindingPoint = 0;
    ubo->data = nullptr;
    ubo->length = 0;
    UBOS.free(ubo);
}

void gpu::UniformBuffer::bind() { glBindBuffer(GL_UNIFORM_BUFFER, *id); }

void gpu::UniformBuffer::unbind() { glBindBuffer(GL_UNIFORM_BUFFER, 0); }

void gpu::UniformBuffer::bindBlock(gpu::ShaderProgram *shader, const char *blockLabel) {
    auto index = glGetUniformBlockIndex(shader->id, blockLabel);
    glUniformBlockBinding(shader->id, index, this->bindingPoint);
}

void gpu::UniformBuffer::bindBufferBase() {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, *id);
}

void gpu::UniformBuffer::bufferData(uint32_t length, void *data) {
    glBufferData(GL_UNIFORM_BUFFER, length, data, GL_STATIC_DRAW);
}

void gpu::UniformBuffer::bufferSubData(uint32_t offset, uint32_t length, void *data) {
    glBufferSubData(GL_UNIFORM_BUFFER, offset, length, data);
}

gpu::Shader *gpu::createShader(uint32_t type, const char *src) {
    gpu::Shader *shader = SHADERS.acquire();
    shader->id = glCreateShader(type);
    if (!Shader_compile(type, *shader, src)) {
        glDeleteShader(shader->id);
        SHADERS.free(shader);
        return nullptr;
    }
    return shader;
}

void gpu::freeShader(gpu::Shader *shader) { SHADERS.free(shader); }

gpu::ShaderProgram *gpu::createShaderProgram(gpu::Shader *vertex, gpu::Shader *fragment,
                                             std::unordered_map<std::string, Uniform> &&uniforms) {
    ShaderProgram *prog = SHADERPROGRAMS.acquire();
    prog->vertex = vertex;
    prog->fragment = fragment;
    prog->uniforms = std::move(uniforms);
    Shader_createProgram(*prog);
    return prog;
}

void gpu::freeShaderProgram(ShaderProgram *shaderProgram) {
    bool onlyUserVertex{true};
    bool onlyUserFragment{true};
    for (size_t i{0}; i < SHADERPROGRAMS.count(); ++i) {
        if (&SHADERPROGRAMS[i] == shaderProgram) {
            continue;
        }
        if (SHADERPROGRAMS[i].vertex == shaderProgram->vertex) {
            onlyUserVertex = false;
        }
        if (SHADERPROGRAMS[i].fragment == shaderProgram->fragment) {
            onlyUserFragment = false;
        }
    }
    if (onlyUserVertex) {
        freeShader(shaderProgram->vertex);
    }
    if (onlyUserFragment) {
        freeShader(shaderProgram->fragment);
    }
    shaderProgram->vertex = nullptr;
    shaderProgram->fragment = nullptr;
    SHADERPROGRAMS.free(shaderProgram);
}