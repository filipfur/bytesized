#include "console.h"
#include "dexterity.h"
#include "gpu.h"
#include "logging.h"
#include "recycler.hpp"
#include <algorithm>
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

static bool _parseAxis(char c, glm::ivec3 &axis) {
    switch (c) {
    case 'x':
    case 'X':
        axis.x = 1;
        break;
    case 'y':
    case 'Y':
        axis.y = 1;
        break;
    case 'z':
    case 'Z':
        axis.z = 1;
        break;
    default:
        return false;
    }
    return true;
}

static bool _parseTransform(const char *commandLine, glm::ivec3 &axis, bool &apply,
                            glm::vec3 &value) {
    if (commandLine[2] == '.') {
        size_t i = 3;
        for (size_t j{0}; j < 3; ++j) {
            if (!_parseAxis(commandLine[i], axis)) {
                break;
            }
            ++i;
        }
        if (i == 3) {
            return false;
        }
        if (commandLine[i] == '=') {
            ++i;
            if (isdigit(commandLine[i]) || commandLine[i] == '-') {
                float f = atof(commandLine + i);
                for (size_t j{0}; j < 3; ++j) {
                    if (axis[j]) {
                        value[j] = f;
                    }
                }
                apply = false;
                return true;
            }
        } else if ((commandLine[i] == '+' || commandLine[i] == '-') && commandLine[i + 1] == '=') {
            bool addition = (commandLine[i] == '+');
            i += 2;
            if (isdigit(commandLine[i]) || commandLine[i] == '-') {
                float f = atof(commandLine + i);
                for (size_t j{0}; j < 3; ++j) {
                    if (axis[j]) {
                        value[j] = f;
                    }
                }
                value = addition ? value : -value;
                apply = true;
                return true;
            }
        }
    }
    return false;
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

Console::Setting *_findSetting(Console::Setting *settings, const char *key) {
    for (size_t i{0}; i < Console::SETTINGS_MAX; ++i) {
        if (settings[i].key[0] == '\0') {
            break;
        }
        if (strcmp(settings[i].key, key) == 0) {
            return settings + i;
        }
    }
    return nullptr;
}

bool Console::evaluate() {
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
        _iConsole->listNodes();
        return true;
    } else if (starts_with(commandLine, ":f ")) {
        return _iConsole->openSaveFile(commandLine + strlen(":f "));
    } else if (starts_with(commandLine, ":w ")) {
        return _iConsole->saveSaveFile(commandLine + strlen(":w "));
    } else if (starts_with(commandLine, ":t.") || starts_with(commandLine, ":r.") ||
               starts_with(commandLine, ":s.")) {
        glm::ivec3 axis{0, 0, 0};
        bool apply;
        glm::vec3 value{0.0f, 0.0f, 0.0f};
        if (_parseTransform(commandLine, axis, apply, value)) {
            switch (commandLine[1]) {
            case 't':
                return apply ? _iConsole->applyNodeTranslation(axis, value)
                             : _iConsole->setNodeTranslation(axis, value);
            case 'r':
                return apply ? _iConsole->applyNodeRotation(axis, value)
                             : _iConsole->setNodeRotation(axis, value);
            case 's':
                return apply ? _iConsole->applyNodeScale(axis, value)
                             : _iConsole->setNodeScale(axis, value);
            }
        }
    } else if (starts_with(commandLine, ":wq")) {
        if (_iConsole->saveSaveFile(nullptr)) {
            _iConsole->quit(false);
        }
    } else if (starts_with(commandLine, ":q!")) {
        _iConsole->quit(true);
    } else if (starts_with(commandLine, ":q")) {
        _iConsole->quit(false);
    } else if (starts_with(commandLine, ":w")) {
        return _iConsole->saveSaveFile(nullptr);
    } else if (starts_with(commandLine, ":spawn ")) {
        return _iConsole->spawnNode(commandLine + strlen(":spawn "));
    } else if (starts_with(commandLine, ":o ")) {
        const char *par = commandLine + strlen(":o ");
        if (isdigit(par[0])) {
            return _iConsole->openScene(atoi(par));
        } else {
            return _iConsole->openScene(par);
        }
    } else {
        for (const auto &command : customCommands) {
            if (command.key == nullptr) {
                break;
            }
            if (starts_with(commandLine, command.key)) {
                return command.callback(commandLine);
            }
        }
    }
    return false;
}

bool Console::keyDown(int key, int mods) {
    if (visible) {
        if (key == SDLK_RETURN) {
            if (evaluate()) {
                visible = false;
                auto it = std::find(history.begin(), history.end(), std::string{commandLine});
                if (it == history.end()) {
                    history.emplace_back(commandLine);
                } else {
                    std::iter_swap(it, history.rbegin());
                }
                SDL_StopTextInput();
            }
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
                    _iConsole->inputChanged(true, commandLine);
                }
            }
        } else if (key == SDLK_DOWN) {
            if (historyIt != history.rend()) {
                if (historyIt != history.rbegin()) {
                    --historyIt;
                }
                strcpy(commandLine, historyIt->c_str());
                cursor = historyIt->length();
                _iConsole->inputChanged(true, commandLine);
            }
        }
        _iConsole->inputChanged(visible, commandLine);
        return true;
    } else {
        if (key == SDLK_PERIOD) {
            if ((mods & KMOD_SHIFT)) {
                visible = true;
                SDL_StartTextInput();
                cursor = 0;
                std::memset(commandLine, 0, sizeof(commandLine));
                commandLine[cursor++] = ':';
                _iConsole->inputChanged(true, commandLine);
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
                _iConsole->inputChanged(true, commandLine);
            }
        }
    }
    return false;
}

bool Console::keyUp(int /*key*/, int /*mods*/) { return false; }

bool Console::textInput(const char *text) {
    if (visible) {
        commandLine[cursor++] = *text;
        _iConsole->inputChanged(true, text);
    }
    return false;
}

void Console::addCustomCommand(const char *key, const Callback &callback) {
    for (size_t i{0}; i < CUSTOM_COMMAND_MAX; ++i) {
        if (customCommands[i].key == nullptr) {
            customCommands[i].key = key;
            customCommands[i].callback = callback;
            break;
        }
    }
}

void Console::setSetting(const char *key, const char *value) {
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

bool Console::settingBool(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        return (strcmp(setting->value, "1") == 0) || (strcmp(setting->value, "true") == 0);
    }
    return false;
}

int Console::settingInt(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        if (isdigit(setting->value[0]) || setting->value[0] == '-') {
            return atoi(setting->value);
        }
    }
    return 0;
}

float Console::settingFloat(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        if (isdigit(setting->value[0]) || setting->value[0] == '-') {
            return atof(setting->value);
        }
    }
    return 0;
}

const char *Console::settingString(const char *key) {
    if (auto *setting = _findSetting(settings, key)) {
        return setting->value;
    }
    return "";
}

void Console::registerIConsole(IConsole *iConsole) { this->_iConsole = iConsole; }
