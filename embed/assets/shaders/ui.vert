#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec3 N;
out vec2 UV;

void main()
{
    UV = aUV;
    vec3 p = vec3(u_model * vec4(aPos, 1.0));
    N = normalize(vec3(u_model * vec4(aNormal, 0.0)));
    gl_Position = u_projection * u_view * vec4(p, 1.0);
}
