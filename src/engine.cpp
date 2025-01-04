#include "engine.h"
#include "animation.h"
#include "assets.h"
#include "component.h"
#include "dexterity.h"
#include "ecs.h"
#include "embed.h"
#include "geom_primitive.h"
#include "gpu.h"
#include "gui.h"
#include "persist.h"
#include "physics.h"
#include "primer.h"
#include "primitive.h"
#include <algorithm>
#include <list>

assets::Collection *Engine::addCollection(const library::Collection &collection) {
    return &_collections.emplace_back(collection);
}

static gpu::Framebuffer *fbo{nullptr};
static persist::SessionData sessionData;
static gpu::Node *gridNode{nullptr};
static Panel *_activePanel = nullptr;
constexpr gpu::Plane<1, 1> grid(64.0f);
static bool _playing = false;

struct Player {
    gpu::Node *node;
    Actor *actor;
    Controller *ctrl;
};
static Player _player;

static Vector axisVectors[] = {
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, rgb(255, 0, 0)},
    {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, rgb(0, 255, 0)},
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, rgb(0, 0, 255)},
};

// Slow, should probably store entity in gpu::Node struct in the future
/*static ecs::Entity *_findEntityByNode(gpu::Node *node) {
    ecs::Entity *entity =
        ecs::System<CActor>::find([node](Actor &actor) { return actor.trs == node; });
    if (entity == nullptr) {
        entity = ecs::System<CPawn>::find([node](Pawn &pawn) { return pawn.trs == node; });
    }
    return entity;
}*/

static geom::Geometry::Type _parseGeometryType(const char *par) {
    if (starts_with(par, "plane")) {
        return geom::Geometry::Type::PLANE;
    } else if (starts_with(par, "sphere")) {
        return geom::Geometry::Type::SPHERE;
    } else if (starts_with(par, "aabb")) {
        return geom::Geometry::Type::AABB;
    } else if (starts_with(par, "obb")) {
        return geom::Geometry::Type::OBB;
    }
    return geom::Geometry::Type::AABB;
}

static void _attachCollider(gpu::Node *node, geom::Geometry::Type type, bool dynamic) {
    ecs::Entity *entity = (ecs::Entity *)node->extra;
    if (entity == nullptr) {
        entity = ecs::create_entity();
        node->extra = entity;
    }
    if (dynamic) {
        ecs::attach<CActor, CCollider>(entity);
        CActor::get(entity).trs = node;
    } else {
        ecs::attach<CPawn, CCollider>(entity);
        CPawn::get(entity).trs = node;
    }
    if (node->mesh) {
        auto [positions, length] = node->mesh->libraryMesh->primitives.front().positions();
        auto &colider = CCollider::get(entity);
        switch (type) {
        case geom::Geometry::Type::PLANE: {
            auto *plane = geom::createPlane();
            assert(!dynamic); // no support
            plane->construct(positions, length, dynamic ? nullptr : node);
            colider.geometry = plane;
            colider.transforming = dynamic;
        } break;
        case geom::Geometry::Type::SPHERE: {
            auto *sphere = geom::createSphere();
            sphere->construct(positions, length, dynamic ? nullptr : node);
            colider.geometry = sphere;
            colider.transforming = dynamic;
        } break;
        case geom::Geometry::Type::AABB: {
            auto *aabb = geom::createAABB();
            aabb->construct(positions, length, dynamic ? nullptr : node);
            colider.geometry = aabb;
            colider.transforming = dynamic;
            /*gpu::Node *node = gpu::createNode(
                gpu::createMesh(gpu::builtinPrimitives(gpu::CUBE),
                                gpu::builtinMaterial(gpu::BuiltinMaterial::WHITE)));
            glm::vec3 e = (aabb->max - aabb->min) * 0.5f;
            node->scale = e;
            sel->addChild(node);*/
            // engine->vectors.emplace_back(aabb->min);
            // engine->vectors.emplace_back(aabb->max);
        } break;
        case geom::Geometry::Type::OBB: {
            auto *obb = geom::createOBB();
            obb->construct(positions, length, dynamic ? nullptr : node);
            colider.geometry = obb;
            colider.transforming = dynamic;
        } break;
        default:
        }
    }
}

static void _attachController(gpu::Node *node, geom::Geometry::Type type) {
    _attachCollider(node, type, true);
    ecs::Entity *entity = (ecs::Entity *)node->extra;
    CController::attach(entity);
    auto &collider = CCollider::get(entity);
    auto &ctrl = CController::get(entity);
    _player.node = node;
    _player.actor = CActor::get_pointer(entity);
    _player.ctrl = &ctrl;
    geom::Sphere *s1 = dynamic_cast<geom::Sphere *>(collider.geometry);
    geom::Sphere *s2 = geom::createSphere();
    ctrl.floorSense = &s2->construct(s2->_center, s1->_radii + 0.1f);
    ctrl.walkControl = {0.0f, 0.0f, 0.0f};
    ctrl.walkSpeed = 10.0f;
    ctrl.jumpControl = 0;
    ctrl.jumpSpeed = 10.0f;
    ctrl.onGround = false;
}

void Engine::init(int drawableWidth, int drawableHeight) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_STENCIL_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    _drawableWidth = drawableWidth;
    _drawableHeight = drawableHeight;

    perspectiveProjection =
        glm::perspective(glm::radians(45.0f), (float)drawableWidth / drawableHeight, 0.1f, 200.0f);

    // create all gpu objects
    gpu::allocate();

    auto builtinGeoms = _collections.emplace_back(*gpu::createBuiltinPrimitives());

    bdf::Font *font = bdf::createFont(__bdf__boxxy, __bdf__boxxy_len);
    gui.create(font, _windowWidth, _windowHeight, 2.0f, GUI::EVERYTHING);

    const glm::vec4 defaultColor = Color::green.vec4();

    shaderProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::OBJECT_VERT),
                                             gpu::builtinShader(gpu::OBJECT_FRAG),
                                             {{"u_projection", perspectiveProjection},
                                              {"u_color", defaultColor},
                                              {"u_diffuse", 0},
                                              {"u_view", glm::mat4{1.0f}},
                                              {"u_model", glm::mat4{1.0f}}});

    animProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::ANIM_VERT),
                                           gpu::builtinShader(gpu::ANIM_FRAG),
                                           {{"u_projection", perspectiveProjection},
                                            {"u_color", defaultColor},
                                            {"u_diffuse", 0},
                                            {"u_view", glm::mat4{1.0f}},
                                            {"u_model", glm::mat4{1.0f}}});

    const Color bgColor(0xe0f8d0);
    const Color textColor(0x081820);
    textProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::TEXT_VERT),
                                           gpu::builtinShader(gpu::TEXT_FRAG),
                                           {{"u_projection", perspectiveProjection},
                                            {"u_bg_color", bgColor.vec4()},
                                            {"u_text_color", textColor.vec4()},
                                            {"u_diffuse", 0},
                                            {"u_view", glm::mat4{1.0f}},
                                            {"u_model", glm::mat4{1.0f}},
                                            {"u_metallic", 1.0f},
                                            {"u_time", 0.0f}});

    uiProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::OBJECT_VERT),
                                         gpu::builtinShader(gpu::TEXTURE_FRAG),
                                         {{"u_projection", gui.projection},
                                          {"u_diffuse", 0},
                                          {"u_view", gui.view},
                                          {"u_model", glm::mat4{1.0f}}});

    uiTextProgram = gpu::createShaderProgram(gpu::builtinShader(gpu::TEXT_VERT),
                                             gpu::builtinShader(gpu::TEXT_FRAG),
                                             {{"u_projection", gui.projection},
                                              {"u_bg_color", bgColor.vec4()},
                                              {"u_text_color", textColor.vec4()},
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
    _clickNPick.create(_windowWidth, _windowHeight, gpu::builtinShader(gpu::OBJECT_VERT),
                       perspectiveProjection, 0.1f);

    _app->init(drawableWidth, drawableHeight);
    if (persist::loadSession(sessionData)) {
        if (strlen(sessionData.recentFile) > 0) {
            openSaveFile(sessionData.recentFile);
        }
        _editor.setTargetView(sessionData.cameraView);
    }

    gridNode = gpu::createNode();
    gridNode->hidden = true;
    gridNode->mesh = gpu::createMesh();
    auto gridPrim = gpu::createPrimitive(grid.positions, grid.normals, grid.uvs, grid.CELLS,
                                         grid.indices, grid.CELLS * 6);
    gridNode->mesh->primitives.emplace_back(gridPrim, gpu::builtinMaterial(gpu::GRID_TILE));
    gridNode->translation = {0.0f, 0.01f, 0.0f};
    gridNode->rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
    gridNode->scale = glm::vec3{64.0f, 64.0f, 64.0f};
    _console.setSetting("wiremode", "0");
    _console.setSetting("tstep", "0");
    _console.setSetting("rstep", "0");
    _console.addCustomCommand(":static ", [this](const char *key) {
        if (auto sel = _editor.selectedNode()) {
            _attachCollider(sel, _parseGeometryType(key + strlen(":static ")), false);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":dynamic ", [this](const char *key) {
        if (auto sel = _editor.selectedNode()) {
            _attachCollider(sel, _parseGeometryType(key + strlen(":dynamic ")), true);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":spyro", [this](const char *key) {
        if (auto sel = _editor.selectedNode()) {
            _attachController(sel, geom::Geometry::Type::SPHERE);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":play", [this](const char * /*key*/) {
        if (!_playing) {
            _playing = true;
            _editor.disable();
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":stop", [this](const char * /*key*/) {
        if (_playing) {
            _playing = false;
            _editor.enable();
            return true;
        }
        return false;
    });
}

bool Engine::update(float dt) {
    _editor.update(dt);
    _editor.setTStep(_console.settingFloat("tstep"));
    _editor.setRStep(_console.settingFloat("rstep"));
    if (auto sel = _editor.selectedNode()) {
        for (auto &vec : axisVectors) {
            vec.O = sel->translation.data();
            vec.hidden = false;
        }
    } else {
        for (auto &vec : axisVectors) {
            vec.hidden = true;
        }
    }
    if (_playing) {
        physics_step(dt);
    }
    animation::animate(dt);
    return _app->update(dt);
}

void Engine::resize(int drawableWidth, int drawableHeight) {
    _drawableWidth = drawableWidth;
    _drawableHeight = drawableHeight;
}

static void _renderNodes(Editor &editor, Console &console, gpu::ShaderProgram *shaderProgram,
                         std::vector<gpu::Node *> &nodes, std::list<Vector> &vectors) {
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
        // glDisable(GL_CULL_FACE);
        auto mat = gpu::builtinMaterial(gpu::BuiltinMaterial::WHITE);
        mat->color = Color(0x44AAFF) * Color::opacity(0.4f);
        gpu::setOverrideMaterial(mat);
        sel->render(shaderProgram);
        mat->color = Color(0x44FFAA) * Color::opacity(0.7f);
        sel->recursive([](gpu::Node *node) { node->wireframe = true; });
        sel->render(shaderProgram);
        sel->recursive([](gpu::Node *node) { node->wireframe = false; });
        gpu::setOverrideMaterial(nullptr);
        // glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }
    glDisable(GL_DEPTH_TEST);
    for (auto &vector : axisVectors) {
        gpu::renderVector(shaderProgram, vector);
    }
    for (auto &vector : vectors) {
        gpu::renderVector(shaderProgram, vector);
    }
    glEnable(GL_DEPTH_TEST);
}

void Engine::draw() {
    float t = Time::seconds();
    const glm::mat4 &view = _editor.view();
    _clickNPick.update(view);

    fbo->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);
    // glViewport(0.0f, 0.0f, window::DRAWABLE_WIDTH * 0.5f, window::DRAWABLE_HEIGHT);

    if (_console.settingBool("wiremode")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (_saveFile) {
        shaderProgram->use();
        shaderProgram->uniforms.at("u_view") << view;
        _renderNodes(_editor, _console, shaderProgram, _saveFile->nodes, vectors);

    } else {
        // render objects
        shaderProgram->use();
        shaderProgram->uniforms.at("u_view") << view;
        _renderNodes(_editor, _console, shaderProgram, nodes, vectors);

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
    glDepthFunc(GL_LEQUAL);
    gridNode->render(shaderProgram);
    glDepthFunc(GL_LESS);

    if (_console.settingBool("wiremode")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    gui.render(uiProgram, uiTextProgram);

    fbo->unbind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    fbo->textures.at(GL_COLOR_ATTACHMENT0)->bind();
    //_clickNPick.fbo->textures.at(GL_COLOR_ATTACHMENT0)->bind();

    screenProgram->use();
    gpu::renderScreen();
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
            gridNode->hidden = true;
        } else {
            _console.setSetting("tstep", "1");
            gridNode->hidden = false;
        }
        _blockMode = !_blockMode;
        return true;
    case SDLK_k:
        _editor.setTargetView(
            {{0.0f, 0.0f, 0.0f}, glm::radians(90.0f), glm::radians(15.0f), 10.0f});
        _editor.setCurrentView(_editor.targetView());
        return true;
    case SDLK_l:
        animation::SETTINGS_LERP = !animation::SETTINGS_LERP;
        return true;
    case SDLK_w:
        if (_player.ctrl) {
            _player.ctrl->walkControl.z = -1.0f;
            return true;
        }
        break;
    case SDLK_a:
        if (_player.ctrl) {
            _player.ctrl->walkControl.x = -1.0f;
            return true;
        }
        break;
    case SDLK_s:
        if (_player.ctrl) {
            _player.ctrl->walkControl.z = 1.0f;
            return true;
        }
        break;
    case SDLK_d:
        if (_player.ctrl) {
            _player.ctrl->walkControl.x = 1.0f;
            return true;
        }
        break;
    case SDLK_SPACE:
        if (_player.ctrl->jumpControl == 0 || _player.ctrl->jumpControl == 2) {
            ++_player.ctrl->jumpControl;
            return true;
        }
        break;
    case SDLK_ESCAPE:
        quit(false);
        return true;
    }

    return false;
}

bool Engine::keyUp(int key, int /*mods*/) {

    switch (key) {
    case SDLK_w:
        if (_player.ctrl) {
            _player.ctrl->walkControl.z = 0.0f;
            return true;
        }
        break;
    case SDLK_a:
        if (_player.ctrl) {
            _player.ctrl->walkControl.x = 0.0f;
            return true;
        }
        break;
    case SDLK_s:
        if (_player.ctrl) {
            _player.ctrl->walkControl.z = 0.0f;
            return true;
        }
        break;
    case SDLK_d:
        if (_player.ctrl) {
            _player.ctrl->walkControl.x = 0.0f;
            return true;
        }
        break;
    }
    return false;
}

void Engine::inputChanged(bool visible, const char * /*value*/) {
    gui.setConsoleText(visible ? _console.commandLine : nullptr);
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
            _editor.addNode(gpu::createNode(*node->libraryNode));
            return true;
        }
    }
    return false;
}

bool Engine::setNodeTranslation(const glm::vec3 &v) { return _editor.translateSelected(v); }
bool Engine::setNodeRotation(const glm::quat &q) { return _editor.rotateSelected(q); }
bool Engine::setNodeScale(const glm::vec3 &v) { return _editor.scaleSelected(v); }

bool Engine::applyNodeTranslation(const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        return _editor.translateSelected(sel->translation.data() + v);
    }
    return false;
}
bool Engine::applyNodeRotation(const glm::quat &q) {
    if (auto sel = _editor.selectedNode()) {
        return _editor.rotateSelected(sel->rotation.data() * q);
    }
    return false;
}
bool Engine::applyNodeScale(const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        return _editor.scaleSelected(sel->scale.data() + v);
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

bool Engine::openSaveFile(const char *fpath) {
    for (auto &saveFile : _saveFiles) {
        if (strcmp(saveFile.path, fpath) == 0) {
            _openSaveFile(saveFile);
            return true;
        }
    }
    if (strlen(fpath) > 0) {
        auto &saveFile = _saveFiles.emplace_back();
        strcpy(saveFile.path, fpath);
        loadWorld(fpath, saveFile);
        _openSaveFile(saveFile);
        assignPanel(Panel::SAVE_FILE, (void *)&saveFile);
        return true;
    }
    return false;
}
bool Engine::saveSaveFile(const char *fpath) {
    if (_saveFile == nullptr) {
        return false;
    }
    const char *path = (fpath != nullptr && strlen(fpath) > 0) ? fpath : _saveFile->path;
    if (path) {
        persist::saveWorld(path, *_saveFile);
        _saveFile->dirty = false;
        _editor.clearHistory();
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
        {0.0f, 0.0f, 0.0f}, glm::radians(90.0f), glm::radians(15.0f), 10.0f};
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
    if (!_playing) {
        if (node) {
            _editor.selectNode(node);
            return true;
        }
    }
    return false;
}

void Engine::quit(bool force) {
    if (_saveFile && (_saveFile->dirty || _editor.hasHistory()) && !force) {
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
        const char *sceneName = "noscene";
        if (node->libraryNode && node->libraryNode->scene) {
            sceneName = node->libraryNode->scene->name.c_str();
        }
        static char componentInfo[25];

        if (auto entity = (ecs::Entity *)node->extra) {
            for (size_t i{0}; i < 24; ++i) {
                componentInfo[23 - i] = ((1 << i) & *entity) ? '1' : '0';
            }
        } else {
            std::memset(componentInfo, '0', 24);
        }
        componentInfo[24] = '\0';
        gui.setNodeInfo(sceneName, node->name().c_str(),
                        node->mesh ? node->meshName().c_str() : "-", componentInfo,
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
        if (node->libraryNode) {
            _editor.addNode(gpu::createNode(*node->libraryNode));
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
    printf("%s%s", tab.c_str(), node->libraryNode->name.c_str());
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

bool Engine::saveNodeInfo(gpu::Node *node, uint32_t &info) {
    if (ecs::Entity *e = (ecs::Entity *)node->extra) {
        info |= *e;
    }
    return true;
}
bool Engine::saveNodeExtra(gpu::Node *node, uint32_t &extra) {
    if (ecs::Entity *e = (ecs::Entity *)node->extra) {
        if (auto *collider = CCollider::get_pointer(e)) {
            extra |= ((0b1 & collider->transforming) << 4) |
                     ((uint8_t)collider->geometry->type() & 0b111);
        }
    }
    return true;
}
gpu::Scene *Engine::loadedScene(const char *name) {
    if (auto collection = findCollection(name)) {
        return collection->scene;
    }
    return nullptr;
}

bool Engine::loadedEntity(ecs::Entity *entity, gpu::Node *node, uint32_t info, uint32_t extra) {
    *entity = info;
    auto geomType = static_cast<geom::Geometry::Type>(extra & 0b111);
    if (CController::has(entity)) {
        _attachController(node, geomType);
    } else if (CCollider::has(entity)) {
        _attachCollider(node, geomType, CActor::has(entity));
    }
    return true;
}