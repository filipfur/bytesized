#include "texture.h"

#include "opengl.h"
#include <cstdio>

void gpu::Texture_create(const gpu::Texture &texture, const uint8_t *data, uint32_t width,
                         uint32_t height, ChannelSetting channels, uint32_t type) {
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLenum format;
    GLenum internalFormat;
    switch (channels) {
    case ChannelSetting::R:
        format = internalFormat = GL_RED;
        break;
    case ChannelSetting::RGB:
        format = internalFormat = GL_RGB;
        break;
    case ChannelSetting::RGBA:
        format = internalFormat = GL_RGBA;
        break;
    case ChannelSetting::DS:
        internalFormat = GL_DEPTH24_STENCIL8;
        format = GL_DEPTH_STENCIL;
        break;
    default:
        printf("ERROR: Unknown texture channel format\n");
        format = internalFormat = GL_RED;
        break;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gpu::Texture::bind() { glBindTexture(GL_TEXTURE_2D, id); }

void gpu::Texture::unbind() { glBindTexture(GL_TEXTURE_2D, 0); }