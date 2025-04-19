#version 330 core

out vec4 FragColor;
  
in vec2 UV;

uniform sampler2D u_texture;

void main()
{ 
    vec4 diffuse = texture(u_texture, UV);
    if(diffuse.a < 0.1) {
        discard;
    }
    FragColor = diffuse;
}
