#version 330 core

out vec4 FragColor;
  
in vec2 UV;

uniform sampler2D u_texture;
uniform vec4 u_sprite;

void main()
{
    vec2 xy = u_sprite.xy;
    vec2 uv = u_sprite.zw;

    vec4 diffuse = texture(u_texture, UV * uv + xy);
    if(diffuse.a < 0.1) {
        discard;
    }
    FragColor = diffuse;
}
