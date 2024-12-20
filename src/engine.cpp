#include "engine.h"
#include "animation.h"
#include "assets.h"
#include "embed.h"
#include "gpu.h"
#include "gui.h"
#include "persist.h"
#include <algorithm>
#include <list>

assets::Collection *Engine::addCollection(const uint8_t *glbData) {
    return &_collections.emplace_back(*library::loadGLB(glbData));
}

static gpu::Framebuffer *fbo{nullptr};
static persist::SessionData sessionData;
static gpu::Node *grid{nullptr};
static Panel *_activePanel = nullptr;

void Engine::init(int drawableWidth, int drawableHeight) {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_STENCIL_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    _drawableWidth = drawableWidth;
    _drawableHeight = drawableHeight;

    // create all gpu objects
    gpu::allocate();

    bdf::Font *font = bdf::createFont(__bdf__boxxy, __bdf__boxxy_len);
    gui.create(font, _windowWidth, _windowHeight, 2.0f, GUI::EVERYTHING);

    const glm::mat4 perspProj =
        glm::perspective(glm::radians(45.0f), (float)_windowWidth / _windowHeight, 0.01f, 100.0f);
    const glm::vec4 defaultColor = Color::green.vec4();

    shaderProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::OBJECT_VERT),
                                             gpu::builtinShader(gpu::OBJECT_FRAG),
                                             {{"u_projection", perspProj},
                                              {"u_color", defaultColor},
                                              {"u_diffuse", 0},
                                              {"u_view", glm::mat4{1.0f}},
                                              {"u_model", glm::mat4{1.0f}}});

    animProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::ANIM_VERT),
                                           gpu::builtinShader(gpu::ANIM_FRAG),
                                           {{"u_projection", perspProj},
                                            {"u_color", defaultColor},
                                            {"u_diffuse", 0},
                                            {"u_view", glm::mat4{1.0f}},
                                            {"u_model", glm::mat4{1.0f}}});

    textProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::TEXT_VERT),
                                           gpu::builtinShader(gpu::TEXT_FRAG),
                                           {{"u_projection", perspProj},
                                            {"u_color", defaultColor},
                                            {"u_diffuse", 0},
                                            {"u_view", glm::mat4{1.0f}},
                                            {"u_model", glm::mat4{1.0f}},
                                            {"u_metallic", 1.0f},
                                            {"u_time", 0.0f}});

    uiProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::TEXT_VERT),
                                         gpu::builtinShader(gpu::TEXT_FRAG),
                                         {{"u_projection", gui.projection},
                                          {"u_color", defaultColor},
                                          {"u_diffuse", 0},
                                          {"u_view", gui.view},
                                          {"u_model", glm::mat4{1.0f}},
                                          {"u_metallic", 0.0f},
                                          {"u_time", 0.0f}});
    screenProgram =
        gpu::createShaderProgram(gpu::builtinShader(gpu::SCREEN_VERT),
                                 gpu::builtinShader(gpu::TEXTURE_FRAG), {{"u_texture", 0}});
    fbo = gpu::createFramebuffer();
    fbo->bind();
    fbo->createTexture(GL_COLOR_ATTACHMENT0, _windowWidth * 2, _windowHeight * 2,
                       gpu::ChannelSetting::RGB, GL_UNSIGNED_BYTE);
    fbo->createDepthStencil(_windowWidth * 2, _windowHeight * 2);
    fbo->checkStatus();
    fbo->unbind();
    _clickNPick.create(_windowWidth, _windowHeight, gpu::builtinShader(gpu::OBJECT_VERT), perspProj,
                       0.1f);

    auto &builtinCollection = _collections.emplace_back();
    builtinCollection.scene = gpu::createScene();

    for (size_t prim{gpu::PLANE}; prim < (size_t)gpu::PRIMITIVE_COUNT; ++prim) {
        gpu::Node *node = builtinCollection.scene->nodes.emplace_back(gpu::createNode());
        node->hidden = false;
        node->mesh = gpu::createMesh();
        node->mesh->primitives.emplace_back(gpu::builtinPrimitive((gpu::BuiltinPrimitive)prim),
                                            gpu::builtinMaterial(gpu::CHECKERS));
        node->translation = {(prim - gpu::PLANE) * 2.0f, 0.0f, 0.0f};
    }

    _app->init(drawableWidth, drawableHeight);
    if (persist::loadSession(sessionData)) {
        if (strlen(sessionData.recentFile) > 0) {
            openSaveFile(sessionData.recentFile);
        }
        _editor.setTargetView(sessionData.cameraView);
    }

    grid = gpu::createNode();
    grid->hidden = true;
    grid->mesh = gpu::createMesh();
    grid->mesh->primitives.emplace_back(gpu::builtinPrimitive(gpu::GRID),
                                        gpu::builtinMaterial(gpu::GRID_TILE));
    grid->rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
    grid->scale = glm::vec3{64.0f, 64.0f, 64.0f};
    _console.setSetting("wiremode", "0");
    _console.setSetting("tstep", "0");
    _console.setSetting("rstep", "0");
}

bool Engine::update(float dt) {
    _editor.update(dt);
    _editor.setTStep(_console.settingFloat("tstep"));
    _editor.setRStep(_console.settingFloat("rstep"));
    animation::animate(dt);
    return _app->update(dt);
}

void Engine::resize(int drawableWidth, int drawableHeight) {
    _drawableWidth = drawableWidth;
    _drawableHeight = drawableHeight;
}

static void _renderNodes(Editor &editor, gpu::ShaderProgram *shaderProgram,
                         std::vector<gpu::Node *> &nodes) {
    for (auto *node : nodes) {
        if (node == editor.selectedNode()) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            node->render(shaderProgram);
            glStencilMask(0x00);
        }
        node->render(shaderProgram);
    }
    if (auto sel = editor.selectedNode()) {
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        auto mat = gpu::builtinMaterial(gpu::BuiltinMaterial::WHITE);
        mat->color = Color(0x44AAFF) * Color::opacity(0.4f);
        gpu::setOverrideMaterial(mat);
        sel->render(shaderProgram);
        mat->color = Color(0x44FFAA) * Color::opacity(0.7f);
        sel->recursive([](gpu::Node *node) { node->wireframe = true; });
        sel->render(shaderProgram);
        sel->recursive([](gpu::Node *node) { node->wireframe = false; });
        gpu::setOverrideMaterial(nullptr);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}

void Engine::draw() {
    float t = Time::seconds();
    const glm::mat4 &view = _editor.view();
    _clickNPick.update(view);

    fbo->bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);
    // glViewport(0.0f, 0.0f, window::DRAWABLE_WIDTH * 0.5f, window::DRAWABLE_HEIGHT);

    if (_console.settingBool("wiremode")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (_saveFile) {
        shaderProgram->use();
        shaderProgram->uniforms.at("u_view") << view;
        _renderNodes(_editor, shaderProgram, _saveFile->nodes);

    } else {
        // render objects
        shaderProgram->use();
        shaderProgram->uniforms.at("u_view") << view;
        _renderNodes(_editor, shaderProgram, nodes);

        // render skeletal animations
        animProgram->use();
        animProgram->uniforms.at("u_view") << view;
        for (gpu::Node *node : skinNodes) {
            node->render(animProgram);
        }

        // render texts
        textProgram->use();
        textProgram->uniforms.at("u_view") << view;
        textProgram->uniforms.at("u_time") << t;
        for (gpu::Node *node : textNodes) {
            node->render(textProgram);
        }
    }

    shaderProgram->use();
    grid->render(shaderProgram);

    if (_console.settingBool("wiremode")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    gui.render(uiProgram);
    fbo->unbind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    fbo->textures.at(GL_COLOR_ATTACHMENT0)->bind();
    //_clickNPick.fbo->textures.at(GL_COLOR_ATTACHMENT0)->bind();

    screenProgram->use();
    gpu::builtinPrimitive(gpu::BuiltinPrimitive::SCREEN)->render();
}

bool Engine::keyDown(int key, int /*mods*/) {
    switch (key) {
    case SDLK_1:
    case SDLK_2:
    case SDLK_3:
    case SDLK_4:
    case SDLK_5:
    case SDLK_6:
    case SDLK_7:
    case SDLK_8:
    case SDLK_9:
        openPanel(key - SDLK_1);
        return true;
    case SDLK_b:
        if (_blockMode) {
            _console.setSetting("tstep", "0");
            grid->hidden = true;
        } else {
            _console.setSetting("tstep", "1");
            grid->hidden = false;
        }
        _blockMode = !_blockMode;
        return true;
    case SDLK_l:
        animation::SETTINGS_LERP = !animation::SETTINGS_LERP;
        return true;
    }
    return false;
}

bool Engine::keyUp(int /*key*/, int /*mods*/) { return false; }

void Engine::inputChanged(bool visible, const char * /*value*/) {
    auto *text = gui.setConsoleText(_console.commandLine);
    text->node->hidden = !visible;
}

void Engine::fps(float fps) { gui.setFps(fps); }

void Engine::listNodes() {
    for (const auto &collection : _collections) {
        for (auto *node : collection.scene->nodes) {
            printf("%s\n", node->name().c_str());
        }
    }
}

bool Engine::spawnNode(const char *name) {
    if (_saveFile == nullptr) {
        return false;
    }
    for (const auto &collection : _collections) {
        if (auto *node = collection.scene->nodeByName(name)) {
            _editor.addNode(gpu::createNode(*node->gltfNode));
            // ecs::Entity *entity = ecs::create();
            return true;
        }
    }
    return false;
}

void Engine::_openSaveFile(persist::SaveFile &saveFile) {
    _saveFile = &saveFile;
    gui.setTitleText(saveFile.path);
    _editor.selectNode(nullptr);
    _clickNPick.clear();
    std::for_each(saveFile.nodes.begin(), saveFile.nodes.end(),
                  [this](gpu::Node *node) { _clickNPick.registerNode(node); });
}

bool Engine::openSaveFile(const char *name) {
    for (auto &saveFile : _saveFiles) {
        if (strcmp(saveFile.path, name) == 0) {
            _openSaveFile(saveFile);
            return true;
        }
    }
    if (strlen(name) > 0) {
        auto &saveFile = _saveFiles.emplace_back();
        strcpy(saveFile.path, name);
        loadWorld(name, saveFile, [this](const char *name) { return findCollection(name)->scene; });
        _openSaveFile(saveFile);
        assignPanel(Panel::SAVE_FILE, (void *)&saveFile);
        return true;
    }
    return false;
}
bool Engine::saveSaveFile(const char *name) {
    if (_saveFile == nullptr) {
        return false;
    }
    if (name != nullptr && strlen(name) > 0) {
        saveWorld(name, *_saveFile);
        _saveFile->dirty = false;
        return true;
    } else {
        saveWorld(_saveFile->path, *_saveFile);
        _saveFile->dirty = false;
        return true;
    }
    return false;
}

bool Engine::assignPanel(Panel::Type type, void *ptr) {
    const int N = sizeof(_panels) / sizeof(Panel);
    static int nextFree{0};
    for (auto &panel : _panels) {
        if (panel.ptr == ptr) {
            assert(panel.type == panel.type);
            return false;
        }
    }
    _panels[nextFree].ptr = ptr;
    _panels[nextFree].type = type;
    _panels[nextFree].cameraView = {
        {0.0f, 0.0f, 0.0f}, glm::radians(90.0f), glm::radians(10.0f), 10.0f};
    changePanel(_panels + nextFree);
    nextFree = (nextFree + 1) % N;
    return true;
}

bool Engine::openPanel(size_t index) {
    if (index < 10) {
        auto &panel = _panels[index];
        if (&panel == _activePanel) {
            return false;
        }
        switch (panel.type) {
        case Panel::COLLECTION:
            _openCollection(*((assets::Collection *)panel.ptr));
            break;
        case Panel::SAVE_FILE:
            _openSaveFile(*((persist::SaveFile *)panel.ptr));
            break;
        default:
            return false;
        }
        changePanel(_panels + index);
    }
    return true;
}

void Engine::changePanel(Panel *panel) {
    if (_activePanel) {
        _activePanel->cameraView = _editor.targetView();
    }
    _activePanel = panel;
    _editor.setTargetView(panel->cameraView);
    _editor.setCurrentView(panel->cameraView);
}

void Engine::_openCollection(const assets::Collection &collection) {
    unstage();
    gui.setTitleText(collection.name().c_str());
    stage(*collection.scene);
    _editor.selectNode(nullptr);
    _saveFile = nullptr;
}

bool Engine::openScene(size_t index) {
    if (index < _collections.size()) {
        auto it = _collections.begin();
        std::advance(it, index);
        _openCollection(*it);
        assignPanel(Panel::COLLECTION, (void *)&(*it));
        return true;
    }
    return false;
}

bool Engine::openScene(const char *name) {
    if (auto *collection = findCollection(name)) {
        _openCollection(*collection);
        assignPanel(Panel::COLLECTION, (void *)collection);
        return true;
    }
    return false;
}

bool Engine::nodeClicked(gpu::Node *node) {
    if (node) {
        _editor.selectNode(node);
    }
    return false;
}

void Engine::quit(bool force) {
    if (_saveFile && _saveFile->dirty && !force) {
        return;
    }
    if (_saveFile) {
        strcpy(sessionData.recentFile, _saveFile->path);
    } /*else {
        std::memset(sessionData.recentFile, 0, sizeof(persist::SessionData::recentFile));
    }*/
    sessionData.cameraView = _editor.targetView();
    persist::storeSession(sessionData);
    exit(0);
}

static void _updateNodeInfo(GUI &gui, gpu::Node *node) {
    if (node) {
        gui.setNodeInfo(node->name().c_str(), node->mesh ? node->meshName().c_str() : "-",
                        node->translation.data(), node->rotation.data(), node->scale.data());
    }
    gui.showNodeInfo(node != nullptr);
}

void Engine::nodeSelected(gpu::Node *node) { _updateNodeInfo(gui, node); }

void Engine::nodeAdded(gpu::Node *node) {
    _saveFile->nodes.emplace_back(node);
    _saveFile->dirty = true;
    _clickNPick.registerNode(node);
    _editor.selectNode(node);
}

void Engine::nodeRemoved(gpu::Node *node) {
    _saveFile->nodes.erase(std::find(_saveFile->nodes.begin(), _saveFile->nodes.end(), node));
    _saveFile->dirty = true;
    _clickNPick.unregisterNode(node);
    _editor.selectNode(nullptr);
}

void Engine::nodeCopied(gpu::Node *node) {
    if (_saveFile) {
        if (node->gltfNode) {
            _editor.addNode(gpu::createNode(*node->gltfNode));
        } else {
            gpu::Node *clone = gpu::createNode(*node);
            _editor.addNode(clone);
        }
        _editor.startTranslation();
    }
}

void Engine::nodeTransformed(gpu::Node *node) { _updateNodeInfo(gui, node); }

gpu::Node *Engine::cycleNode(gpu::Node *prev) {
    if (_saveFile) {
        if (prev) {
            bool found{false};
            for (auto *node : _saveFile->nodes) {
                if (found) {
                    return node;
                }
                found = prev == node;
            }
        }
        return _saveFile->nodes.empty() ? nullptr : _saveFile->nodes.front();
    }
    if (prev) {
        bool found{false};
        for (gpu::Node *node : nodes) {
            if (found) {
                return node;
            }
            if (node == prev) {
                found = true;
            }
        }
        for (gpu::Node *node : skinNodes) {
            if (found) {
                return node;
            }
            if (node == prev) {
                found = true;
            }
        }
        assert(found);
    }
    return nodes.empty() ? nullptr : nodes.front();
}

Engine::~Engine() { gpu::dispose(); }

[[maybe_unused]] inline static void printNode(gpu::Node *node, std::string tab) {
    printf("%s%s", tab.c_str(), node->gltfNode->name.c_str());
    printf(" t=[%.1f %.1f %.1f]", node->translation.x, node->translation.y, node->translation.z);
    printf(" r=[%.1f %.1f %.1f %.1f]", node->rotation.data().w, node->rotation.x, node->rotation.y,
           node->rotation.z);
    printf(" s=[%.1f %.1f %.1f]", node->scale.x, node->scale.y, node->scale.z);
    printf("\n");
    for (gpu::Node *child : node->children) {
        printNode(child, tab + "  ");
    }
}

void Engine::unstage() {
    nodes.clear();
    skinNodes.clear();
    _clickNPick.clear();
};

void Engine::stage(const gpu::Scene &scene) {
    for (gpu::Node *node : scene.nodes) {
        if (node->skin) {
            skinNodes.emplace_back(node);
        } else {
            nodes.emplace_back(node);
            _clickNPick.registerNode(node);
        }
    }
}
assets::Collection *Engine::findCollection(const char *name) {
    for (auto &collection : _collections) {
        if (collection.name().compare(name) == 0) {
            return &collection;
        }
    }
    return nullptr;
}