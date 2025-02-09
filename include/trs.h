#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TRS {
    template <typename T> struct Attribute {
        Attribute(const std::function<void()> &callback, const T &t)
            : _callback{callback}, _t{t}, x{_t.x}, y{_t.y}, z{_t.z} {}
        Attribute &operator=(const T &t) {
            _t = t;
            _callback();
            return *this;
        }
        Attribute &operator+=(const T &t) {
            _t += t;
            _callback();
            return *this;
        }
        Attribute &operator-=(const T &t) {
            _t -= t;
            _callback();
            return *this;
        }
        Attribute &operator*=(const T &t) {
            _t *= t;
            _callback();
            return *this;
        }
        Attribute &operator/=(const T &t) {
            _t /= t;
            _callback();
            return *this;
        }

        const T &data() const { return _t; }
        T &data() { return _t; }

        std::function<void()> _callback;
        T _t;
        const float &x;
        const float &y;
        const float &z;
    };

    TRS()
        : translation{[this]() { invalidate(); }, {0.0f, 0.0f, 0.0f}},
          scale{[this]() { invalidate(); }, {1.0f, 1.0f, 1.0f}},
          rotation{[this]() { invalidate(); }, {1.0f, 0.0f, 0.0f, 0.0f}},
          euler{[this]() {
                    _invalidEuler = true;
                    invalidate();
                },
                {0.0f, 0.0f, 0.0f}} {}

    Attribute<glm::vec3> translation;
    Attribute<glm::vec3> scale;
    Attribute<glm::quat> rotation;
    Attribute<glm::vec3> euler;

    glm::mat4 &model();

    TRS *parent();
    void setParent(TRS *parent);

    void setTransform(TRS &other) {
        _model = other.model();
        _valid = true;
    }

    const glm::vec3 &t() { return translation.data(); }

    const glm::quat &r() { return rotation.data(); }

    const glm::vec3 &s() { return scale.data(); }

    void invalidate() { _valid = false; }
    bool valid() const { return _valid; }

  private:
    glm::mat4 _model{1.0f};
    bool _valid{false};
    TRS *_parent{nullptr};
    bool _invalidEuler{false};
};