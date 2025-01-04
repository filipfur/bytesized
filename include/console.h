#pragma once

#include "window.h"
#include <functional>
#include <glm/glm.hpp>
#include <list>
#include <string>

struct IConsole {
    virtual void inputChanged(bool /*visible*/, const char * /*text*/) = 0;
    virtual bool openSaveFile(const char * /*name*/) { return false; }
    virtual bool saveSaveFile(const char * /*name*/) { return false; }
    virtual bool openScene(size_t /*index*/) { return false; }
    virtual bool openScene(const char * /*name*/) { return false; }
    virtual bool selectNode(size_t /*index*/) { return false; }
    virtual void listNodes() {}
    virtual bool spawnNode(const char * /*name*/) { return false; }
    virtual bool setNodeTranslation(const glm::vec3 &) { return false; }
    virtual bool setNodeRotation(const glm::quat &) { return false; }
    virtual bool setNodeScale(const glm::vec3 &) { return false; }
    virtual bool applyNodeTranslation(const glm::vec3 &) { return false; }
    virtual bool applyNodeRotation(const glm::quat &) { return false; }
    virtual bool applyNodeScale(const glm::vec3 &) { return false; }
    virtual void quit(bool /*force*/) {}
};

struct Console : public window::IKeyListener, window::ITextListener {

    using Callback = std::function<bool(const char *key)>;
    struct CustomCommand {
        const char *key;
        Callback callback;
    };

    struct Setting {
        char key[32];
        char value[64];
    };

    Console() {
        window::registerKeyListener(this);
        window::registerTextListener(this);
    }

    virtual bool keyDown(int key, int mods) override;
    virtual bool keyUp(int key, int mods) override;
    virtual bool textInput(const char *text) override;

    void addCustomCommand(const char *key, const Callback &callback);

    void setSetting(const char *key, const char *defaultValue);

    bool settingBool(const char *key);
    int settingInt(const char *key);
    float settingFloat(const char *key);
    const char *settingString(const char *key);

    void registerIConsole(IConsole *iConsole);

    bool evaluate();

    IConsole *_iConsole{nullptr};
    char commandLine[256] = {};
    std::list<std::string> history;
    std::list<std::string>::reverse_iterator historyIt;
    uint8_t cursor{0};
    bool visible{false};
    static const size_t CUSTOM_COMMAND_MAX{50};
    CustomCommand customCommands[CUSTOM_COMMAND_MAX] = {};
    static const size_t SETTINGS_MAX{50};
    Setting settings[SETTINGS_MAX] = {};
};
