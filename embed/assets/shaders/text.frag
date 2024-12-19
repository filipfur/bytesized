#version 330 core

precision highp float;

out vec4 FragColor;

uniform vec4 u_color;
uniform sampler2D u_diffuse;

in vec2 UV;

void main()
{
    vec4 diffuse = texture(u_diffuse, UV);
    if(diffuse.r < 0.1) {
        discard;
    }
    FragColor = vec4(diffuse.r) * u_color;
}