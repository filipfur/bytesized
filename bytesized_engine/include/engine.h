#pragma once

#include "assets.h"
#include "camera.h"
#include "clicknpick.h"
#include "console.h"
#include "editor.h"
#include "geom_primitive.h"
#include "gui.h"
#include "panel.h"
#include "persist.h"
#include "window.h"
#include <list>

struct IGame {
    virtual void gameInit() = 0;
    virtual void gameUpdate(float dt) = 0;
    virtual void gameDraw() = 0;
};

struct IApplication {
    virtual void appInit(struct Engine *engine) = 0;
    virtual void appLoad(struct Engine *engine) = 0;
};

struct Engine : public window::IEngine,
                public window::IKeyListener,
                public IConsole,
                public IEditor,
                public IClickNPick,
                public persist::IPersist {

    Engine(Camera &camera_, Editor &editor_, Console &console_, int windowWidth, int windowHeight,
           IApplication *iApp)
        : _camera{camera_}, _editor{editor_}, _console{console_}, _clickNPick{this},
          _windowWidth{windowWidth}, _windowHeight{windowHeight}, _iApp{iApp} {
        _console.registerIConsole(this);
        _editor.registerIEditor(this);
        window::registerEngine(this);
        window::registerKeyListener(this);
        persist::registerIPersist(this);
    }

    void init(int drawableWidth, int drawableHeight) override;
    bool update(float dt) override;
    void resize(int drawableWidth, int drawableHeight) override;
    void draw() override;

    void startGame(IGame *iGame);
    void endGame();

    bool keyDown(int key, int mods) override;
    bool keyUp(int key, int mods) override;

    void inputChanged(bool visible, const char *text) override;
    void fps(float fps) override;

    void listNodes() override;
    bool spawnNode(const char *name) override;
    bool setNodeTranslation(const glm::ivec3 &, const glm::vec3 &) override;
    bool setNodeRotation(const glm::ivec3 &, const glm::vec3 &) override;
    bool setNodeScale(const glm::ivec3 &, const glm::vec3 &) override;
    bool applyNodeTranslation(const glm::ivec3 &, const glm::vec3 &) override;
    bool applyNodeRotation(const glm::ivec3 &, const glm::vec3 &) override;
    bool applyNodeScale(const glm::ivec3 &, const glm::vec3 &) override;
    bool closeSaveFile();
    bool openSaveFile(const char *fpath) override;
    bool saveSaveFile(const char *fpath) override;
    bool openScene(size_t index) override;
    bool openScene(const char *name) override;
    void quit(bool force) override;

    void nodeSelected(gpu::Node *node) override;
    void nodeAdded(gpu::Node *node) override;
    void nodeRemoved(gpu::Node *node) override;
    void nodeCopied(gpu::Node *node) override;
    void nodeTransformed(gpu::Node *node) override;
    gpu::Node *cycleNode(gpu::Node *prev, bool reverse) override;

    void attachCollider(gpu::Node *node, geom::Geometry::Type type, bool dynamic);
    void attachController(gpu::Node *node, geom::Geometry::Type type);

    void unstage();
    void stage(const gpu::Scene &scene);

    assets::Collection *addCollection(const library::Collection &collection);
    assets::Collection *addGLB(const uint8_t *data);
    assets::Collection *findCollection(const char *name);

    void _openCollection(const assets::Collection &collection);

    bool nodeClicked(gpu::Node *node) override;

    bool saveNodeInfo(gpu::Node *node, uint32_t &info) override;
    bool saveNodeExtra(gpu::Node *node, uint32_t &extra) override;
    gpu::Scene *getSceneByName(const char *name) override;
    bool loadEntity(ecs::Entity *entity, gpu::Node *node, uint32_t info, uint32_t extra) override;

    void loadLastSession();

    ~Engine();

    Camera &_camera;
    Editor &_editor;
    Console &_console;
    ClickNPick _clickNPick;
    int _windowWidth;
    int _windowHeight;
    int _drawableWidth;
    int _drawableHeight;
    IApplication *_iApp{nullptr};
    IGame *_iGame{nullptr};
    gpu::ShaderProgram *shaderProgram;
    gpu::ShaderProgram *billboardProgram;
    gpu::ShaderProgram *animProgram;
    gpu::ShaderProgram *textProgram;
    gpu::ShaderProgram *uiProgram;
    gpu::ShaderProgram *uiTextProgram;
    gpu::ShaderProgram *screenProgram;
    std::vector<gpu::Node *> nodes;
    std::vector<gpu::Node *> skinNodes;
    std::vector<gpu::Node *> textNodes;
    GUI gui;
    std::list<assets::Collection> _collections;
    persist::SaveFile _saveFile;
    bool _snapping{false};
    glm::mat4 perspectiveProjection;
    std::list<Vector> vectors;

  private:
    bool _openPanel(Panel *panel);
};