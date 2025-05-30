#pragma once

#include "window.h"
#include <cstring>
#include <functional>

template <std::size_t N>
struct TextField : public window::IKeyListener, public window::ITextListener {

    TextField() {
        window::registerKeyListener(this);
        window::registerTextListener(this);
        setEnabled(false);
    }

    void setValue(const char *value) {
        size_t len = strlen(value);
        strncpy(this->value, value, len);
        cursor = len;
    }

    void setEnabled(bool enabled) {
        _enabled = enabled;
        window::disableKeyListener(this, !enabled);
        window::disableTextListener(this, !enabled);
        if (enabled) {
            SDL_StartTextInput();
        } else {
            SDL_StopTextInput();
        }
    }
    bool enabled() const { return _enabled; }

    void setHidden(bool hidden) {
        _hidden = hidden;
        if (hidden && _enabled) {
            setEnabled(false);
        }
    }
    bool hidden() const { return _hidden; }

    virtual bool keyDown(int key, int mods) override {
        if (!_enabled) {
            return false;
        }
        if (key == SDLK_RETURN) {
            if (onEnter && onEnter(this)) {
                setEnabled(false);
            }
        } else if (key == SDLK_ESCAPE) {
            setEnabled(false);
        } else if (key == SDLK_BACKSPACE) {
            if (cursor > 0) {
                value[--cursor] = '\0';
                if (onChanged) {
                    onChanged(this);
                }
            }
        }
        return true;
    }
    virtual bool keyUp(int key, int mods) override {
        if (!_enabled) {
            return false;
        }
        return true;
    }
    virtual bool textInput(const char *text) override {
        if (!_enabled) {
            return false;
        }
        value[cursor++] = *text;
        if (onChanged) {
            onChanged(this);
        }
        return true;
    }

    uint8_t cursor{0};
    char value[N] = {};

    std::function<bool(TextField *)> onEnter;
    std::function<void(TextField *)> onChanged;

  private:
    bool _hidden = false;
    bool _enabled = false;
};