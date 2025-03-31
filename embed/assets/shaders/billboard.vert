#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

layout (std140) uniform CameraBlock
{
    mat4 u_projection;
    mat4 u_view;
    vec3 u_cameraPos;
};

uniform mat4 u_model;
//uniform vec2 u_scale;

out vec2 UV;
out vec3 N;
out vec3 P;
out vec3 C;

void main()
{
    UV = aUV;

    mat4 modelView = u_view * u_model;

    // Billboard FTW!
    modelView[0][0] = 1.0; // * u_scale.x;
    modelView[0][1] = 0.0;
    modelView[0][2] = 0.0;

    modelView[1][0] = 0.0;
    modelView[1][1] = 1.0; // * u_scale.y;
    modelView[1][2] = 0.0;

    modelView[2][0] = 0.0;
    modelView[2][1] = 0.0;
    modelView[2][2] = 1.0;

    N = normalize(vec3(modelView * vec4(aNormal, 0.0)));

    vec3 p = vec3(modelView * vec4(aPos, 1.0f));
    P = p;
    C = u_cameraPos;

    gl_Position = u_projection * vec4(p, 1.0f);
}