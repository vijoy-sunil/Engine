#version 450

layout (location = 0) in  vec3 inPosition;
layout (location = 0) out vec4 fragColor;

struct InstanceDataSSBO {
    mat4 modelMatrix;
    uint texIdLUT[64];
};

layout (set = 0, binding = 0) readonly buffer InstanceData {
    InstanceDataSSBO instances[];
} instanceData;

layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneDataVert;

void main (void) {
    gl_Position = sceneDataVert.projectionMatrix *
                  sceneDataVert.viewMatrix       *
                  instanceData.instances[gl_InstanceIndex].modelMatrix *
                  vec4 (inPosition, 1.0);
    /* Decode color from packets
    */
    float r     = (instanceData.instances[gl_InstanceIndex].texIdLUT[0])/255.0;
    float g     = (instanceData.instances[gl_InstanceIndex].texIdLUT[1])/255.0;
    float b     = (instanceData.instances[gl_InstanceIndex].texIdLUT[2])/255.0;
    float a     = (instanceData.instances[gl_InstanceIndex].texIdLUT[3])/255.0;
    fragColor   = vec4 (r, g, b, a);
}