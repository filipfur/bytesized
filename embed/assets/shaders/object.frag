#version 330 core

precision highp float;

out vec4 FragColor;

uniform vec4 u_color;
uniform sampler2D u_diffuse;

in vec3 N;
in vec2 UV;
in vec3 C;
in vec3 P;

layout (std140) uniform LightBlock
{
    vec3 u_light_hi;
    vec3 u_light_lo;
    vec3 u_ambient;
};

const float toon_levels = 3.0;
const float toon_factor = 1.0 / toon_levels;

void main()
{
    const vec3 up = vec3(0,1,0);
    const vec3 L = normalize(vec3(-1,1,1));
    vec4 diffuse = texture(u_diffuse, UV);
    vec4 color = diffuse * u_color;
    if(color.a < 0.001)  {
        discard;
    }
    vec3 PC = C - P;
    
    vec3 light = mix(u_light_hi, u_light_lo, step(dot(N, L), 0.1));

    FragColor = vec4(color.rgb * 0.6 * u_ambient + light * 0.2, 1.0);
}