#version 450

layout (location = 0) in  vec3 inPosition;
layout (location = 0) out vec4 fragColor;

struct ModelInstanceDataSSBO {
    mat4 modelMatrix;
    mat4 normalMatrix;          /* Unused */

    uint diffuseTexIdLUT [64];
    uint specularTexIdLUT[64];  /* Unused */
    uint emissionTexIdLUT[64];  /* Unused */
};

layout (set = 0, binding = 0) readonly buffer ModelInstanceData {
    ModelInstanceDataSSBO instances[];
} modelInstanceData;

layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneDataVert;

void main (void) {
    gl_Position = sceneDataVert.projectionMatrix *
                  sceneDataVert.viewMatrix       *
                  modelInstanceData.instances[gl_InstanceIndex].modelMatrix *
                  vec4 (inPosition, 1.0);
    /* Decode color from packets
    */
    float r     = (modelInstanceData.instances[gl_InstanceIndex].diffuseTexIdLUT[0])/255.0;
    float g     = (modelInstanceData.instances[gl_InstanceIndex].diffuseTexIdLUT[1])/255.0;
    float b     = (modelInstanceData.instances[gl_InstanceIndex].diffuseTexIdLUT[2])/255.0;
    float a     = (modelInstanceData.instances[gl_InstanceIndex].diffuseTexIdLUT[3])/255.0;
    fragColor   = vec4 (r, g, b, a);
}