#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, binding = 0) uniform UniformBuffer
{
    mat4 mvp; 
    vec4 position[12 * 3];
    vec4 attr[12 * 3];
} ubo;

layout(location = 0) out vec4 texCoord;
layout(location = 1) out vec3 fragPos;

void main()
{
    texCoord = ubo.attr[gl_VertexIndex];
    transpose(ubo.mvp);
    gl_Position = ubo.mvp * ubo.position[gl_VertexIndex];
    fragPos = gl_Position.xyz;
}