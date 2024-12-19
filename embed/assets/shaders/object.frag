#version 330 core

precision highp float;

out vec4 FragColor;

uniform vec4 u_color;
uniform sampler2D u_diffuse;

in vec2 UV;

void main()
{
    vec4 diffuse = texture(u_diffuse, UV);
    vec4 color = diffuse * u_color;
    if(color.a < 0.001)  {
        discard;
    }
    FragColor = color;
    //FragColor = vec4(color_in);
}