#pragma once

#include "bdf.h"
#include "color.h"
#include "gltf.hpp"
#include "opengl.h"
#include "recycler.hpp"
#include "shader.h"
#include "texture.h"
#include "trs.h"
#include "uniform.h"
#include <functional>
#include <glm/glm.hpp>
#include <string>

namespace gpu {

struct Material {
    Color color;
    float metallic;
    float roughness;
    std::unordered_map<uint32_t, Texture *> textures;
};

void bindMaterial(ShaderProgram *shaderProgram, Material *material);

struct Primitive {
    uint32_t *vao;
    std::vector<uint32_t *> vbos;
    uint32_t *ebo;
    uint32_t count;

    void render();
};

struct UniformBuffer {
    uint32_t *id;
    uint32_t bindingPoint;
    size_t length;
    void *data;

    void bind();
    void unbind();
    void bindBlock(gpu::ShaderProgram *shader, const char *blockLabel);
    void bindBufferBase();
    void bufferData(uint32_t length, void *data);
    void bufferSubData(uint32_t offset, uint32_t length, void *data);
};

struct Mesh {
    const gltf::Mesh *gltfMesh;
    std::vector<std::pair<Primitive *, Material *>> primitives;
};

struct Node : TRS {
    const gltf::Node *gltfNode;
    Mesh *mesh;
    std::vector<Node *> children;
    bool hidden;
    struct Skin *skin;
    bool wireframe;

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

struct Skin {
    const gltf::Skin *gltfSkin;
    std::vector<Node *> joints;
    std::vector<glm::mat4> invBindMatrices;
};

struct Scene {
    const gltf::Scene *gltfScene;
    std::vector<Node *> nodes;
    Node *nodeByName(const char *name);
};

struct Text {
    const bdf::Font *bdfFont;
    Node *node;

    void init();
    void setText(const char *value, bool center);
    const char *text() const;
    float width() const;
    float height() const;

  private:
    const char *_value;
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
    void checkStatus();
    void unbind();
};

void allocate();
void dispose();

void setOverrideMaterial(gpu::Material *material);

Mesh *createMesh();
Primitive *createPrimitive(const void *vertices, size_t vertices_len, const uint16_t *indices,
                           size_t indices_len);
enum BuiltinMaterial { CHECKERS, GRID_TILE, WHITE, MATERIAL_COUNT };
Material *builtinMaterial(BuiltinMaterial builtinMaterial);
enum BuiltinPrimitive { SCREEN, GRID, PLANE, CUBE, SPHERE, CYLINDER, CONE, PRIMITIVE_COUNT };
Primitive *builtinPrimitive(BuiltinPrimitive builtinPrimitive);

Framebuffer *createFramebuffer();
void freeFramebuffer(Framebuffer *framebuffer);

Texture *createTexture(const uint8_t *data, uint32_t width, uint32_t height,
                       ChannelSetting channels, uint32_t type);
Texture *createTextureFromFile(const char *data);
Texture *createTextureFromMem(const uint8_t *addr, uint32_t len);
Texture *createTexture(const gltf::Texture &texture);
Texture *createTexture(const bdf::Font &font);

Text *createText(const bdf::Font &font, const char *text, bool center);
void freeText(gpu::Text *text);

void freeTexture(Texture *texture);

Material *createMaterial();
Material *createMaterial(const gltf::Material &material);
void freeMaterial(Material *material);

Node *createNode();
Node *createNode(const gpu::Node &other);
Node *createNode(const gltf::Node &node);
void freeNode(gpu::Node *node);

Scene *createScene();
Scene *createScene(const gltf::Scene &scene);
void freeScene(gpu::Scene *scene);

UniformBuffer *createUniformBuffer(uint32_t bindingPoint, uint32_t len, void *data = NULL);
void freeUniformBuffer(UniformBuffer *ubo);

Shader *createShader(uint32_t type, const char *src);
void freeShader(Shader *shader);
ShaderProgram *createShaderProgram(Shader *vertex, Shader *fragment,
                                   std::unordered_map<std::string, Uniform> &&uniforms);
void freeShaderProgram(ShaderProgram *shaderProgram);

}; // namespace gpu