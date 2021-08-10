#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec4 texCoord;
layout(location = 1) in vec3 fragPos;
layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(0.424, 0.566, 0.707);

void main()
{
   vec3 dX = dFdx(fragPos);
   vec3 dY = dFdy(fragPos);
   vec3 normal = normalize(cross(dX,dY));
   float light = max(0.0, dot(lightDir, normal));
   outColor = light * texture(texSampler, texCoord.xy);;
}