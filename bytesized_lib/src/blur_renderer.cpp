#include "blur_renderer.h"

static const char *_blurVertSrc = BYTESIZED_GLSL_VERSION R"(
layout(location=0) in vec4 aVertex;

out vec2 texCoords;

void main()
{
  texCoords = aVertex.zw;
  gl_Position = vec4(aVertex.xy, 0.0, 1.0);
}
)";

static const char *_blurFragSrc = BYTESIZED_GLSL_VERSION R"(
precision highp float;

in vec2 texCoords;
out vec4 FragColor;

uniform sampler2D u_texture;
  
uniform bool u_horizontal;
const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    vec2 tex_offset = 1.0 / vec2(textureSize(u_texture, 0)); // gets size of single texel
    vec3 result = texture(u_texture, texCoords).rgb * weight[0]; // current fragment's contribution
    if(u_horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_texture, texCoords + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(u_texture, texCoords - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_texture, texCoords + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture(u_texture, texCoords - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)";

static gpu::ShaderProgram *sharedShader() {
    static auto shader =
        gpu::createShaderProgram(gpu::createShader(GL_VERTEX_SHADER, _blurVertSrc),
                                 gpu::createShader(GL_FRAGMENT_SHADER, _blurFragSrc),
                                 {{"u_texture", 0}, {"u_horizontal", 0}});
    return shader;
}

void BlurRenderer::create(const glm::ivec2 &resolution, gpu::ShaderProgram *shaderProgram_) {
    shaderProgram = shaderProgram_ ? shaderProgram_ : sharedShader();
    for (int i{0}; i < 2; ++i) {
        blurFBO[i] = gpu::createFramebuffer();
        blurFBO[i]->bind();
        blurFBO[i]->createTexture(GL_COLOR_ATTACHMENT0, resolution.x, resolution.y,
                                  gpu::ChannelSetting::RGB, GL_UNSIGNED_BYTE);
        blurFBO[i]->unbind();
    }
}

gpu::Texture *BlurRenderer::blur(gpu::Texture *inputTexture) {
    shaderProgram->use();
    size_t amount = 10;

    blurFBO[0]->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    blurFBO[1]->bind();
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);

    bool horizontal = true, first_iteration = true;
    for (size_t i = 0; i < amount; i++) {
        blurFBO[horizontal]->bind();
        shaderProgram->uniforms.at("u_horizontal") << horizontal;
        if (first_iteration) {
            inputTexture->bind();
            first_iteration = false;
        } else {
            blurFBO[!horizontal]->textures.at(GL_COLOR_ATTACHMENT0)->bind();
        }
        gpu::renderScreen();
        horizontal = !horizontal;
    }
    blurFBO[0]->unbind();
    return outputTexture();
}

gpu::Texture *BlurRenderer::outputTexture() {
    return blurFBO[0]->textures.at(GL_COLOR_ATTACHMENT0);
}