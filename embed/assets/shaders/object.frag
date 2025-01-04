#version 330 core

precision highp float;

out vec4 FragColor;

uniform vec4 u_color;
uniform sampler2D u_diffuse;

in vec3 N;
in vec2 UV;

void main()
{
    const vec3 up = vec3(0,1,0);
    vec4 diffuse = texture(u_diffuse, UV);
    vec4 color = diffuse * u_color;
    if(color.a < 0.001)  {
        discard;
    }
    FragColor = vec4(color.rgb + dot(N, up) * 0.14, 1.0);
}