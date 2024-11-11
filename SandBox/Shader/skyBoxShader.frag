#version 450

layout (location = 0) in  vec3 fragTexCoord;
layout (location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform samplerCube texSamplerCubeMap;

void main (void) {
    outColor = texture (texSamplerCubeMap, fragTexCoord);
}