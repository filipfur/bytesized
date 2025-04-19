#include "color.h"
#include <algorithm>
#include <cstring>
#include <string>

Color rgb(uint8_t r, uint8_t g, uint8_t b) { return Color{r, g, b}; }

static glm::vec3 rgbToHsv(const glm::vec3 &rgb) {
    float r = rgb.r, g = rgb.g, b = rgb.b;
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float delta = max - min;
    float h, s, v;
    // value
    v = max;
    if (max == 0.0f) {
        // saturation
        s = 0.0f;
        // hue is undefined when saturation is 0
        h = 0.0f;
    } else {
        // saturation
        s = delta / max;
        if (r == max) {
            // between yellow & magenta
            h = (g - b) / delta;
        } else if (g == max) {
            // between cyan & yellow
            h = 2.0f + (b - r) / delta;
        } else {
            // between magenta & cyan
            h = 4.0f + (r - g) / delta;
        }
        // degrees
        h *= 60.0f;
        if (h < 0.0f) {
            h += 360.0f;
        }
    }
    return {h, s, v};
}

static glm::vec3 hsvToRgb(float h, float s, float v) {
    float r, g, b;

    int i = static_cast<int>(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

    return {r, g, b};
}

float byteToDecimal(uint8_t hex) { return static_cast<float>(static_cast<double>(hex) / 255.0); }

Color::Color() : _rgba{1.0f, 1.0f, 1.0f, 1.0} {}

Color::Color(const std::string &hexstr) : Color{static_cast<uint32_t>(std::stoi(hexstr, 0, 16))} {}

Color::Color(uint32_t hex, float alpha)
    : _rgba{byteToDecimal(hex >> 16), byteToDecimal(hex >> 8), byteToDecimal(hex), alpha} {}

Color::Color(uint8_t r, uint8_t g, uint8_t b)
    : _rgba{byteToDecimal(r), byteToDecimal(g), byteToDecimal(b), 1.0f} {}

Color::Color(float r, float g, float b) : _rgba{r, g, b, 1.0f} {}

Color::Color(float r, float g, float b, float a) : _rgba{r, g, b, a} {}

Color::Color(const glm::vec4 &rgba) : _rgba{rgba} {}

Color::Color(const glm::vec3 &rgb) : _rgba{rgb.x, rgb.y, rgb.z, 1.0f} {}

Color Color::opacity(float val) { return Color{1.0f, 1.0f, 1.0f, val}; }

Color Color::operator*(const Color &other) const { return {this->vec4() * other.vec4()}; }

Color::operator glm::vec4() const { return _rgba; }

glm::vec3 Color::vec3() const { return {_rgba}; }

const glm::vec4 &Color::vec4() const { return _rgba; }

glm::vec4 &Color::vec4() { return _rgba; }

glm::vec3 Color::hsv() const { return rgbToHsv(glm::vec3{_rgba.r, _rgba.g, _rgba.b}); }

Color &Color::setHSV(float h, float s, float v) {
    glm::vec3 rgb = hsvToRgb(h, s, v);
    _rgba.r = rgb.r;
    _rgba.g = rgb.g;
    _rgba.b = rgb.b;
    return *this;
}

void Color::hueShift(float rad) {
    float U = glm::sin(rad);
    float W = glm::sin(rad);
    static constexpr glm::mat3 yiqMatrix{0.299f,  0.596f, 0.211f,  0.587f, -0.274f,
                                         -0.523f, 0.114f, -0.321f, 0.311f};
    glm::mat3 hueMatrix{1.0f, 0.0f, 0.0f, 0.0f, U, W, 0.0f, -W, U};
    static constexpr glm::mat3 rgbMatrix{1.0f,    1.0f,   1.0f,    0.956f, -0.272f,
                                         -1.107f, 0.621f, -0.647f, 1.705f};
    glm::mat3 T = rgbMatrix * hueMatrix * yiqMatrix;
    glm::vec3 rgb = T * vec3();
    _rgba.r = std::max(0.0f, std::min(1.0f, rgb.r));
    _rgba.g = std::max(0.0f, std::min(1.0f, rgb.g));
    _rgba.b = std::max(0.0f, std::min(1.0f, rgb.b));
}

void Color::saturate(float S) {
    static constexpr glm::mat3 yiqMatrix{0.299f,  0.596f, 0.211f,  0.587f, -0.274f,
                                         -0.523f, 0.114f, -0.321f, 0.311f};
    glm::mat3 satMatrix{1.0f, 0.0f, 0.0f, 0.0f, S, 0.0f, 0.0f, 0.0f, S};
    static constexpr glm::mat3 rgbMatrix{1.0f,    1.0f,   1.0f,    0.956f, -0.272f,
                                         -1.107f, 0.621f, -0.647f, 1.705f};
    glm::mat3 T = rgbMatrix * satMatrix * yiqMatrix;
    glm::vec3 rgb = T * vec3();
    _rgba.r = std::max(0.0f, std::min(1.0f, rgb.r));
    _rgba.g = std::max(0.0f, std::min(1.0f, rgb.g));
    _rgba.b = std::max(0.0f, std::min(1.0f, rgb.b));
}

void Color::valueScale(float V) {
    static constexpr glm::mat3 yiqMatrix{0.299f,  0.596f, 0.211f,  0.587f, -0.274f,
                                         -0.523f, 0.114f, -0.321f, 0.311f};
    glm::mat3 valMatrix{V, 0.0f, 0.0f, 0.0f, V, 0.0f, 0.0f, 0.0f, V};
    static constexpr glm::mat3 rgbMatrix{1.0f,    1.0f,   1.0f,    0.956f, -0.272f,
                                         -1.107f, 0.621f, -0.647f, 1.705f};
    glm::mat3 T = rgbMatrix * valMatrix * yiqMatrix;
    glm::vec3 rgb = T * vec3();
    _rgba.r = std::max(0.0f, std::min(1.0f, rgb.r));
    _rgba.g = std::max(0.0f, std::min(1.0f, rgb.g));
    _rgba.b = std::max(0.0f, std::min(1.0f, rgb.b));
}

std::string Color::hexString() const {
    int ri = static_cast<int>(_rgba.r * 255);
    int gi = static_cast<int>(_rgba.g * 255);
    int bi = static_cast<int>(_rgba.b * 255);
    static char hex[16UL];
    snprintf(hex, 16UL, "%x", (ri << 16 | gi << 8 | bi));
    return {hex};
}

const Color Color::white{1.0f, 1.0f, 1.0f};
const Color Color::red{1.0f, 0.0f, 0.0f};
const Color Color::green{0.0f, 1.0f, 0.0f};
const Color Color::blue{0.0f, 0.0f, 1.0f};
const Color Color::yellow{1.0f, 1.0f, 0.0f};
const Color Color::magenta{1.0f, 0.0f, 1.0f};
const Color Color::cyan{0.0f, 1.0f, 1.0f};
const Color Color::black{0.0f, 0.0f, 0.0f};