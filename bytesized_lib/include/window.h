#pragma once

// Default headers (more than necessary)
#include "timer.h"
#include <SDL.h>

#define __WBINDERS                                                                                 \
    __WBINDER(KeyListener)                                                                         \
    __WBINDER(TextListener)                                                                        \
    __WBINDER(MouseListener)                                                                       \
    __WBINDER(MouseWheelListener)                                                                  \
    __WBINDER(MouseMotionListener)

namespace window {

struct IEngine {
    virtual void init(int drawableWidth, int drawableHeight) = 0;
    virtual bool update(float dt) = 0;
    virtual void resize(int drawableWidth, int drawableHeight) = 0;
    virtual void draw() = 0;
    virtual void fps(float fps) = 0;
};

struct IKeyListener {
    virtual bool keyDown(int key, int mods) = 0;
    virtual bool keyUp(int key, int mods) = 0;
};

struct IMouseListener {
    virtual bool mouseDown(int button, int x, int y) = 0;
    virtual bool mouseUp(int button, int x, int y) = 0;
};

struct IMouseWheelListener {
    virtual bool mouseScrolled(float x, float y) = 0;
};

struct IMouseMotionListener {
    virtual bool mouseMoved(float x, float y, float xrel, float yrel) = 0;
};

struct ITextListener {
    virtual bool textInput(const char *c) = 0;
};

void create(const char *title, int winX, int winY, bool fullscreen);
void registerEngine(IEngine *iApplication);
#define __WBINDER(symbol)                                                                          \
    void register##symbol(I##symbol *i##symbol);                                                   \
    void disable##symbol(I##symbol *i##symbol, bool disabled);
__WBINDERS
#undef __WBINDER

void loop_forever();

} // namespace window