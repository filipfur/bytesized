#include "skydome.h"
#include "gpu_primitive.h"

static skydome::Sky _defaultSky{
    rgb(236, 239, 187),
    rgb(86, 113, 153),
    0.0f,
};

const char *skyboxVertSource = R"(#version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;

    layout (std140) uniform CameraBlock
    {
        mat4 u_projection;
        mat4 u_view;
        vec3 u_eye_pos;
    };

    out vec3 position;
    out vec3 normal;
    out vec2 texCoords;

    void main()
    {
        mat3 model = mat3(1.0);
        position = vec3(model * aPos);
        normal = normalize(model * aNormal);
        texCoords = aTexCoords;
        mat4 view = mat4(mat3(u_view));
        vec4 pos = u_projection * view * vec4(position, 1.0);
        gl_Position = pos.xyww;
    }
)";

const char *skyboxFragSource = R"(#version 330 core
precision highp float;
// Simplex 2D noise
//
vec3 permute(vec3 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

float snoise(vec2 v){
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
           -0.577350269189626, 0.024390243902439);
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);
  vec2 i1;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod(i, 289.0);
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
  + i.x + vec3(0.0, i1.x, 1.0 ));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
    dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}

    in vec2 texCoords;
    in vec3 normal;

    out vec4 fragColor;

    uniform float u_time;

    uniform vec3 u_color_lo;
    uniform vec3 u_color_hi;
    uniform float u_clouds;

    const vec3 lightDir = vec3(0.0, -1.0, 0.0);

    void main()
    {
        fragColor = vec4(mix(u_color_lo, u_color_hi, texCoords.y), 1.0);
        fragColor.rgb = mix(fragColor.rgb, fragColor.rgb + fragColor.rgb * max(0.0, snoise(vec2(sin(texCoords.x * 2.0 * 3.141592) * 0.5 + 0.5 + u_time * 0.02, texCoords.y) * vec2(2,2))) * 0.5, u_clouds);
    }
)";

static gpu::ShaderProgram *_shaderProgram{nullptr};
void skydome::create() {
    _shaderProgram =
        gpu::createShaderProgram(gpu::createShader(GL_VERTEX_SHADER, skyboxVertSource),
                                 gpu::createShader(GL_FRAGMENT_SHADER, skyboxFragSource),
                                 {
                                     {"u_time", 2.2f},
                                     {"u_color_hi", _defaultSky.high.vec3()},
                                     {"u_color_lo", _defaultSky.low.vec3()},
                                     {"u_clouds", _defaultSky.clouds},
                                 });
    setSky(_defaultSky);
    gpu::builtinUBO(gpu::UBO_CAMERA)->bindShaders({_shaderProgram});
}

void skydome::setSky(const Sky &sky) {
    _shaderProgram->use();
    _shaderProgram->uniforms.at("u_color_hi") << sky.high.vec3();
    _shaderProgram->uniforms.at("u_color_lo") << sky.low.vec3();
    _shaderProgram->uniforms.at("u_clouds") << sky.clouds;
    gpu::LightBlock_setLightColor(sky.high, sky.low);
}

void skydome::render() {
    _shaderProgram->use();
    glFrontFace(GL_CW);
    glActiveTexture(0);
    gpu::builtinMaterial(gpu::BuiltinMaterial::WHITE)->textures[GL_TEXTURE0]->bind();
    gpu::builtinPrimitives(gpu::SPHERE)->render();
    glFrontFace(GL_CCW);
}