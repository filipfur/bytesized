#version 330 core

layout (location = 0) in vec4 aXYUV;
//layout (location = 1) in vec2 aUV;

out vec2 UV;

void main()
{
    gl_Position = vec4(aXYUV.x, aXYUV.y, 0.0, 1.0); 
    UV = aXYUV.zw;
}
