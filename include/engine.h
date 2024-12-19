#pragma once

#include "assets.h"
#include "cameraview.h"
#include "clicknpick.h"
#include "console.h"
#include "editor.h"
#include "gui.h"
#include "persist.h"
#include "window.h"
#include <list>

struct IApplication {
    virtual void init(int drawableWidth, int drawableHeight) = 0;
    virtual bool update(float dt) = 0;
    // virtual void draw() = 0;
};

struct Panel {
    enum Type { UNUSED, COLLECTION, SAVE_FILE } type;
    void *ptr;
    CameraView cameraView;
};

struct Engine : public window::IEngine,
                public window::IKeyListener,
                public console::IConsole,
                public IEditor,
                public IClickNPick {

    Engine(Editor &editor_, console::Console &console_, int windowWidth, int windowHeight)
        : _clickNPick{this}, _editor{editor_}, _console{console_}, _windowWidth{windowWidth},
          _windowHeight{windowHeight} {
        _console.registerIConsole(this);
        _editor.registerIEditor(this);
        window::registerApplication(this);
        window::registerKeyListener(this);
    }

    void setIApplication(IApplication *app) { _app = app; }

    void init(int drawableWidth, int drawableHeight) override;
    bool update(float dt) override;
    void resize(int drawableWidth, int drawableHeight) override;
    void draw() override;

    bool keyDown(int key, int mods) override;
    bool keyUp(int key, int mods) override;

    void inputChanged(bool visible, const char *text) override;
    void fps(float fps) override;

    void listNodes() override;
    bool spawnNode(const char *name) override;
    bool openSaveFile(const char *name) override;
    bool saveSaveFile(const char *name) override;
    bool openScene(size_t index) override;
    bool openScene(const char *name) override;
    void quit(bool force) override;

    void nodeSelected(gpu::Node *node) override;
    void nodeAdded(gpu::Node *node) override;
    void nodeRemoved(gpu::Node *node) override;
    void nodeCopied(gpu::Node *node) override;
    void nodeTransformed(gpu::Node *node) override;
    gpu::Node *cycleNode(gpu::Node *prev) override;

    void unstage();
    void stage(const gpu::Scene &scene);

    assets::Collection *addCollection(const uint8_t *glbData);
    assets::Collection *findCollection(const char *name);

    bool assignPanel(Panel::Type type, void *ptr);
    bool openPanel(size_t index);
    void changePanel(Panel *panel);

    void _openCollection(const assets::Collection &collection);
    void _openSaveFile(persist::SaveFile &saveFile);

    bool nodeClicked(gpu::Node *node) override;

    ~Engine();

    ClickNPick _clickNPick;
    Editor &_editor;
    console::Console &_console;
    int _windowWidth;
    int _windowHeight;
    int _drawableWidth;
    int _drawableHeight;
    IApplication *_app{nullptr};
    gpu::ShaderProgram *shaderProgram;
    gpu::ShaderProgram *animProgram;
    gpu::ShaderProgram *textProgram;
    gpu::ShaderProgram *uiProgram;
    gpu::ShaderProgram *screenProgram;
    std::vector<gpu::Node *> nodes;
    std::vector<gpu::Node *> skinNodes;
    std::vector<gpu::Node *> textNodes;
    GUI gui;
    std::list<assets::Collection> _collections;
    std::list<persist::SaveFile> _saveFiles;
    persist::SaveFile *_saveFile{nullptr};
    Panel _panels[9];
    bool _blockMode{false};
};