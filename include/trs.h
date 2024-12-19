#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TRS {
    template <typename T> struct Attribute {
        Attribute(TRS *trs, const T &t) : _trs{trs}, _t{t}, x{_t.x}, y{_t.y}, z{_t.z} {}
        Attribute &operator=(const T &t) {
            _t = t;
            _trs->invalidate();
            return *this;
        }
        Attribute &operator+=(const T &t) {
            _t += t;
            _trs->invalidate();
            return *this;
        }
        Attribute &operator-=(const T &t) {
            _t -= t;
            _trs->invalidate();
            return *this;
        }
        Attribute &operator*=(const T &t) {
            _t *= t;
            _trs->invalidate();
            return *this;
        }
        Attribute &operator/=(const T &t) {
            _t /= t;
            _trs->invalidate();
            return *this;
        }

        const T &data() const { return _t; }
        T &data() { return _t; }

        TRS *_trs;
        T _t;
        const float &x;
        const float &y;
        const float &z;
    };

    TRS()
        : translation{this, {0.0f, 0.0f, 0.0f}}, rotation{this, {1.0f, 0.0f, 0.0f, 0.0f}},
          scale{this, {1.0f, 1.0f, 1.0f}} {}

    Attribute<glm::vec3> translation;
    Attribute<glm::quat> rotation;
    Attribute<glm::vec3> scale;

    glm::mat4 &model();

    TRS *parent();
    void setParent(TRS *parent);

    void setTransform(TRS &other) {
        _model = other.model();
        _valid = true;
    }

    void invalidate() { _valid = false; }
    bool valid() const { return _valid; }

  private:
    glm::mat4 _model{1.0f};
    bool _valid{false};
    TRS *_parent{nullptr};
};