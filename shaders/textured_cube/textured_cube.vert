#version 450

layout(std140, binding = 0) uniform UniformBuffer
{
    mat4 mvp;
} ubo;

layout(location = 0) in vec4 inPos;
layout(location = 2) in vec2 inTexCoord;

layout(location = 1) out vec2 outTexCoord;

void main()
{
    outTexCoord = inTexCoord;
    gl_Position = ubo.mvp * inPos;
}