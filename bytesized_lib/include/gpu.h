#pragma once

#include "bdf.h"
#include "bytesized_info.h"
#include "color.h"
#include "ecs.h"
#include "gpu_skinning.h"
#include "gpu_texture.h"
#include "library_types.h"
#include "opengl.h"
#include "recycler.hpp"
#include "trs.h"
#include "uniform.h"
#include "vector.h"
#include "vertexobject.h"
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <unordered_set>

namespace gpu {

struct Shader {
    uint32_t id;
};

struct ShaderProgram {
    uint32_t id;
    Shader *vertex;
    Shader *fragment;
    std::unordered_map<std::string, Uniform> uniforms;
    Uniform *uniform(const char *key);

    void use();
};

bool Shader_compile(const gpu::Shader &id, const char *src);
void Shader_createProgram(ShaderProgram &shaderProgram);

struct Material {
    Color color;
    float metallic;
    float roughness;
    std::unordered_map<uint32_t, Texture *> textures;
};

void bindMaterial(ShaderProgram *shaderProgram, Material *material);

struct VertexArray {
    uint32_t id;
    void bind() { glBindVertexArray(id); }
    void unbind() { glBindVertexArray(0); }
};
static_assert(sizeof(VertexArray) == sizeof(uint32_t));

struct Primitive {
    VertexArray *vao;
    std::vector<uint32_t *> vbos;
    uint32_t *ebo;
    uint32_t count;

    void render();
};

struct UniformBuffer {
    uint32_t *id;
    uint32_t bindingPoint;
    const char *label;
    void bindShader(gpu::ShaderProgram *shaders);
    void bindShaders(std::initializer_list<gpu::ShaderProgram *> shaders);

    void bind();
    void unbind();
    void bindBlock(gpu::ShaderProgram *shader, const char *blockLabel);
    void bindBufferBase();
    void bufferData(uint32_t length, void *data_);
    void bufferSubData(uint32_t offset, uint32_t length_, void *data_);
};

struct Mesh {
    const library::Mesh *libraryMesh;
    std::vector<std::pair<Primitive *, Material *>> primitives;
};

struct Node : TRS {
    const library::Node *libraryNode;
    Mesh *mesh;
    std::vector<Node *> children;
    bool hidden;
    bool wireframe;
    ecs::Entity *entity;
    Node *find(std::function<bool(Node *)> callback) {
        if (callback(this)) {
            return this;
        }
        for (Node *child : children) {
            if (auto n = child->find(callback)) {
                return n;
            }
        }
        return nullptr;
    }
#ifdef BYTESIZED_USE_SKINNING
    struct Skin *skin;
    struct Skin *getSkin() {
        if (skin) {
            return skin;
        }
        auto *n = find([](Node *n) { return n->skin; });
        return n ? n->skin : nullptr;
    }
#endif

    void render(ShaderProgram *shaderProgram);
    Node *childByName(const char *name);
    const std::string &name() const;
    const std::string &meshName() const;

    void addChild(gpu::Node *node);
    void recursive(const std::function<void(gpu::Node *)> &callback);
    void forEachChild(const std::function<void(gpu::Node *)> &callback);

    Primitive *primitive(size_t primitiveIndex = 0);
    Material *material(size_t primitiveIndex = 0);
};

struct Scene {
    const library::Scene *libraryScene;
    std::vector<Node *> nodes;
    Node *nodeByName(const char *name);
};

struct Text {
    const bdf::Font *bdfFont;
    Node *node;

    void init();
    void setText(const char *value, bool center);
    float width() const;
    float height() const;

    const char *value;
};

struct Framebuffer {
    uint32_t id;
    std::unordered_map<uint32_t, Texture *> textures;
    uint32_t rbo;

    void bind();
    void attach(uint32_t attachment, Texture *texture);
    void createTexture(uint32_t attachment, uint32_t width, uint32_t height,
                       ChannelSetting channels, uint32_t type);
    void createDepthStencil(uint32_t width, uint32_t height);
    void createRenderBufferDS(uint32_t width, uint32_t height);
    void checkStatus(const char *label);
    void unbind();
};

void printAllocations();
void allocate();
void dispose();

void setOverrideMaterial(gpu::Material *material);

VertexArray *createVertexArray();
uint32_t *createVertexBuffer();

Mesh *createMesh();
Mesh *createMesh(gpu::Primitive *primitive, gpu::Material *material = nullptr);
Primitive *createPrimitive(const glm::vec3 *positions, const glm::vec3 *normals,
                           const glm::vec2 *uvs, size_t vertex_count, const uint16_t *indices,
                           size_t index_count);
Primitive *createPrimitive(const VertexObject &vertexObject);

enum BuiltinMaterial { CHECKERS, GRID_TILE, WHITE, COLOR_MAP, MATERIAL_COUNT };
Material *builtinMaterial(BuiltinMaterial builtinMaterial);

Framebuffer *createFramebuffer();
void freeFramebuffer(Framebuffer *framebuffer);

Texture *createTexture(const uint8_t *data, uint32_t width, uint32_t height,
                       ChannelSetting channels, uint32_t type);
Texture *createTextureFromFile(const char *data, bool flip);
Texture *createTextureFromMem(const uint8_t *addr, uint32_t len, bool flip);
Texture *createTexture(const library::Texture &texture);
Texture *createTexture(const bdf::Font &font);

Text *createText(const bdf::Font &font, const char *text, bool center);
void freeText(gpu::Text *text);

void freeTexture(Texture *texture);

Material *createMaterial();
Material *createMaterial(const Color &color);
Material *createMaterial(gpu::Texture *texture);
Material *createMaterial(const library::Material &material);
void freeMaterial(Material *material);

Node *createNode();
Node *createNode(gpu::Mesh *mesh);
Node *createNode(const gpu::Node &other);
Node *createNode(const library::Node &node);
void freeNode(gpu::Node *node);
size_t nodeCount();

Scene *createScene();
Scene *createScene(const library::Scene &scene);
void freeScene(gpu::Scene *scene);

UniformBuffer *createUniformBuffer(uint32_t bindingPoint, const char *label, uint32_t len,
                                   void *data = NULL);
void freeUniformBuffer(UniformBuffer *ubo);

void createSharedSource(const char *name, const char *src);
Shader *createShader(uint32_t type, const char *src,
                     const std::unordered_set<std::string> &defines = {});
void freeShader(Shader *shader);
ShaderProgram *createShaderProgram(Shader *vertex, Shader *fragment,
                                   std::unordered_map<std::string, Uniform> &&uniforms);
void freeShaderProgram(ShaderProgram *shaderProgram);

struct CameraBlock {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 cameraPos;
    float padding;
};
void CameraBlock_setProjection(const glm::mat4 &projection);
void CameraBlock_setViewPos(const glm::mat4 &view, const glm::vec3 &pos);
static constexpr int MAX_BONES = 32;
struct SkinBlock {
    glm::mat4 bones[MAX_BONES];
};
SkinBlock &getSkinBlock();

struct LightBlock {
    glm::vec4 lightHigh;
    glm::vec4 lightLow;
    glm::vec4 ambient;
};
void LightBlock_setLightColor(const Color &high, const Color &low, const Color &ambient);

struct GUIBlock {
    glm::mat4 projection;
    glm::mat4 view;
};

enum BuiltinUBO {
    UBO_CAMERA,
    UBO_LIGHT,
#ifdef BYTESIZED_USE_SKINNING
    UBO_SKINNING,
#endif
};
UniformBuffer *builtinUBO(BuiltinUBO bultinUBO);
void createBuiltinUBOs();

struct Collection {
    Collection(const library::Collection &collection);
    void create(const library::Collection &collection);
    const std::string &name() const;
    gpu::Scene *scene;
    std::vector<Texture> texture;
#ifdef BYTESIZED_USE_SKINNING
    gpu::Animation *animationByName(const char *name);
    std::vector<gpu::Animation *> animations;
#endif
};

void renderScreen();

}; // namespace gpu