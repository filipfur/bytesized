#include "gpu.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "debug.hpp"
#include "gpu_primitive.h"
#include "logging.h"
#include "opengl.h"
#include "primer.h"
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
#define GPU__ANIMATION__COUNT 50
#define GPU___PLAYBACK_COUNT 10

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
static recycler<gpu::Animation, GPU__ANIMATION__COUNT> ANIMATIONS = {};
static recycler<gpu::Playback, GPU___PLAYBACK_COUNT> PLAYBACKS = {};
bool gpu::SETTINGS_LERP{true};

static gpu::Material *_builtinMaterials[gpu::MATERIAL_COUNT];
static gpu::Material *_overrideMaterial{nullptr};
static gpu::Texture *_blankDiffuse{nullptr};

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

    uint8_t onePixel[]{0xFF, 0xFF, 0xFF};
    _blankDiffuse = createTexture(onePixel, 1, 1, ChannelSetting::RGB, GL_UNSIGNED_BYTE);

    _builtinMaterials[BuiltinMaterial::CHECKERS] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::CHECKERS]->color = 0xFF00FF;
    constexpr StaticTexture<8, 8, 3> checkers_tex{[](uint32_t x, uint32_t y) {
        return glm::vec4(glm::vec3(0.75f + ((y % 2 + x) % 2) * 0.25f), 1.0f);
    }};
    _builtinMaterials[BuiltinMaterial::CHECKERS]->textures.emplace(
        GL_TEXTURE0,
        createTexture(checkers_tex.buf, checkers_tex.width, checkers_tex.height,
                      static_cast<ChannelSetting>(checkers_tex.channels), GL_UNSIGNED_BYTE));

    // auto gridColor = rgb(1, 0, 0); // 0x44AAFF
    _builtinMaterials[BuiltinMaterial::GRID_TILE] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::GRID_TILE]->color = rgb(60, 178, 237);
    constexpr size_t quad_width = 64;
    constexpr size_t quad_height = 64;
    constexpr StaticTexture<quad_width, quad_height, 4> quadratic_tex{[](uint32_t x, uint32_t y) {
        // glm::vec2 st = glm::vec2(x, y) / glm::vec2(8, 8);
        // st -= 0.5f;
        bool paint = (x <= 1 || x == (quad_width / 2 + 1) || y <= 1 || y == (quad_width / 2 + 1));
        return glm::vec4(paint ? 1.0f : 0.24f);
    }};
    _builtinMaterials[BuiltinMaterial::GRID_TILE]->textures.emplace(
        GL_TEXTURE0,
        createTexture(quadratic_tex.buf, quadratic_tex.width, quadratic_tex.height,
                      static_cast<ChannelSetting>(quadratic_tex.channels), GL_UNSIGNED_BYTE));

    _builtinMaterials[BuiltinMaterial::WHITE] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::WHITE]->color = 0xFFFFFF;
    _builtinMaterials[BuiltinMaterial::WHITE]->textures.emplace(GL_TEXTURE0, _blankDiffuse);
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

gpu::Mesh *gpu::createMesh(gpu::Primitive *primitive, gpu::Material *material) {
    auto mesh = createMesh();
    mesh->primitives.emplace_back(primitive, material ? material : builtinMaterial(CHECKERS));
    return mesh;
}

gpu::Primitive *gpu::createPrimitive(const glm::vec3 *positions, const glm::vec3 *normals,
                                     const glm::vec2 *uvs, size_t vertex_count,
                                     const uint16_t *indices, size_t index_count) {
    gpu::Primitive *prim = PRIMITIVES.acquire();
    prim->vao = VAOS.acquire();
    glBindVertexArray(*prim->vao);

    uint32_t *vbo = prim->vbos.emplace_back(VBOS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    vbo = prim->vbos.emplace_back(VBOS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    vbo = prim->vbos.emplace_back(VBOS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec2), uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(2);

    prim->ebo = VBOS.acquire();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *prim->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STATIC_DRAW);
    prim->count = index_count;
    return prim;
}
gpu::Primitive *gpu::createPrimitive(const VertexObject &vertexObject) {
    return createPrimitive(vertexObject.positions.data(), vertexObject.normals.data(),
                           vertexObject.uvs.data(), vertexObject.positions.size(),
                           vertexObject.indices.data(), vertexObject.indices.size());
}

gpu::Material *gpu::builtinMaterial(BuiltinMaterial builtinMaterial) {
    return _builtinMaterials[builtinMaterial];
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

gpu::Texture *gpu::createTextureFromFile(const char *path, bool flip) {
    int iw, ih, ic;
    stbi_set_flip_vertically_on_load(flip);
    const uint8_t *idata = stbi_load(path, &iw, &ih, &ic, 0);
    return createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
}

gpu::Texture *gpu::createTextureFromMem(const uint8_t *addr, uint32_t len, bool flip) {
    int iw, ih, ic;
    stbi_set_flip_vertically_on_load(flip);
    const uint8_t *idata = stbi_load_from_memory((uint8_t *)addr, len, &iw, &ih, &ic, 0);
    return createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
}
gpu::Texture *gpu::createTexture(const library::Texture &texture) {
    return createTextureFromMem((uint8_t *)texture.image->view->data(), texture.image->view->length,
                                false);
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
    text->node->hidden = false;
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

gpu::Material *gpu::createMaterial(const Color &color) {
    gpu::Material *mat = MATERIALS.acquire();
    mat->color = color;
    mat->textures.emplace(GL_TEXTURE0, _blankDiffuse);
    return mat;
}

gpu::Material *gpu::createMaterial(gpu::Texture *texture) {
    gpu::Material *mat = MATERIALS.acquire();
    mat->color = Color::white;
    mat->textures.emplace(GL_TEXTURE0, texture);
    return mat;
}

gpu::Material *gpu::createMaterial(const library::Material &material) {
    gpu::Material *mat = MATERIALS.acquire();
    bool noTex = true;
    for (size_t i{0}; i < library::Material::TEX_COUNT; ++i) {
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

static gpu::Mesh *_createMesh(const library::Mesh &libraryMesh) {
    gpu::Mesh *mesh = MESHES.acquire();
    for (size_t i{0}; i < libraryMesh.primitives.size(); ++i) {
        const auto &libraryPrimitive = libraryMesh.primitives[i];
        auto &[primitive, material] = mesh->primitives.emplace_back(
            PRIMITIVES.acquire(), libraryPrimitive.material
                                      ? gpu::createMaterial(*libraryPrimitive.material)
                                      : &MATERIALS[0]);
        const_cast<library::Primitive &>(libraryPrimitive).gpuInstance = primitive;
        primitive->vao = VAOS.acquire();
        glBindVertexArray(*primitive->vao);
        for (size_t j{0}; j < library::Primitive::Attribute::COUNT; ++j) {
            const library::Accessor *libraryAttr = libraryPrimitive.attributes[j];
            if (libraryAttr == nullptr) {
                continue;
            }
            uint32_t vbo = *primitive->vbos.emplace_back(VBOS.acquire());
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, libraryAttr->bufferView->length,
                         libraryAttr->bufferView->data(), GL_STATIC_DRAW);
            int size = libraryAttr->type + 1;
            assert(libraryAttr->type != library::Accessor::MAT4);
            int stride;
            if (libraryAttr->componentType == GL_FLOAT) {
                stride = size * sizeof(float);
            } else {
                stride = size;
            }
            glVertexAttribPointer(j, size, libraryAttr->componentType, GL_FALSE, stride, (void *)0);
            glEnableVertexAttribArray(j);
        }
        if (libraryPrimitive.indices) {
            primitive->ebo = VBOS.acquire();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *primitive->ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, libraryPrimitive.indices->bufferView->length,
                         libraryPrimitive.indices->bufferView->buffer->data +
                             libraryPrimitive.indices->bufferView->offset,
                         GL_STATIC_DRAW);
            primitive->count = libraryPrimitive.indices->count;
        } else {
            primitive->count = libraryPrimitive.attributes[0]->count;
        }
    }
    mesh->libraryMesh = &libraryMesh;
    const_cast<library::Mesh *>(mesh->libraryMesh)->gpuInstance = mesh;
    return mesh;
}

gpu::Node *gpu::createNode() { return NODES.acquire(); }

gpu::Node *gpu::createNode(gpu::Mesh *mesh) {
    auto node = createNode();
    node->mesh = mesh;
    return node;
}

gpu::Node *gpu::createNode(const gpu::Node &other) {
    gpu::Node *node = createNode();
    node->mesh = other.mesh;
    for (gpu::Node *child : other.children) {
        node->children.emplace_back(createNode(*child));
    }
    node->libraryNode = other.libraryNode;
    node->translation = other.translation.data();
    node->rotation = other.rotation.data();
    node->scale = other.scale.data();
    node->hidden = other.hidden;
    assert(other.skin == nullptr);
    return node;
}

gpu::Node *gpu::createNode(const library::Node &libraryNode) {
    Node *node = createNode();
    node->libraryNode = &libraryNode;
    if (libraryNode.gpuInstance == nullptr) {
        const_cast<library::Node &>(libraryNode).gpuInstance = (void *)node;
    }
    if (libraryNode.mesh) {
        gpu::Mesh *loadedMesh = (gpu::Mesh *)libraryNode.mesh->gpuInstance;
        if (loadedMesh) {
            node->mesh = loadedMesh;
            LOG_TRACE("Reusing mesh: %s", libraryNode.mesh->name.c_str());
        } else {
            node->mesh = _createMesh(*libraryNode.mesh);
            LOG_TRACE("Loading mesh: %s", libraryNode.mesh->name.c_str());
        }
    }
    glBindVertexArray(0);
    node->translation = libraryNode.translation.data();
    node->rotation = libraryNode.rotation.data();
    node->scale = libraryNode.scale.data();
    node->hidden = false;
    for (library::Node *libraryChild : libraryNode.children) {
        node->children.emplace_back(createNode(*libraryChild))->setParent(node);
        if (libraryChild->skin) {
            node->skin = SKINS.acquire();
            node->skin->librarySkin = libraryChild->skin;
        }
    }
    if (node->skin) {
        for (library::Node *joint : node->skin->librarySkin->joints) {
            node->skin->joints.emplace_back(node->childByName(joint->name.c_str()));
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
    const_cast<library::Mesh *>(mesh->libraryMesh)->gpuInstance = nullptr;
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
    node->entity = nullptr;
    node->hidden = false;
    node->translation = {0.0f, 0.0f, 0.0f};
    node->rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    node->euler = {0.0f, 0.0f, 0.0f};
    node->scale = {1.0f, 1.0f, 1.0f};
    node->wireframe = false;
    node->mesh = nullptr;
    node->setParent(nullptr);
    if (node->libraryNode->gpuInstance == node) {
        const_cast<library::Node *>(node->libraryNode)->gpuInstance = nullptr;
    }
    node->libraryNode = nullptr;
    if (node->skin) {
        for (auto *anim : node->skin->animations) {
            gpu::freeAnimation(anim);
        }
        node->skin->playback->animation = nullptr;
        gpu::freePlayback(node->skin->playback);
        node->skin->animations.clear();
        node->skin->playback = nullptr;

        node->skin->joints.clear();
        node->skin->librarySkin = nullptr;
        SKINS.free(node->skin);
        node->skin = nullptr;
    }
    for (Node *child : node->children) {
        freeNode(child);
    }
    node->children.clear();
    NODES.free(node);
}

size_t gpu::nodeCount() { return NODES.count(); }

gpu::Scene *gpu::createScene() {
    gpu::Scene *scene = SCENES.acquire();
    assert(scene->libraryScene == nullptr);
    return scene;
}

gpu::Scene *gpu::createScene(const library::Scene &libraryScene) {
    gpu::Scene *scene = createScene();
    for (library::Node *libraryNode : libraryScene.nodes) {
        scene->nodes.emplace_back(createNode(*libraryNode));
    }
    scene->libraryScene = &libraryScene;
    const_cast<library::Scene &>(libraryScene).gpuInstance = (void *)scene;
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
    const_cast<library::Scene *>(scene->libraryScene)->gpuInstance = nullptr;
    scene->libraryScene = nullptr;
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
        // printf("%s: %u\n", this->libraryNode->name.c_str(), *it.second);
        glActiveTexture(it.first);
        glBindTexture(GL_TEXTURE_2D, it.second->id);
    }
}

void gpu::Primitive::render() {
    glBindVertexArray(*vao);
    if (ebo) {
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, NULL);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, count);
    }
    glBindVertexArray(0);
}

void gpu::UniformBuffer::bindShaders(std::initializer_list<gpu::ShaderProgram *> shaders) {
    bind();
    for (auto &shader : shaders) {
        bindBlock(shader, label);
    }
    bindBufferBase();
    unbind();
}

static gpu::CameraBlock cameraBlock;

void gpu::CameraBlock_setProjection(const glm::mat4 &projection) {
    cameraBlock.projection = projection;
    auto *ubo = builtinUBO(gpu::UBO_CAMERA);
    ubo->bind();
    ubo->bufferSubData(0, sizeof(gpu::CameraBlock::projection), &cameraBlock);
}
void gpu::CameraBlock_setViewPos(const glm::mat4 &view, const glm::vec3 &pos) {
    cameraBlock.view = view;
    cameraBlock.cameraPos = pos;
    auto *ubo = builtinUBO(gpu::UBO_CAMERA);
    ubo->bind();
    ubo->bufferSubData(sizeof(glm::mat4), sizeof(glm::mat4) + sizeof(glm::vec3), &cameraBlock.view);
}

static gpu::SkinBlock skinBlock;
gpu::SkinBlock &gpu::getSkinBlock() { return skinBlock; }

static gpu::LightBlock lightBlock;
void gpu::LightBlock_setLightColor(const Color &high, const Color &low, const Color &ambient) {
    lightBlock.lightHigh = high.vec4();
    lightBlock.lightLow = low.vec4();
    lightBlock.ambient = ambient.vec4();
    auto *ubo = builtinUBO(gpu::UBO_LIGHT);
    ubo->bind();
    ubo->bufferData(sizeof(LightBlock), &lightBlock);
}

gpu::UniformBuffer *gpu::builtinUBO(BuiltinUBO bultinUBO) { return &UBOS.at(bultinUBO); }
void gpu::createBuiltinUBOs() {
    static const char *cameraBlockLabel = "CameraBlock";
    static const char *skinBlockLabel = "SkinBlock";
    // static const char *guiBlockLabel = "GUIBlock";
    createUniformBuffer(UBO_CAMERA, cameraBlockLabel, sizeof(gpu::CameraBlock), &cameraBlock);
    createUniformBuffer(UBO_SKINNING, skinBlockLabel, sizeof(gpu::SkinBlock), skinBlock.bones);
    createUniformBuffer(UBO_LIGHT, "LightBlock", sizeof(gpu::LightBlock), &lightBlock);
    // createUniformBuffer(UBO_GUI, guiBlockLabel, sizeof(gpu::SkinBlock), &cameraBlock);
}

static void _updateBonesArray(gpu::Node *node) {
    const glm::mat4 globalWorldInverse = glm::inverse(node->model());
    for (size_t j{0}; j < node->skin->joints.size() && j < gpu::MAX_BONES; ++j) {
        skinBlock.bones[j] = globalWorldInverse * node->skin->joints.at(j)->model() *
                             node->skin->librarySkin->inverseBindMatrices.at(j);
    }
}

void gpu::Node::render(ShaderProgram *shaderProgram) {
    if (parent() == nullptr && !valid()) {
        invalidateRecursive(this);
    }
    if (skin) {
        gpu::UniformBuffer *ubo = gpu::builtinUBO(gpu::UBO_SKINNING);
        _updateBonesArray(this);
        ubo->bind();
        ubo->bufferData(sizeof(skinBlock), &skinBlock);
    }
    for (gpu::Node *child : children) {
        child->render(shaderProgram);
    }
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    if (!hidden && mesh && !mesh->primitives.empty()) {
        shaderProgram->uniforms.at("u_model") << model();
        for (auto &[primitive, material] : mesh->primitives) {
            bindMaterial(shaderProgram, _overrideMaterial ? _overrideMaterial : material);
            primitive->render();
        }
    }
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

gpu::Node *gpu::Node::childByName(const char *name) {
    if (strcmp(this->libraryNode->name.c_str(), name) == 0) {
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
    return libraryNode ? libraryNode->name : noname;
}
const std::string &gpu::Node::meshName() const {
    static const std::string noname{"noname"};
    return (mesh && mesh->libraryMesh) ? mesh->libraryMesh->name : noname;
}

void gpu::Node::addChild(gpu::Node *node) { children.emplace_back(node)->setParent(this); }

void gpu::Node::recursive(const std::function<void(gpu::Node *)> &callback) {
    callback(this);
    for (gpu::Node *child : children) {
        child->recursive(callback);
    }
}

void gpu::Node::forEachChild(const std::function<void(gpu::Node *)> &callback) {
    for (gpu::Node *child : children) {
        callback(child);
    }
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
        if (node->libraryNode->name.compare(name) == 0) {
            return node;
        }
    }
    return nullptr;
}

gpu::UniformBuffer *gpu::createUniformBuffer(uint32_t bindingPoint, const char *label,
                                             uint32_t length, void *data) {
    gpu::UniformBuffer *ubo = UBOS.acquire();
    ubo->id = VBOS.acquire();
    ubo->bindingPoint = bindingPoint;
    ubo->label = label;
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

void gpu::UniformBuffer::bufferData(uint32_t length_, void *data_) {
    glBufferData(GL_UNIFORM_BUFFER, length_, data_, GL_DYNAMIC_DRAW);
}

void gpu::UniformBuffer::bufferSubData(uint32_t offset, uint32_t length, void *data) {
    glBufferSubData(GL_UNIFORM_BUFFER, offset, length, data);
}

gpu::Shader *gpu::createShader(uint32_t type, const char *src) {
    gpu::Shader *shader = SHADERS.acquire();
    shader->id = glCreateShader(type);
    if (!Shader_compile(*shader, src)) {
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

#if 0
    prim->vao = VAOS.acquire();
    glBindVertexArray(*prim->vao);
    uint32_t *vbo = prim->vbos.emplace_back(VBOS.acquire());
    prim->ebo = VBOS.acquire();
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *prim->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STATIC_DRAW);
    prim->count = index_count;
#endif

void gpu::renderScreen() {
    static uint32_t *vao{nullptr};
    static uint32_t *vbo{nullptr};
    static uint32_t *ebo{nullptr};
    if (vao == nullptr) {
        vao = VAOS.acquire();
        glBindVertexArray(*vao);
        vbo = VBOS.acquire();
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(gpu::screen_vertices), gpu::screen_vertices,
                     GL_STATIC_DRAW);
        ebo = VBOS.acquire();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gpu::screen_indices), gpu::screen_indices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    }
    glBindVertexArray(*vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
}

inline static void _setupAttribPosNorUV_Interleaved() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void gpu::renderVector(gpu::ShaderProgram *shaderProgram, Vector &vector) {
    if (vector.hidden) {
        return;
    }
    static uint32_t *vao{nullptr};
    static uint32_t *vbo{nullptr};
    static uint32_t *ebo{nullptr};
    shaderProgram->use();
    const glm::mat4 model =
        glm::scale(glm::translate(glm::mat4(1.0f), vector.O) *
                       glm::mat4(glm::mat3_cast(primer::quaternionFromDirection(vector.Ray))),
                   glm::vec3{1.0f, glm::length(vector.Ray), 1.0f});
    glActiveTexture(GL_TEXTURE0);
    builtinMaterial(gpu::WHITE)->textures.at(GL_TEXTURE0)->bind();
    shaderProgram->uniforms.at("u_model") << model;
    shaderProgram->uniforms.at("u_color") << vector.color.vec4();
    if (vao == nullptr) {
        vao = VAOS.acquire();
        glBindVertexArray(*vao);
        vbo = VBOS.acquire();
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(gpu::vector_vertices), gpu::vector_vertices,
                     GL_STATIC_DRAW);
        ebo = VBOS.acquire();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gpu::vector_indices), gpu::vector_indices,
                     GL_STATIC_DRAW);
        _setupAttribPosNorUV_Interleaved();
    }
    glBindVertexArray(*vao);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, NULL);
}

gpu::Animation *gpu::Skin::findAnimation(const char *name) {
    for (auto anim : animations) {
        if (anim->name.compare(name) == 0) {
            return anim;
        }
    }
    return nullptr;
}

gpu::Playback *gpu::Skin::playAnimation(const char *name) {
    gpu::Animation *anim = findAnimation(name);
    if (anim != playback->animation) {
        playback->animation = anim;
        playback->time = playback->animation->startTime;
        for (auto &channel : anim->channels) {
            gpu::Channel *ch = &channel.second.at(gpu::Animation::CH_TRANSLATION);
            ch->current = ch->frames.begin();
            ch = &channel.second.at(gpu::Animation::CH_ROTATION);
            ch->current = ch->frames.begin();
            ch = &channel.second.at(gpu::Animation::CH_SCALE);
            ch->current = ch->frames.begin();
        }
    }
    return playback;
}

gpu::Animation *gpu::createAnimation(const library::Animation &anim, gpu::Node *retargetNode) {
    auto rval = ANIMATIONS.acquire();
    rval->libraryAnimation = &anim;
    rval->startTime = FLT_MAX;
    rval->endTime = -FLT_MAX;
    rval->looping = true;
    for (size_t j{0}; j < 96; ++j) {
        const auto &channel = anim.channels[j];
        if (channel.type) {
            float *fp = (float *)channel.sampler->input->data();
            glm::vec3 *v3p = (glm::vec3 *)channel.sampler->output->data();
            glm::vec4 *v4p = (glm::vec4 *)channel.sampler->output->data();
            gpu::Node *targetNode = (gpu::Node *)channel.targetNode->gpuInstance;
            if (retargetNode) {
                targetNode = retargetNode->childByName(channel.targetNode->name.c_str());
                assert(targetNode);
            }
            rval->name = anim.name;
            switch (channel.type) {
            case library::Channel::TRANSLATION:
                rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].current =
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).time =
                        fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.x =
                        v3p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.y =
                        v3p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.z =
                        v3p[k].z;
                }
                break;
            case library::Channel::ROTATION:
                rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_ROTATION].current =
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).time =
                        fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.x =
                        v4p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.y =
                        v4p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.z =
                        v4p[k].z;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.w =
                        v4p[k].w;
                }
                break;
            case library::Channel::SCALE:
                rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_SCALE].current =
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).time = fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.x =
                        v3p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.y =
                        v3p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.z =
                        v3p[k].z;
                }
                break;
            case library::Channel::NONE:
                LOG_ERROR("Unknown animation channel");
                break;
            }
        }
    }
    return rval;
}

void gpu::freeAnimation(Animation *animation) {
    animation->startTime = 0;
    animation->endTime = 0;
    animation->name = {};
    animation->channels.clear();
    animation->libraryAnimation = nullptr;
    animation->looping = true;
    ANIMATIONS.free(animation);
}

void gpu::Animation::start() {
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        if (PLAYBACKS[i].animation == this) {
            return;
        }
    }
    auto playback = PLAYBACKS.acquire();
    playback->animation = this;
    playback->paused = false;
    playback->time = 0.0f;
}

void gpu::Animation::stop() {
    gpu::Playback *playback{nullptr};
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        if (PLAYBACKS[i].animation == this) {
            playback = PLAYBACKS.data() + 1;
            break;
        }
    }
    if (playback) {
        playback->animation = nullptr;
        PLAYBACKS.free(playback);
    }
}

gpu::Playback *gpu::createPlayback(gpu::Animation *animation) {
    gpu::Playback *handle = PLAYBACKS.acquire();
    handle->animation = animation;
    return handle;
};

void gpu::freePlayback(gpu::Playback *playback) {
    playback->animation = nullptr;
    playback->paused = false;
    playback->time = 0.0f;
    PLAYBACKS.free(playback);
};

static inline gpu::Frame *getFrame(gpu::Channel &channel, float time);

void gpu::animate(float dt) {
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        auto &playback = PLAYBACKS[i];
        if (auto anim = playback.animation) {
            if (playback.paused) {
                continue;
            }
            for (auto &channel : anim->channels) {
                gpu::Node *node = channel.first;
                if (auto tframe = getFrame(channel.second.at(gpu::Animation::CH_TRANSLATION),
                                           playback.time)) {
                    if (SETTINGS_LERP) {
                        node->translation =
                            glm::mix(node->translation.data(), tframe->v3, 16.0f * dt);
                    } else {
                        node->translation = tframe->v3;
                    }
                }
                if (auto rframe =
                        getFrame(channel.second.at(gpu::Animation::CH_ROTATION), playback.time)) {
                    if (SETTINGS_LERP) {
                        node->rotation = glm::slerp(node->rotation.data(), rframe->q, 16.0f * dt);
                    } else {
                        node->rotation = rframe->q;
                    }
                }
                if (auto sframe =
                        getFrame(channel.second.at(gpu::Animation::CH_SCALE), playback.time)) {
                    if (SETTINGS_LERP) {
                        node->scale = glm::mix(node->scale.data(), sframe->v3, 16.0f * dt);
                    } else {
                        node->scale = sframe->v3;
                    }
                }
            }
            playback.time += dt;
            if (playback.time > playback.animation->endTime) {
                if (playback.animation->looping) {
                    playback.time = playback.animation->startTime +
                                    (playback.time - playback.animation->endTime);
                } else {
                    playback.time = std::min(playback.time, playback.animation->endTime);
                }
            }
        }
    }
}

static inline gpu::Frame *getFrame(gpu::Channel &channel, float time) {
    if (channel.current == channel.frames.end()) {
        if (channel.frames.empty()) {
            return nullptr;
        }
        channel.current = channel.frames.begin();
    }
    while (channel.current->time < time) {
        if (channel.current == (channel.frames.end() - 1)) {
            break;
        }
        ++channel.current;
    }
    if (channel.current == (channel.frames.end() - 1)) {
        float d1 = time - channel.frames.begin()->time;
        float d2 = time - channel.current->time;
        // check if we wrapped
        if (glm::abs(d1) < glm::abs(d2)) {
            channel.current = channel.frames.begin();
        }
    }
    if (channel.current != channel.frames.end()) {
        return &(*channel.current);
    }
    return nullptr;
}