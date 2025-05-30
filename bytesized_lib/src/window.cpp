#include "window.h"

#include "opengl.h"
#include <cassert>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef _WIN32
#undef main
#endif

#define FOR_ITH(pointer, functionCall)                                                             \
    for (size_t i{0}; i < BINDERS_MAX; ++i) {                                                      \
        functionCall                                                                               \
    }

static const int UPDATE_FREQ = 60;
static const int PERIOD_TIME_MS = 1000 / UPDATE_FREQ;
static constexpr float DELTA_TIME_S = 1.0f / static_cast<float>(UPDATE_FREQ);

static const size_t BINDERS_MAX = 4;
static window::IWindowApp *_iWindowApp;
#define __WBINDER(symbol)                                                                          \
    static window::I##symbol *_i##symbol[BINDERS_MAX] = {};                                        \
    static bool _d##symbol[BINDERS_MAX] = {};
__WBINDERS
#undef __WBINDER
static SDL_Window *_window{nullptr};
static SDL_GLContext _glContext{nullptr};

void window::create(const char *title, int winX, int winY, bool fullscreen) {
    SDL_Init(SDL_INIT_EVERYTHING);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    _window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winX, winY,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    _glContext = SDL_GL_CreateContext(_window);
    assert(SDL_GL_MakeCurrent(_window, _glContext) == 0);
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(_window, &drawableWidth, &drawableHeight);

    if (fullscreen) {
        SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN);
    }

#ifndef __EMSCRIPTEN__
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    SDL_StopTextInput();

    _iWindowApp->init(drawableWidth, drawableHeight);
}

static bool _running = true;
static void _mainLoop() {
    static SDL_Event event;
    static uint32_t lastTick{0};
    static uint32_t deltaTicks{0};
    static uint32_t fpsTime{0};
    static uint16_t fpsCounter{0};
    static float fpsAcc{0};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            FOR_ITH(
                _iMouseListener,
                if (_iMouseListener[i] && !_dMouseListener[i]) if (_iMouseListener[i]->mouseDown(
                                                                       event.button.button,
                                                                       event.button.x,
                                                                       event.button.y)) { break; })
            break;
        case SDL_MOUSEBUTTONUP:
            FOR_ITH(
                _iMouseListener,
                if (_iMouseListener[i] && !_dMouseListener[i]) if (_iMouseListener[i]->mouseUp(
                                                                       event.button.button,
                                                                       event.button.x,
                                                                       event.button.y)) { break; })
            break;
        case SDL_MOUSEWHEEL:
            FOR_ITH(
                _iMouseWheelListener,
                if (_iMouseWheelListener[i] &&
                    !_dMouseWheelListener[i]) if (_iMouseWheelListener[i]
                                                      ->mouseScrolled(event.wheel.preciseX,
                                                                      event.wheel.preciseY)) {
                    break;
                })
            break;
        case SDL_MOUSEMOTION:
            FOR_ITH(
                _iMouseMotionListener,
                if (_iMouseMotionListener[i] &&
                    !_dMouseMotionListener[i]) if (_iMouseMotionListener[i]
                                                       ->mouseMoved(event.motion.x, event.motion.y,
                                                                    event.motion.xrel,
                                                                    event.motion.yrel)) { break; })
            break;
        case SDL_KEYDOWN:
            FOR_ITH(
                _iKeyListener,
                if (_iKeyListener[i] &&
                    !_dKeyListener[i]) if (_iKeyListener[i]->keyDown(event.key.keysym.sym,
                                                                     event.key.keysym.mod)) {
                    break;
                })
            break;
        case SDL_KEYUP:
            FOR_ITH(
                _iKeyListener,
                if (_iKeyListener[i] &&
                    !_dKeyListener[i]) if (_iKeyListener[i]->keyUp(event.key.keysym.sym,
                                                                   event.key.keysym.mod)) {
                    break;
                })
            break;
        case SDL_TEXTINPUT:
            FOR_ITH(
                _iTextListener,
                if (_iTextListener[i] && !_dTextListener[i]) if (_iTextListener[i]->textInput(
                                                                     event.text.text)) { break; })
            break;
        case SDL_QUIT:
            _running = false;
            return;
        default:
            break;
        }
    }

    uint32_t tick = SDL_GetTicks();
    deltaTicks += (tick - lastTick);
    lastTick = tick;
    if (tick > fpsTime + 100) {
        if (_iWindowApp) {
            fpsAcc = fpsAcc * 0.5f + fpsCounter * 5.0f;
            _iWindowApp->fps(fpsAcc);
            fpsCounter = 0;
        }
        fpsTime = fpsTime + 100;
    }
    bool updated = false;
    while (deltaTicks >= PERIOD_TIME_MS) {
        if (_iWindowApp->update(DELTA_TIME_S)) {
            updated = true;
        }
        deltaTicks -= PERIOD_TIME_MS;
        Time::increment(Time::fromMilliseconds(PERIOD_TIME_MS));
    }
    if (updated) {
        _iWindowApp->draw();
        ++fpsCounter;
        SDL_GL_SwapWindow(_window);
    }
}

void window::loop_forever() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(_mainLoop, -1, 1);
#else
    while (_running) {
        _mainLoop();
        SDL_Delay(1);
    }
#endif
    SDL_GL_DeleteContext(_glContext);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void window::registerWindowApp(window::IWindowApp *iWindowApp) { _iWindowApp = iWindowApp; }

#define __WBINDER(symbol)                                                                          \
    void window::register##symbol(I##symbol *i##symbol) {                                          \
        for (size_t i{0}; i < BINDERS_MAX; ++i) {                                                  \
            if (_i##symbol[i]) {                                                                   \
                continue;                                                                          \
            }                                                                                      \
            _i##symbol[i] = i##symbol;                                                             \
            break;                                                                                 \
        }                                                                                          \
    }                                                                                              \
    void window::disable##symbol(I##symbol *i##symbol, bool disabled) {                            \
        for (size_t i{0}; i < BINDERS_MAX; ++i) {                                                  \
            if (_i##symbol[i] == i##symbol) {                                                      \
                _d##symbol[i] = disabled;                                                          \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    }
__WBINDERS
#undef __WBINDER
