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

out vec3 N;
out vec2 UV;
out vec3 C;
out vec3 P;

void main()
{
    UV = aUV;
    vec3 p = vec3(u_model * vec4(aPos, 1.0));
    N = normalize(vec3(u_model * vec4(aNormal, 0.0)));
    C = u_cameraPos;
    P = p;
    gl_Position = u_projection * u_view * vec4(p, 1.0);
}
