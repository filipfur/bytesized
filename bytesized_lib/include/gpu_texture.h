#pragma once

#include <cstdint>

namespace gpu {

struct Texture {
    uint32_t id;
    void bind();
    void unbind();
};

enum class ChannelSetting {
    NONE,
    R,
    RG,
    RGB,
    RGBA,
    DS,
};

void Texture_create(const gpu::Texture &texture, const uint8_t *data, uint32_t width,
                    uint32_t height, ChannelSetting channels, uint32_t type);
} // namespace gpu