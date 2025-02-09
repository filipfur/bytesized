#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

layout (std140) uniform CameraBlock
{
    mat4 u_projection;
    mat4 u_view;
    vec3 u_cameraPos;
};
uniform mat4 u_model;
uniform float u_time;
uniform float u_metallic;

out vec3 N;
out vec2 UV;

void main()
{
    UV = aUV;
    float wave = sin(u_time * 4.0 + aPos.x * 4.0) * 2.0 * u_metallic;
    vec3 p = vec3(u_model * vec4(aPos + vec3(0.0, wave, 0.0), 1.0));
    N = normalize(vec3(u_model * vec4(aNormal, 0.0)));
    gl_Position = u_projection * u_view * vec4(p, 1.0);
}
