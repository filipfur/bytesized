#include "console.h"
#include "gpu.h"
#include "logging.h"
#include "recycler.hpp"
#include <optional>

enum TransformParameter {
    X = (1 << 0),
    Y = (1 << 1),
    Z = (1 << 2),
};

enum ColorParameter {
    R = (1 << 0),
    G = (1 << 1),
    B = (1 << 2),
    A = (1 << 2),
};

static bool starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

#ifdef DEXTERITY
static const char *trim(const char *str) {
    size_t i{0};
    while (str[i] != '\0') {
        if (!isspace(str[i])) {
            return str + i;
        }
        ++i;
    }
    return str;
}
#endif

console::Setting *_findSetting(console::Setting *settings, const char *key) {
    for (size_t i{0}; i < console::Console::SETTINGS_MAX; ++i) {
        if (settings[i].key[0] == '\0') {
            break;
        }
        if (strcmp(settings[i].key, key) == 0) {
            return settings + i;
        }
    }
    return nullptr;
}

bool console::Console::evaluate() {
    if (starts_with(commandLine, ":set ")) {
        static char key[32];
        char *par = commandLine + strlen(":set ");
        size_t i{1};
        for (; i < 100; ++i) {
            if (par[i] == ' ' || par[i] == '=' || par[i] == '\0') {
                break;
            }
        }
        strncpy(key, par, i);
        key[i] = '\0';
        if (auto set = _findSetting(settings, key)) {
            strcpy(set->value, par + i + 1);
        } else {
            LOG_INFO("Failed to find setting: %s", key);
        }
    } else if (starts_with(commandLine, ":list nodes")) {
        iConsole->listNodes();
        return true;
    } else if (starts_with(commandLine, ":f ")) {
        return iConsole->openSaveFile(commandLine + strlen(":f "));
    } else if (starts_with(commandLine, ":w ")) {
        return iConsole->saveSaveFile(commandLine + strlen(":w "));

    } else if (starts_with(commandLine, ":wq")) {
        if (iConsole->saveSaveFile(nullptr)) {
            iConsole->quit(false);
        }
    } else if (starts_with(commandLine, ":q!")) {
        iConsole->quit(true);
    } else if (starts_with(commandLine, ":q")) {
        iConsole->quit(false);
    } else if (starts_with(commandLine, ":w")) {
        return iConsole->saveSaveFile(nullptr);
    } else if (starts_with(commandLine, ":spawn ")) {
        return iConsole->spawnNode(commandLine + strlen(":spawn "));
    } else if (starts_with(commandLine, ":o ")) {
        const char *par = commandLine + strlen(":o ");
        if (isdigit(par[0])) {
            return iConsole->openScene(atoi(par));
        } else {
            return iConsole->openScene(par);
        }
    } else {
        for (const auto &command : customCommands) {
            if (command.key == nullptr) {
                break;
            }
            if (starts_with(commandLine, command.key)) {
                command.callback(command.key);
                break;
            }
        }
    }
    return false;
}

bool console::Console::keyDown(int key, int mods) {
    if (visible) {
        if (key == SDLK_RETURN) {
            evaluate();
            visible = false;
            auto it = std::find(history.begin(), history.end(), std::string{commandLine});
            if (it == history.end()) {
                history.emplace_back(commandLine);
            } else {
                std::iter_swap(it, history.rbegin());
            }
            SDL_StopTextInput();
        } else if (key == SDLK_ESCAPE) {
            visible = false;
            SDL_StopTextInput();
        } else if (key == SDLK_BACKSPACE) {
            if (cursor < 2) {
                visible = false;
                SDL_StopTextInput();
            } else {
                commandLine[--cursor] = '\0';
            }
        } else if (key == SDLK_UP) {
            if (!history.empty()) {
                if (historyIt == history.rend()) {
                    historyIt = history.rbegin();
                } else {
                    ++historyIt;
                    if (historyIt == history.rend()) {
                        --historyIt;
                    }
                }
                if (historyIt != history.rend()) {
                    strcpy(commandLine, historyIt->c_str());
                    cursor = historyIt->length();
                    iConsole->inputChanged(true, commandLine);
                }
            }
        } else if (key == SDLK_DOWN) {
            if (historyIt != history.rend()) {
                if (historyIt != history.rbegin()) {
                    --historyIt;
                }
                strcpy(commandLine, historyIt->c_str());
                cursor = historyIt->length();
                iConsole->inputChanged(true, commandLine);
            }
        }
        iConsole->inputChanged(visible, commandLine);
        return true;
    } else {
        if (key == SDLK_PERIOD) {
            if ((mods & KMOD_SHIFT)) {
                visible = true;
                SDL_StartTextInput();
                cursor = 0;
                std::memset(commandLine, 0, sizeof(commandLine));
                commandLine[cursor++] = ':';
                iConsole->inputChanged(true, commandLine);
                historyIt = history.rend();
            } else {
                if (cursor > 0) {
                    evaluate();
                }
            }
            return true;
        } else if (key == SDLK_UP) {
            if (!history.empty()) {
                visible = true;
                SDL_StartTextInput();
                historyIt = history.rbegin();
                strcpy(commandLine, historyIt->c_str());
                cursor = historyIt->length();
                iConsole->inputChanged(true, commandLine);
            }
        }
    }
    return false;
}

bool console::Console::keyUp(int /*key*/, int /*mods*/) { return false; }

bool console::Console::textInput(const char *text) {
    if (visible) {
        commandLine[cursor++] = *text;
        iConsole->inputChanged(true, text);
    }
    return false;
}

void console::Console::addCustomCommand(const char *key, const Callback &callback) {
    for (size_t i{0}; i < CUSTOM_COMMAND_MAX; ++i) {
        if (customCommands[i].key == nullptr) {
            customCommands[i].key = key;
            customCommands[i].callback = callback;
            break;
        }
    }
}

void console::Console::setSetting(const char *key, const char *value) {
    for (size_t i{0}; i < SETTINGS_MAX; ++i) {
        if (strcmp(settings[i].key, key) == 0) {
            strncpy(settings[i].value, value, strlen(value));
            break;
        }
        if (settings[i].key[0] == '\0') {
            strncpy(settings[i].key, key, strlen(key));
            strncpy(settings[i].value, value, strlen(value));
            break;
        }
    }
}

bool console::Console::settingBool(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        return (strcmp(setting->value, "1") == 0) || (strcmp(setting->value, "true") == 0);
    }
    return false;
}

int console::Console::settingInt(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        if (isdigit(setting->value[0]) || setting->value[0] == '-') {
            return atoi(setting->value);
        }
    }
    return 0;
}

float console::Console::settingFloat(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        if (isdigit(setting->value[0]) || setting->value[0] == '-') {
            return atof(setting->value);
        }
    }
    return 0;
}

const char *console::Console::settingString(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        return setting->value;
    }
    return "";
}

void console::Console::registerIConsole(IConsole *iConsole) { this->iConsole = iConsole; }
