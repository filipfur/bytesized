#include "gpu.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "logging.h"
#include "opengl.h"
#include "primer.h"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>

static recycler<gpu::VertexArray, BYTESIZED_VERTEXARRAY_COUNT> VERTEXARRAYS = {};
static recycler<uint32_t, BYTESIZED_VERTEXBUFFER_COUNT> VERTEXBUFFERS = {};
static recycler<gpu::Texture, BYTESIZED_TEXTURE_COUNT> TEXTURES = {};
static recycler<gpu::Material, BYTESIZED_MATERIAL_COUNT> MATERIALS = {};
static recycler<gpu::UniformBuffer, BYTESIZED_UNIFORMBUFFER_COUNT> UBOS = {};
static recycler<gpu::Mesh, BYTESIZED_MESH_COUNT> MESHES = {};
static recycler<gpu::Primitive, BYTESIZED_PRIMITIVE_COUNT> PRIMITIVES = {};
static recycler<gpu::Node, BYTESIZED_NODE_COUNT> NODES = {};
static recycler<gpu::Scene, BYTESIZED_SCENE_COUNT> SCENES = {};
static recycler<gpu::Shader, BYTESIZED_SHADER_COUNT> SHADERS = {};
static recycler<gpu::ShaderProgram, BYTESIZED_SHADERPROGRAM_COUNT> SHADERPROGRAMS = {};
static recycler<gpu::Text, BYTESIZED_TEXT_COUNT> TEXTS = {};
static recycler<gpu::Framebuffer, BYTESIZED_FRAMEBUFFER_COUNT> FRAMEBUFFERS = {};

#define PRINT_USAGE(var)                                                                           \
    do {                                                                                           \
        printf("%s: %zu / %zu\n", #var, var.count(), var.size());                                  \
    } while (0);

void gpu::printAllocations() {
    printf("\nGPU allocations:\n");
    PRINT_USAGE(VERTEXARRAYS);
    PRINT_USAGE(VERTEXBUFFERS);
    PRINT_USAGE(TEXTURES);
    PRINT_USAGE(MATERIALS);
    PRINT_USAGE(UBOS);
    PRINT_USAGE(MESHES);
    PRINT_USAGE(PRIMITIVES);
    PRINT_USAGE(NODES);
    PRINT_USAGE(SCENES);
    PRINT_USAGE(SHADERS);
    PRINT_USAGE(SHADERPROGRAMS);
    PRINT_USAGE(TEXTS);
    PRINT_USAGE(FRAMEBUFFERS);
#ifdef BYTESIZED_USE_SKINNING
    gpu::printSkinningUsages();
#endif
    printf("-------------------------\n\n");
}

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
    glGenVertexArrays(BYTESIZED_VERTEXARRAY_COUNT, (uint32_t *)VERTEXARRAYS.data());
    glGenBuffers(BYTESIZED_VERTEXBUFFER_COUNT, VERTEXBUFFERS.data());
    glGenTextures(BYTESIZED_TEXTURE_COUNT, (GLuint *)TEXTURES.data());
    size_t allocatedBytes =
        (sizeof(VERTEXARRAYS) + sizeof(VERTEXBUFFERS) + sizeof(TEXTURES) + sizeof(MATERIALS) +
         sizeof(UBOS) + sizeof(MESHES) + sizeof(PRIMITIVES) + sizeof(NODES) + sizeof(SCENES) +
         sizeof(SHADERS) + sizeof(SHADERPROGRAMS) + sizeof(TEXTS) + sizeof(FRAMEBUFFERS));
#ifdef BYTESIZED_USE_SKINNING
    allocatedBytes += gpu::skinningBufferSize();
#endif
    LOG_INFO("GPU recyclers RAM usage: %.1f KB", allocatedBytes * 1e-3f);

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

    _builtinMaterials[BuiltinMaterial::COLOR_MAP] = MATERIALS.acquire();
    _builtinMaterials[BuiltinMaterial::COLOR_MAP]->color = rgb(255, 255, 255);
    constexpr StaticTexture<8, 8, 4> colormap_tex{[](uint32_t x, uint32_t y) {
        glm::vec3 c;
        switch (x) {
        case 0:
            c = glm::vec3{1.0f, 0.0f, 0.0f};
            break;
        case 1:
            c = glm::vec3{0.0f, 1.0f, 0.0f};
            break;
        case 2:
            c = glm::vec3{0.0f, 0.0f, 1.0f};
            break;
        case 3:
            c = glm::vec3{1.0f, 1.0f, 0.0f};
            break;
        case 4:
            c = glm::vec3{0.0f, 1.0f, 1.0f};
            break;
        case 5:
            c = glm::vec3{1.0f, 0.0f, 1.0f};
            break;
        case 7:
            c = glm::vec3{0.0f, 0.0f, 0.0f};
            break;
        case 6:
            c = glm::vec3{1.0f, 1.0f, 1.0f};
            break;
        }
        return glm::vec4(c * (float)(y + 1.0f) / 8.0f, 1.0f);
    }};
    _builtinMaterials[BuiltinMaterial::COLOR_MAP]->textures.emplace(
        GL_TEXTURE0,
        createTexture(colormap_tex.buf, colormap_tex.width, colormap_tex.height,
                      static_cast<ChannelSetting>(colormap_tex.channels), GL_UNSIGNED_BYTE));
}

void gpu::dispose() {
    glDeleteVertexArrays(BYTESIZED_VERTEXARRAY_COUNT, (uint32_t *)VERTEXARRAYS.data());
    glDeleteBuffers(BYTESIZED_VERTEXBUFFER_COUNT, VERTEXBUFFERS.data());
    glDeleteTextures(BYTESIZED_TEXTURE_COUNT, (GLuint *)TEXTURES.data());
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
void gpu::Framebuffer::checkStatus(const char *label) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer '%s' is not complete: %d", label, id);
    }
}
void gpu::Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

gpu::VertexArray *gpu::createVertexArray() { return VERTEXARRAYS.acquire(); }
uint32_t *gpu::createVertexBuffer() { return VERTEXBUFFERS.acquire(); }

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
    prim->vao = VERTEXARRAYS.acquire();
    prim->vao->bind();

    uint32_t *vbo = prim->vbos.emplace_back(VERTEXBUFFERS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    vbo = prim->vbos.emplace_back(VERTEXBUFFERS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    vbo = prim->vbos.emplace_back(VERTEXBUFFERS.acquire());
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec2), uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(2);

    prim->ebo = VERTEXBUFFERS.acquire();
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

static const int _charbuf_len = 1024;
static GLchar _charbuf[_charbuf_len] = {};

void gpu::ShaderProgram::use() { glUseProgram(id); }

gpu::Uniform *gpu::ShaderProgram::uniform(const char *key) {
    auto it = uniforms.find(key);
    if (it == uniforms.end()) {
        return nullptr;
    }
    return &it->second;
}

bool gpu::Shader_compile(const gpu::Shader &shader, const char *src) {
    glShaderSource(shader.id, 1, &src, nullptr);
    glCompileShader(shader.id);
    GLint status{0};
    GLint length{0};
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(shader.id, GL_INFO_LOG_LENGTH, &length);
        GLint n = length < _charbuf_len ? length : _charbuf_len;
        glGetShaderInfoLog(shader.id, length, &n, _charbuf);
        printf("%s\n %.*s\n", src, n, _charbuf);
        return false;
    }
    return true;
}

void gpu::Shader_createProgram(ShaderProgram &prog) {
    prog.id = glCreateProgram();
    glAttachShader(prog.id, prog.vertex->id);
    glAttachShader(prog.id, prog.fragment->id);
    glLinkProgram(prog.id);
    GLint res = GL_FALSE;
    glGetProgramiv(prog.id, GL_LINK_STATUS, &res);
    if (!res) {
        glGetProgramInfoLog(prog.id, _charbuf_len, NULL, _charbuf);
        printf("%s\n", _charbuf);
        return;
    }
    prog.use();
    for (auto it = prog.uniforms.begin(); it != prog.uniforms.end(); ++it) {
        it->second.location = glGetUniformLocation(prog.id, it->first.c_str());
        it->second.update();
    }
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
    auto *tex = createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
    stbi_image_free((void *)idata);
    return tex;
}

gpu::Texture *gpu::createTextureFromMem(const uint8_t *addr, uint32_t len, bool flip) {
    int iw, ih, ic;
    stbi_set_flip_vertically_on_load(flip);
    const uint8_t *idata = stbi_load_from_memory((uint8_t *)addr, len, &iw, &ih, &ic, 0);
    auto *tex = createTexture(idata, iw, ih, static_cast<ChannelSetting>(ic), GL_UNSIGNED_BYTE);
    stbi_image_free((void *)idata);
    return tex;
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
    primitive->vao = VERTEXARRAYS.acquire();
    primitive->vbos.emplace_back(VERTEXBUFFERS.acquire());
    primitive->ebo = VERTEXBUFFERS.acquire();
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
        primitive->vao = VERTEXARRAYS.acquire();
        primitive->vao->bind();
        for (size_t j{0}; j < library::Primitive::Attribute::COUNT; ++j) {
            const library::Accessor *libraryAttr = libraryPrimitive.attributes[j];
            if (libraryAttr == nullptr) {
                continue;
            }
            uint32_t vbo = *primitive->vbos.emplace_back(VERTEXBUFFERS.acquire());
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
            primitive->ebo = VERTEXBUFFERS.acquire();
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
#ifdef BYTESIZED_USE_SKINNING
    assert(other.skin == nullptr);
#endif
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
#ifdef BYTESIZED_USE_SKINNING
        if (libraryChild->skin) {
            node->skin = gpu::createSkin(*libraryChild->skin);
        }
#endif
    }
#ifdef BYTESIZED_USE_SKINNING
    if (node->skin) {
        for (library::Node *joint : node->skin->librarySkin->joints) {
            node->skin->joints.emplace_back(node->childByName(joint->name.c_str()));
        }
    }
#endif
    return node;
};

static void freeMesh(gpu::Mesh *mesh) {
    for (auto &[primitive, material] : mesh->primitives) {

        VERTEXARRAYS.free(primitive->vao);
        for (uint32_t *vbo : primitive->vbos) {
            VERTEXBUFFERS.free(vbo);
        }
        VERTEXBUFFERS.free(primitive->ebo);
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
#ifdef BYTESIZED_USE_SKINNING
    if (node->skin) {
        for (auto *anim : node->skin->animations) {
            gpu::freeAnimation(anim);
        }
        node->skin->playback->animation = nullptr;
        gpu::freePlayback(node->skin->playback);
        gpu::freeSkin(node->skin);
    }
#endif
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
    vao->bind();
    if (ebo) {
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, NULL);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, count);
    }
    vao->unbind();
}

void gpu::UniformBuffer::bindShader(gpu::ShaderProgram *shader) {
    bind();
    bindBlock(shader, label);
    bindBufferBase();
    unbind();
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
    createUniformBuffer(UBO_CAMERA, cameraBlockLabel, sizeof(gpu::CameraBlock), &cameraBlock);
    createUniformBuffer(UBO_LIGHT, "LightBlock", sizeof(gpu::LightBlock), &lightBlock);
#ifdef BYTESIZED_USE_SKINNING
    static const char *skinBlockLabel = "SkinBlock";
    createUniformBuffer(UBO_SKINNING, skinBlockLabel, sizeof(gpu::SkinBlock), skinBlock.bones);
#endif
}

#ifdef BYTESIZED_USE_SKINNING
static void _updateBonesArray(gpu::Node *node) {
    const glm::mat4 globalWorldInverse = glm::inverse(node->model());
    for (size_t j{0}; j < node->skin->joints.size() && j < gpu::MAX_BONES; ++j) {
        skinBlock.bones[j] = globalWorldInverse * node->skin->joints.at(j)->model() *
                             node->skin->librarySkin->inverseBindMatrices.at(j);
    }
}
#endif

void gpu::Node::render(ShaderProgram *shaderProgram) {
    if (parent() == nullptr && !valid()) {
        invalidateRecursive(this);
    }
#ifdef BYTESIZED_USE_SKINNING
    if (skin) {
        gpu::UniformBuffer *ubo = gpu::builtinUBO(gpu::UBO_SKINNING);
        _updateBonesArray(this);
        ubo->bind();
        ubo->bufferData(sizeof(skinBlock), &skinBlock);
    }
#endif
    for (gpu::Node *child : children) {
        child->render(shaderProgram);
    }
#ifndef __EMSCRIPTEN__
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
#endif
    if (!hidden && mesh && !mesh->primitives.empty()) {
        shaderProgram->uniforms.at("u_model") << model();
        for (auto &[primitive, material] : mesh->primitives) {
            bindMaterial(shaderProgram, _overrideMaterial ? _overrideMaterial : material);
            primitive->render();
        }
    }
#ifndef __EMSCRIPTEN__
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#endif
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
    ubo->id = VERTEXBUFFERS.acquire();
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
    VERTEXBUFFERS.free(ubo->id);
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

struct SharedSource {
    const char *name;
    const char *src;
};
static const size_t SHARED_SOURCES_MAX = 10;
static SharedSource _sharedSources[SHARED_SOURCES_MAX];
void gpu::createSharedSource(const char *name, const char *src) {
    for (size_t i{0}; i < SHARED_SOURCES_MAX; ++i) {
        if (_sharedSources[i].name == nullptr) {
            _sharedSources[i].name = name;
            _sharedSources[i].src = src;
            return;
        }
    }
    LOG_ERROR("Failed to create shared source, array is full.");
}

const char *getSharedSource(const char *name) {
    for (size_t i{0}; i < SHARED_SOURCES_MAX; ++i) {
        if (_sharedSources[i].name == nullptr) {
            continue;
        }
        if (strcmp(_sharedSources[i].name, name) == 0) {
            return _sharedSources[i].src;
        }
    }
    return nullptr;
}

gpu::Shader *gpu::createShader(uint32_t type, const char *src,
                               const std::unordered_set<std::string> &defines) {
    gpu::Shader *shader = SHADERS.acquire();
    shader->id = glCreateShader(type);

    bool useShaderExtension{!defines.empty()};
    for (size_t i{0}; i < 100'000 && !useShaderExtension; ++i) {
        if (src[i] == '#') {
            if (strncmp(src + i, "#include", sizeof("#include") - 1) == 0) {
                useShaderExtension = true;
                break;
            }
        } else if (src[i] == '\0') {
            break;
        }
    }
    std::string str;
    if (useShaderExtension) {
        str.assign(src);

        if (!defines.empty()) {
            size_t i = str.find("\n");
            const std::string version = str.substr(0, i + 1);
            str = str.substr(i);
            for (const auto &define : defines) {
                str = "#define " + define + "\n" + str;
            }
            str = version + str;
        }

        size_t a = str.find("#include");
        while (a != std::string::npos) {
            size_t b0 = str.find('"', a);
            size_t b1 = str.find('"', b0 + 1);
            std::string name = str.substr(b0 + 1, (b1 - b0 - 1));
            const char *shared = getSharedSource(name.c_str());
            if (shared == nullptr) {
                LOG_WARN("Failed to find SharedSource: %s\n", name.c_str());
            }
            str.replace(a, b1 - a + 1, shared ? shared : "");
            a = str.find("#include", a + 1);
        }
        src = str.c_str();
        // printf("\n%s\n", src);
    }

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

gpu::Collection::Collection(const library::Collection &collection) { create(collection); }

void gpu::Collection::create(const library::Collection &collection) {
    scene = gpu::createScene(*collection.scene);
    LOG_INFO("Collection %s: #materials=%u #meshes=%u #nodes=%u #textures=%u",
             collection.scene->name.c_str(), collection.materials_count, collection.meshes_count,
             collection.nodes_count, collection.textures_count);
#ifdef BYTESIZED_USE_SKINNING
    for (size_t i{0}; i < collection.animations_count; ++i) {
        animations.emplace_back(gpu::createAnimation(collection.animations[i], nullptr));
    }
    if (!animations.empty()) {
        if (auto idleAnim = animationByName("Idle")) {
            idleAnim->start();
        } else {
            animations.front()->start();
        }
    }
#endif

    assert(scene->libraryScene != nullptr);
    assert(scene->libraryScene->gpuInstance == scene);
    for (gpu::Node *node : scene->nodes) {
        assert(node->libraryNode);
        assert(node->libraryNode->gpuInstance != nullptr);
        assert(node->libraryNode->scene == scene->libraryScene);
        assert(node->libraryNode->scene != nullptr);
    }
}

#ifdef BYTESIZED_USE_SKINNING
gpu::Animation *gpu::Collection::animationByName(const char *name) {
    for (auto &animation : animations) {
        if (animation->name.compare(name) == 0) {
            return animation;
        }
    }
    return nullptr;
}
#endif

const std::string &gpu::Collection::name() const {
    if (scene && scene->libraryScene) {
        return scene->libraryScene->name;
    }
    static const std::string noname{};
    return noname;
}

void gpu::renderScreen() {
    // clang-format off
    static const float screen_vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        +1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, +1.0f, 0.0f, 1.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
    };
    static const uint16_t screen_indices[] = {
        0, 1, 2,
        1, 3, 2
    };
    // clang-format off
    static VertexArray *vao{nullptr};
    static uint32_t *vbo{nullptr};
    static uint32_t *ebo{nullptr};
    if (vao == nullptr) {
        vao = createVertexArray();
        vao->bind();
        vbo = createVertexBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices,
                     GL_STATIC_DRAW);
        ebo = createVertexBuffer();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screen_indices), screen_indices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    }
    vao->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
}