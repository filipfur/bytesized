#version 330 core

precision highp float;

out vec4 FragColor;

uniform vec4 u_color;
uniform sampler2D u_diffuse;

in vec2 UV;

// in vec3 color;

void main()
{
    vec4 diffuse = texture(u_diffuse, UV);
    FragColor = vec4(diffuse * u_color);
    // FragColor = vec4(color, 1.0);
}