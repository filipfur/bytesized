#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform vec4 u_transform;
uniform ivec2 u_flip;

out vec3 N;
out vec2 UV;

void main()
{
    UV = aUV;
    if(u_flip.x > 0) {
        UV.x = 1.0 - UV.x;
    }
    if(u_flip.y > 0) {
        UV.y = 1.0 - UV.y;
    }
    mat4 model = mat4(1.0);
    model[0][0] = u_transform.z;
    model[1][1] = u_transform.w;
    model[3][0] = u_transform.x;
    model[3][1] = u_transform.y;
    vec3 p = vec3(model * vec4(aPos, 1.0));
    N = vec3(0.0, 0.0, 1.0);
    gl_Position = u_projection * u_view * vec4(p, 1.0);
}
