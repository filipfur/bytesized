#include "engine.h"
#include "builtin_shader.h"
#include "character.h"
#include "component.h"
#include "dexterity.h"
#include "ecs.h"
#include "embed/boxxy_bdf.hpp"
#include "geom_primitive.h"
#include "gpu.h"
#include "gpu_primitive.h"
#include "gui.h"
#include "persist.h"
#include "playercontroller.h"
#include "primer.h"
#include "skydome.h"
#include <algorithm>
#include <list>

gpu::Collection *Engine::addCollection(const library::Collection &collection) {
    return &_collections.emplace_back(collection);
}
gpu::Collection *Engine::addGLB(const uint8_t *data) {
    return addCollection(*library::loadGLB(data, true));
}

static gpu::Framebuffer *fbo{nullptr};
static persist::SessionData sessionData;
static gpu::Node *gridNode{nullptr};
constexpr gpu::Plane<1, 1> grid(64.0f);
Panel *_panel{nullptr};
Panel *_previousPanel{nullptr};

static Vector axisVectors[] = {
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, rgb(255, 0, 0)},
    {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, rgb(0, 255, 0)},
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, rgb(0, 0, 255)},
};

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

void Engine::attachCollider(gpu::Node *node, geom::Geometry::Type type, bool dynamic) {
    ecs::Entity *entity = node->entity;
    if (entity == nullptr) {
        entity = ecs::create_entity();
        node->entity = entity;
    }
    if (dynamic) {
        ecs::attach<CActor, CCollider, CGravity>(entity);
        auto &actor = CActor::get(entity);
        actor.trs = node;
        actor.mass = 1.0f;
        actor.restitution = 0.5f;
        CGravity::get(entity).factor = 1.0f;
    } else {
        ecs::attach<CPawn, CCollider>(entity);
        CPawn::get(entity).trs = node;
    }
    if (auto *n = node->find([](gpu::Node *n) { return n->mesh && n->mesh->libraryMesh; })) {
        for (gpu::Node *child : n->children) {
            if (child->libraryNode->name.compare(0, 3, "C__") == 0) {
                n = child;
                break;
            }
        }
        auto [positions, length] = n->mesh->libraryMesh->primitives.front().positions();
        auto &colider = CCollider::get(entity);
        if (n->libraryNode->name.starts_with("C__Sphere")) {
            type = geom::Geometry::Type::SPHERE;
            n->hidden = true;
        }
        if (n->libraryNode->name.starts_with("C__Box")) {
            type = (glm::dot(node->euler.data(), node->euler.data()) > primer::EPSILON)
                       ? geom::Geometry::Type::OBB
                       : geom::Geometry::Type::AABB;
            n->hidden = true;
        }
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
            if (auto actor = CActor::get_pointer(entity)) {
                float r = (sphere->_radii * node->s().x);
                actor->mass = r * r * r * 1000.0f;
            }
        } break;
        case geom::Geometry::Type::AABB: {
            auto *aabb = geom::createAABB();
            aabb->construct(positions, length, dynamic ? nullptr : node);
            colider.geometry = aabb;
            colider.transforming = dynamic;
        } break;
        case geom::Geometry::Type::OBB: {
            auto *obb = geom::createOBB();
            obb->construct(positions, length, nullptr); // dynamic ? nullptr : node);
            colider.geometry = obb;
            colider.transforming = true;
        } break;
        default:
        }
    } else {
        node->hidden = true;
        auto &collider = CCollider::get(node->entity);
        geom::AABB *aabb;
        geom::OBB *obb;
        switch (type) {
        case geom::Geometry::Type::AABB:
            aabb = geom::createAABB();
            aabb->construct(glm::vec3{-1.0f, -1.0f, -1.0f} * node->s() + node->t(),
                            glm::vec3{1.0f, 1.0f, 1.0f} * node->s() + node->t());
            collider.geometry = aabb;
            collider.transforming = false;
            break;
        case geom::Geometry::Type::OBB:
            obb = geom::createOBB();
            obb->construct(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{1.0f, 1.0f, 1.0f}, node);
            collider.geometry = obb;
            collider.transforming = true;
            break;
        default:
            assert(false); // not implemented
            break;
        }
    }
}

void Engine::attachController(gpu::Node *node, geom::Geometry::Type type) {
    attachCollider(node, type, true);
    CController::attach(node->entity);
    auto &actor = CActor::get(node->entity);
    actor.restitution = 0.5f;
    auto &collider = CCollider::get(node->entity);
    auto &ctrl = CController::get(node->entity);
    geom::Sphere *s1 = dynamic_cast<geom::Sphere *>(collider.geometry);
    geom::Sphere *s2 = geom::createSphere();
    ctrl.floorSense = &s2->construct(s1->_center, s1->_radii + 0.02f);
    ctrl.movement = 0x0;
    ctrl.state = Controller::STATE_FALLING;
    ctrl.walkSpeed = 6.0f;
    ctrl.jumpTimer = 0;
    ctrl.jumpCount = 2;
}

static const char *_clickNPickFrag = BYTESIZED_GLSL_VERSION R"(
precision highp float;
out vec4 FragColor;
in vec2 UV;
uniform float u_object_id;
void main()
{ 
    FragColor.r = u_object_id / 255.0;
}
)";

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

    gpu::createBuiltinUBOs();

    auto builtinGeoms = _collections.emplace_back(*gpu::createBuiltinPrimitives());

    font = bdf::createFont((const char *)_embed_boxxy_bdf, sizeof(_embed_boxxy_bdf));
    gui.create(font, _windowWidth, _windowHeight, 2.0f, GUI::EVERYTHING);

    const glm::vec4 defaultColor = Color::green.vec4();

    shaderProgram = gpu::createShaderProgram(
        builtin::shader(builtin::OBJECT_VERT), builtin::shader(builtin::OBJECT_FRAG),
        {{"u_color", defaultColor}, {"u_diffuse", 0}, {"u_model", glm::mat4{1.0f}}});

    billboardProgram = gpu::createShaderProgram(
        builtin::shader(builtin::BILLBOARD_VERT), builtin::shader(builtin::OBJECT_FRAG),
        {{"u_color", defaultColor}, {"u_diffuse", 0}, {"u_model", glm::mat4{1.0f}}});

    animProgram = gpu::createShaderProgram(builtin::shader(builtin::ANIM_VERT),
                                           builtin::shader(builtin::OBJECT_FRAG),
                                           {//{"u_projection", perspectiveProjection},
                                            //{"u_view", glm::mat4{1.0f}},
                                            {"u_color", defaultColor},
                                            {"u_diffuse", 0},
                                            {"u_model", glm::mat4{1.0f}}});

    const Color bgColor(0xe0f8d0);
    const Color textColor(0x081820);
    textProgram = gpu::createShaderProgram(builtin::shader(builtin::TEXT_VERT),
                                           builtin::shader(builtin::TEXT_FRAG),
                                           {{"u_bg_color", bgColor.vec4()},
                                            {"u_text_color", textColor.vec4()},
                                            {"u_diffuse", 0},
                                            {"u_model", glm::mat4{1.0f}},
                                            {"u_metallic", 1.0f},
                                            {"u_time", 0.0f}});

    gpu::Shader *clickNPickShader = gpu::createShader(GL_FRAGMENT_SHADER, _clickNPickFrag);
    clickProgram = gpu::createShaderProgram(builtin::shader(builtin::OBJECT_VERT), clickNPickShader,
                                            {{"u_model", glm::mat4{1.0f}}, {"u_object_id", 0.0f}});
    clickAnimProgram =
        gpu::createShaderProgram(builtin::shader(builtin::ANIM_VERT), clickNPickShader,
                                 {{"u_model", glm::mat4{1.0f}}, {"u_object_id", 0.0f}});
    gpu::builtinUBO(gpu::UBO_CAMERA)
        ->bindShaders({
            shaderProgram,
            billboardProgram,
            animProgram,
            textProgram,
            clickProgram,
            clickAnimProgram,
        });
    gpu::builtinUBO(gpu::UBO_LIGHT)
        ->bindShaders({
            shaderProgram,
            billboardProgram,
            animProgram,
            textProgram,
        });
    gpu::builtinUBO(gpu::UBO_SKINNING)->bindShaders({animProgram, clickAnimProgram});

    uiProgram = gpu::createShaderProgram(builtin::shader(builtin::UI_VERT),
                                         builtin::shader(builtin::TEXTURE_FRAG),
                                         {{"u_projection", gui.projection},
                                          {"u_view", gui.view},
                                          {"u_diffuse", 0},
                                          {"u_model", glm::mat4{1.0f}}});

    uiTextProgram = gpu::createShaderProgram(builtin::shader(builtin::UI_VERT),
                                             builtin::shader(builtin::TEXT_FRAG),
                                             {{"u_projection", gui.projection},
                                              {"u_view", gui.view},
                                              {"u_bg_color", bgColor.vec4()},
                                              {"u_text_color", textColor.vec4()},
                                              {"u_diffuse", 0},
                                              {"u_model", glm::mat4{1.0f}},
                                              {"u_metallic", 0.0f},
                                              {"u_time", 0.0f}});

    skydome::create();
    screenProgram =
        gpu::createShaderProgram(builtin::shader(builtin::SCREEN_VERT),
                                 builtin::shader(builtin::TEXTURE_FRAG), {{"u_texture", 0}});
    fbo = gpu::createFramebuffer();
    fbo->bind();
    fbo->createTexture(GL_COLOR_ATTACHMENT0, _windowWidth * 2, _windowHeight * 2,
                       gpu::ChannelSetting::RGB, GL_UNSIGNED_BYTE);
    fbo->createDepthStencil(_windowWidth * 2, _windowHeight * 2);
    fbo->checkStatus("engine.fbo");
    fbo->unbind();
    _clickNPick.create(_windowWidth, _windowHeight, perspectiveProjection, 0.1f, clickProgram);
    _clickNPick.shaderPrograms.push_back(clickAnimProgram);

    gridNode = gpu::createNode();
    gridNode->hidden = true;
    gridNode->mesh = gpu::createMesh();
    auto gridPrim = gpu::createPrimitive(grid.positions, grid.normals, grid.uvs, grid.CELLS,
                                         grid.indices, grid.CELLS * 6);
    gridNode->mesh->primitives.emplace_back(gridPrim, gpu::builtinMaterial(gpu::GRID_TILE));
    gridNode->translation = {0.0f, 0.01f, 0.0f};
    gridNode->rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
    gridNode->scale = glm::vec3{64.0f, 64.0f, 64.0f};

    gpu::CameraBlock_setProjection(perspectiveProjection);

    _console.setSetting("wiremode", "0");
    _console.setSetting("tstep", "0");
    _console.setSetting("rstep", "0");
    _console.addCustomCommand(":static ", [this](const char *key) {
        if (auto sel = _editor.selectedNode()) {
            attachCollider(sel, _parseGeometryType(key + strlen(":static ")), false);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":dynamic ", [this](const char *key) {
        if (auto sel = _editor.selectedNode()) {
            attachCollider(sel, _parseGeometryType(key + strlen(":dynamic ")), true);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":spyro", [this](const char * /*key*/) {
        if (auto sel = _editor.selectedNode()) {
            attachController(sel, geom::Geometry::Type::SPHERE);
            nodeSelected(sel);
            return true;
        }
        return false;
    });
    _console.addCustomCommand(":c.pitch=", [this](const char *cmd) {
        _camera.targetView.pitch = std::atof(cmd + strlen(":c.pitch="));
        return true;
    });
    _console.addCustomCommand(":c.yaw=", [this](const char *cmd) {
        _camera.targetView.yaw = std::atof(cmd + strlen(":c.yaw="));
        return true;
    });
    _console.addCustomCommand(":c.distance=", [this](const char *cmd) {
        _camera.targetView.distance = std::atof(cmd + strlen(":c.distance="));
        return true;
    });
    _console.addCustomCommand(":c.origo", [this](const char *cmd) {
        _camera.targetView.center = glm::vec3(0.0f);
        return true;
    });
    _console.addCustomCommand(":c.x=", [this](const char *cmd) {
        _camera.targetView.center.x = std::atof(cmd + strlen(":c.x="));
        return true;
    });
    _console.addCustomCommand(":c.y=", [this](const char *cmd) {
        _camera.targetView.center.y = std::atof(cmd + strlen(":c.y="));
        return true;
    });
    _console.addCustomCommand(":c.z=", [this](const char *cmd) {
        _camera.targetView.center.z = std::atof(cmd + strlen(":c.z="));
        return true;
    });
    _console.addCustomCommand(":viz", [this](const char * /*key*/) {
        if (auto sel = _editor.selectedNode()) {
            auto n = sel->find([](gpu::Node *node) { return node->entity; });
            if (auto entity = n->entity) {
                if (auto collider = CCollider::get_pointer(entity)) {
                    vectors.clear();
                    collider->geometry->trs = collider->transforming ? sel : nullptr;
                    switch (collider->geometry->type()) {
                    case geom::Geometry::Type::PLANE: {
                        auto plane = (geom::Plane *)collider->geometry;
                        vectors.emplace_back(plane->origin(), plane->normal, Color::yellow);
                    }
                        return true;
                    case geom::Geometry::Type::SPHERE: {
                        auto sphere = (geom::Sphere *)collider->geometry;
                        glm::vec3 r = glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f}) * sphere->radii();
                        vectors.emplace_back(sphere->origin(), r, Color::yellow);
                        vectors.emplace_back(sphere->origin(), -r, Color::red);
                    }
                        return true;
                    case geom::Geometry::Type::AABB: {
                        auto aabb = (geom::AABB *)collider->geometry;
                        vectors.emplace_back(aabb->origin(), aabb->extents(), Color::yellow);
                        vectors.emplace_back(aabb->origin(), -aabb->extents(), Color::red);
                    }
                        return true;
                    case geom::Geometry::Type::OBB: {
                    } break;
                    }
                }
            }
        }
        return false;
    });
    if (_iEngineApp) {
        _iEngineApp->appInit(this);
    }
    _openPanel(Panel_assign(Panel::SAVE_FILE, (void *)&_saveFile));
}

bool Engine::update(float dt) {
    if (_iGame) {
        _iGame->gameUpdate(dt);
    } else {
        _editor.setTStep(_console.settingFloat("tstep"));
        _editor.setRStep(_console.settingFloat("rstep"));
        if (_iEngineApp) {
            _iEngineApp->appUpdate(dt);
        }
        for (size_t i{0}; i < 3; ++i) {
            auto &vec = axisVectors[i];
            if (auto node = _editor.selectedNode()) {
                vec.O = node->translation.data();
                vec.Ray = node->r() * primer::AXES[i];
                vec.hidden = false;
            } else {
                vec.hidden = true;
            }
        }
    }
    _camera.update(dt);
    gpu::animate(dt);
    return true;
}

void Engine::resize(int drawableWidth, int drawableHeight) {
    _drawableWidth = drawableWidth;
    _drawableHeight = drawableHeight;
}

void Engine::startGame(IGame *iGame) {
    _iGame = iGame;
    _iGame->gameInit();
    _editor.selectNode(nullptr);
    _editor.disable(true);
}

void Engine::endGame() {
    _iGame = nullptr;
    _editor.disable(false);
}

static void _renderNodes(Editor &editor, gpu::ShaderProgram *shaderProgram,
                         std::vector<gpu::Node *> &nodes, bool skinned) {
    for (auto *node : nodes) {
        if ((node->skin == nullptr) == skinned) {
            continue;
        }
        if (CBillboard::has(node->entity)) {
            continue;
        }
        if (node == editor.selectedNode()) {
            auto mat = gpu::builtinMaterial(gpu::BuiltinMaterial::WHITE);
            mat->color = Color(0x44AAFF) * Color::opacity(0.4f);
            gpu::setOverrideMaterial(mat);
            node->render(shaderProgram);
            mat->color = Color(0x44FFAA) * Color::opacity(0.7f);
            node->recursive([](gpu::Node *n) { n->wireframe = true; });
            node->render(shaderProgram);
            node->recursive([](gpu::Node *n) { n->wireframe = false; });
            gpu::setOverrideMaterial(nullptr);
            continue;
        }
        if (auto entity = node->entity) {
            if (Controller *ctrl = CController::get_pointer(entity)) {
                static gpu::Material *ctrlMat =
                    gpu::createMaterial(gpu::builtinMaterial(gpu::WHITE)->textures.at(GL_TEXTURE0));
                auto meshNode = node->find([](gpu::Node *node) { return node->mesh; });
                meshNode->mesh->primitives.front().second = ctrlMat;
                meshNode->material()->color =
                    (ctrl && ctrl->state == Controller::STATE_ON_GROUND) ? Color::blue : Color::red;
            }
        }
        node->render(shaderProgram);
    }
}

void Engine::draw() {
    const glm::mat4 &view = _camera.view();

    gpu::CameraBlock_setViewPos(view, _camera.currentView.center -
                                          _camera.orientation() * _camera.currentView.distance);

    if (_iGame) {
        _iGame->gameDraw();
    } else {
        float t = Time::seconds();
        _clickNPick.update();

        fbo->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glStencilMask(0x00);
        // glViewport(0.0f, 0.0f, window::DRAWABLE_WIDTH * 0.5f, window::DRAWABLE_HEIGHT);

        if (_console.settingBool("wiremode")) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        if (_panel) {
            if (_panel->type == Panel::SAVE_FILE) {
                shaderProgram->use();
                _renderNodes(_editor, shaderProgram, _saveFile.nodes, false);

                billboardProgram->use();
                for (gpu::Node *node : _saveFile.nodes) {
                    if (auto *billboard = CBillboard::get_pointer(node->entity)) {
                        billboardProgram->uniforms.at("u_color") << Color::white.vec4();
                        billboardProgram->uniforms.at("u_model") << node->model();
                        glActiveTexture(GL_TEXTURE0);
                        billboard->texture->bind();
                        node->primitive()->render();
                    }
                }

                animProgram->use();
                // animProgram->uniforms.at("u_view") << view;
                _renderNodes(_editor, animProgram, _saveFile.nodes, true);
            } else {
                // render objects
                shaderProgram->use();
                _renderNodes(_editor, shaderProgram, nodes, false);

                // render skeletal animations
                animProgram->use();
                // animProgram->uniforms.at("u_view") << view;
                _renderNodes(_editor, animProgram, skinNodes, true);

                // render texts
                textProgram->use();
                textProgram->uniforms.at("u_time") << t;
                for (gpu::Node *node : textNodes) {
                    node->render(textProgram);
                }
            }
        }

        if (_iEngineApp) {
            _iEngineApp->appDraw();
        }

        if (_console.settingBool("wiremode")) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        shaderProgram->use();

        glDepthFunc(GL_LEQUAL);
        gridNode->render(shaderProgram);
        skydome::render();
        glDepthFunc(GL_LESS);

        glDisable(GL_DEPTH_TEST);
        for (auto &vector : axisVectors) {
            gpu::renderVector(shaderProgram, vector);
        }
        for (auto &vector : vectors) {
            gpu::renderVector(shaderProgram, vector);
        }

        glEnable(GL_DEPTH_TEST);

        gui.render(uiProgram, uiTextProgram);

        fbo->unbind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        fbo->textures.at(GL_COLOR_ATTACHMENT0)->bind();

        screenProgram->use();
        gpu::renderScreen();
    }
}

static void _changePanel(Panel *panel, Camera &camera) {
    if (_panel) {
        _panel->cameraView = camera.targetView;
        _previousPanel = _panel;
    }
    _panel = panel;
    camera.set(panel->cameraView);
}

bool Engine::keyDown(int key, int mods) {
    switch (key) {
    case SDLK_0:
        break;
    case SDLK_1:
    case SDLK_2:
    case SDLK_3:
        if (mods & KMOD_SHIFT) {
            if (_previousPanel) {
                _changePanel(_previousPanel, _camera);
                return true;
            }
            return false;
        }
    case SDLK_4:
    case SDLK_5:
    case SDLK_6:
    case SDLK_7:
    case SDLK_8:
    case SDLK_9:
        return _openPanel(Panel_byIndex(key - SDLK_1));
    case SDLK_g:
        if (mods & KMOD_SHIFT) {
            gridNode->hidden = !gridNode->hidden;
            return true;
        }
        break;
    case SDLK_s:
        if (mods & KMOD_SHIFT) {
            if (_snapping) {
                _console.setSetting("tstep", "0");
                _console.setSetting("rstep", "0");
            } else {
                _console.setSetting("tstep", "1");
                _console.setSetting("rstep", "15");
            }
            _snapping = !_snapping;
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
    default:
        return false;
    }
    return true;
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
    if (_panel->type != Panel::SAVE_FILE) {
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

bool Engine::setNodeTranslation(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 trans = sel->translation.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                trans[i] = v[i];
            }
        }
        return _editor.translateSelected(trans);
    }
    return false;
}
bool Engine::setNodeRotation(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 rot = sel->euler.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                rot[i] = v[i];
            }
        }
        return _editor.rotateSelected(rot);
    }
    return false;
}
bool Engine::setNodeScale(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 scale = sel->scale.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                scale[i] = v[i];
            }
        }
        return _editor.scaleSelected(scale);
    }
    return false;
}

bool Engine::applyNodeTranslation(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 trans = sel->translation.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                trans[i] += v[i];
            }
        }
        return _editor.translateSelected(trans);
    }
    return false;
}
bool Engine::applyNodeRotation(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 rot = sel->euler.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                rot[i] += v[i];
            }
        }
        return _editor.rotateSelected(rot);
    }
    return false;
}
bool Engine::applyNodeScale(const glm::ivec3 &mask, const glm::vec3 &v) {
    if (auto sel = _editor.selectedNode()) {
        glm::vec3 scale = sel->scale.data();
        for (size_t i{0}; i < 3; ++i) {
            if (mask[i]) {
                scale[i] += v[i];
            }
        }
        return _editor.scaleSelected(scale);
    }
    return false;
}

bool Engine::closeSaveFile() {
    if (!_saveFile.nodes.empty()) {
        _saveFile.dirty = false;
        for (gpu::Node *node : _saveFile.nodes) {
            __freeEntity(&node->entity);
            gpu::freeNode(node);
        }
        ecs::entities().clear();
        _editor.clearHistory();
        _saveFile.nodes.clear();
        return true;
    }
    return false;
}

bool Engine::openSaveFile(const char *fpath) {
    if (fpath == nullptr || strlen(fpath) == 0) {
        return false;
    }
    if (_saveFile.dirty) {
        return false;
    }
    closeSaveFile();
    strcpy(_saveFile.path, fpath);
    loadWorld(fpath, _saveFile);
    _editor.selectNode(nullptr);
    if (_iEngineApp) {
        _iEngineApp->appLoad();
    }
    _changePanel(Panel_byIndex(0), _camera);
    return true;
}
bool Engine::saveSaveFile(const char *fpath) {
    const char *path = (fpath != nullptr && strlen(fpath) > 0) ? fpath : _saveFile.path;
    if (path) {
        persist::saveWorld(path, _saveFile);
        _saveFile.dirty = false;
        _editor.clearHistory();
        openSaveFile(path);
        return true;
    }
    return false;
}

bool Engine::_openPanel(Panel *panel) {
    size_t index = Panel_index(panel);
    if (panel == nullptr) {
        printf("Panel %zu is not assigned\n", index);
        return false;
    }
    if (panel == _panel) {
        printf("Panel %zu already open\n", index);
        return false;
    }
    _editor.selectNode(nullptr);
    switch (panel->type) {
    case Panel::COLLECTION:
        gui.setTitleText(((gpu::Collection *)panel->data)->name().c_str());
        _openCollection(*((gpu::Collection *)panel->data));
        break;
    case Panel::SAVE_FILE:
        gui.setTitleText(_saveFile.path);
        openSaveFile(((persist::SaveFile *)panel->data)->path);
        break;
    default:
        return false;
    }
    _changePanel(panel, _camera);
    return true;
}

void Engine::_openCollection(const gpu::Collection &collection) {
    unstage();
    stage(*collection.scene);
    _editor.selectNode(nullptr);
}

bool Engine::openScene(size_t index) {
    if (index < _collections.size()) {
        auto it = _collections.begin();
        std::advance(it, index);
        _openCollection(*it);
        _openPanel(Panel_assign(Panel::COLLECTION, (void *)&(*it)));
        return true;
    }
    return false;
}

bool Engine::openScene(const char *name) {
    if (auto *collection = findCollection(name)) {
        _openCollection(*collection);
        _openPanel(Panel_assign(Panel::COLLECTION, (void *)collection));
        return true;
    }
    return false;
}

void Engine::renderClickables(gpu::ShaderProgram *sp, uint8_t &id) {
    if (sp == clickProgram) {
        for (gpu::Node *node : nodes) {
            sp->uniforms.at("u_object_id") << static_cast<float>(id++);
            node->render(sp);
        }
    } else if (sp == clickAnimProgram) {
        for (gpu::Node *skinnedNode : skinNodes) {
            sp->uniforms.at("u_object_id") << static_cast<float>(id++);
            skinnedNode->render(sp);
        }
    }
}

bool Engine::nodeClicked(size_t index) {
    size_t numNodes = nodes.size();
    printf("clicked index=%zu\n", index);
    if (index < numNodes) {
        _editor.selectNode(nodes[index]);
        return true;
    } else if (index < (numNodes + skinNodes.size())) {
        _editor.selectNode(skinNodes[index - numNodes]);
        return true;
    }
    return false;
}

void Engine::quit(bool force) {
    if ((_saveFile.dirty || _editor.hasHistory()) && !force) {
        return;
    }
    strcpy(sessionData.recentFile, _saveFile.path);
    sessionData.cameraView = _camera.targetView;
    persist::storeSession(sessionData);
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

static void _updateNodeInfo(GUI &gui, gpu::Node *node) {
    if (node) {
        const char *sceneName = "noscene";
        if (node->libraryNode && node->libraryNode->scene) {
            sceneName = node->libraryNode->scene->name.c_str();
        }
        static char componentInfo[33];

        uint32_t id = (uint32_t)(uint64_t)&node;
        if (auto entity = node->entity) {
            for (size_t i{0}; i < 32; ++i) {
                componentInfo[31 - i] = ((1 << i) & *entity) ? '1' : '0';
            }
            id = ecs::ID(entity);
        } else {
            std::memset(componentInfo, '0', 32);
        }
        componentInfo[32] = '\0';
        gui.setNodeInfo(sceneName, node->name().c_str(), id,
                        node->mesh ? node->meshName().c_str() : "-", componentInfo,
                        node->translation.data(), node->euler.data(), node->scale.data());
    }
    gui.showNodeInfo(node != nullptr);
}

void Engine::nodeSelected(gpu::Node *node) { _updateNodeInfo(gui, node); }

void Engine::nodeAdded(gpu::Node *node) {
    _saveFile.nodes.emplace_back(node);
    _saveFile.dirty = true;
    _editor.selectNode(node);
}

void Engine::nodeRemoved(gpu::Node *node) {
    _saveFile.nodes.erase(std::find(_saveFile.nodes.begin(), _saveFile.nodes.end(), node));
    _saveFile.dirty = true;
    _editor.selectNode(nullptr);
}

void Engine::nodeCopied(gpu::Node *node) {
    if (_panel->type == Panel::SAVE_FILE) {
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

static gpu::Node *_cycleNode(std::vector<gpu::Node *> nodes, gpu::Node *cur, bool reverse) {
    if (nodes.empty()) {
        return nullptr;
    }
    bool found{false};
    auto *trail = nodes.front();
    for (gpu::Node *node : nodes) {
        if (found) {
            return node;
        }
        found = (node == cur);
        if (reverse && found) {
            return trail;
        }
        trail = node;
    }
    return nullptr;
}

gpu::Node *Engine::cycleNode(gpu::Node *cur, bool reverse) {
    if (_panel == nullptr) {
        return nullptr;
    }
    if (_panel->type == Panel::SAVE_FILE) {
        if (cur) {
            return _cycleNode(_saveFile.nodes, cur, reverse);
        }
        return _saveFile.nodes.empty() ? nullptr : _saveFile.nodes.front();
    }
    if (cur) {
        if (gpu::Node *node = _cycleNode(nodes, cur, reverse)) {
            return node;
        }
        return _cycleNode(skinNodes, cur, reverse);
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
};

void Engine::stage(const gpu::Scene &scene) {
    for (gpu::Node *node : scene.nodes) {
        if (node->skin) {
            skinNodes.emplace_back(node);
        } else {
            nodes.emplace_back(node);
        }
    }
}
gpu::Collection *Engine::findCollection(const char *name) {
    for (auto &collection : _collections) {
        if (collection.name().compare(name) == 0) {
            return &collection;
        }
    }
    return nullptr;
}

bool Engine::saveNodeInfo(gpu::Node *node, uint32_t &info) {
    if (ecs::Entity *e = node->entity) {
        info |= *e;
    }
    return true;
}
bool Engine::saveNodeExtra(gpu::Node *node, uint32_t &extra) {
    if (ecs::Entity *e = node->entity) {
        if (auto *collider = CCollider::get_pointer(e)) {
            extra |= ((0b1 & collider->transforming) << 4) |
                     ((uint8_t)collider->geometry->type() & 0b111);
        }
        if (auto *billboard = CBillboard::get_pointer(e)) {
            extra |= billboard->id;
        }
    }
    return true;
}
gpu::Scene *Engine::getSceneByName(const char *name) {
    if (auto collection = findCollection(name)) {
        return collection->scene;
    }
    return nullptr;
}

bool Engine::loadEntity(ecs::Entity *entity, gpu::Node *node, uint32_t info, uint32_t extra) {
    *entity = info;
    auto geomType = static_cast<geom::Geometry::Type>(extra & 0b111);
    if (CController::has(entity)) {
        attachController(node, geomType);
    } else if (CCollider::has(entity)) {
        attachCollider(node, geomType, CActor::has(entity));
        if (geomType == geom::Geometry::Type::OBB) {
            auto obb = ((geom::OBB *)CCollider::get(entity).geometry);
            obb->trs = node;
        }
    }
    if (auto *pawn = CPawn::get_pointer(entity)) {
        pawn->trs = node;
    } else if (auto *actor = CActor::get_pointer(entity)) {
        actor->trs = node;
        actor->mass = 1.0f;
        actor->restitution = 0.5f;
        actor->velocity = {};
    }
    if (auto *billboard = CBillboard::get_pointer(entity)) {
        billboard->id = extra;
    }
    if (node->skin) {
        auto *collection = findCollection(node->libraryNode->scene->name.c_str());
        for (const auto &anim : collection->animations) {
            node->skin->animations.emplace_back(
                gpu::createAnimation(*anim->libraryAnimation, node));
        }
        node->skin->playback = gpu::createPlayback(node->skin->animations.front());
        // skinAnim.playAnimation("Idle");
    }
    return true;
}

void Engine::loadLastSession() {
    if (persist::loadSession(sessionData)) {
        if (strlen(sessionData.recentFile) > 0) {
            openSaveFile(sessionData.recentFile);
            sessionData.cameraView.yaw = primer::normalizedAngle(sessionData.cameraView.yaw);
            _camera.set(sessionData.cameraView);
        }
    }
}