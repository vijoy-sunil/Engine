#version 450

layout (location = 0) in  vec3 inPosition;
layout (location = 0) out vec3 fragTexCoord;

layout (set = 0, binding = 0) uniform InstanceData {
    mat4 modelMatrix;
} instanceData;

layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneDataVert;

void main (void) {
    /* We want the sky box to be centered around the player so that no matter how far the player moves, the sky box
     * won't get any closer, giving the impression the surrounding environment is extremely large. The current view
     * matrix however transforms all the sky box's positions by rotating, scaling and translating them, so if the player
     * moves, the cube map moves as well. We want to remove the translation part of the view matrix so only rotation
     * will affect the sky box's position vectors
     *
     * Note that, we can remove the translation section of transformation matrices by taking the upper-left 3x3 matrix
     * of the 4x4 matrix. We can achieve this by converting the view matrix to a 3x3 matrix (removing translation) and
     * converting it back to a 4x4 matrix. This removes any translation, but keeps all rotation transformations so the
     * user can still look around the scene
    */
    mat4 viewMatrix  = mat4 (mat3 (sceneDataVert.viewMatrix));
    vec4 position    = sceneDataVert.projectionMatrix *
                       viewMatrix                     *
                       instanceData.modelMatrix       *
                       vec4 (inPosition, 1.0);
    /* Set the z component of the position vector equal to its w component which will result in a z component (depth)
     * that is always equal to 1.0
    */
    gl_Position      = position.xyww;
    /* A cube map used to texture a 3D cube can be sampled using the local positions of the cube as its texture
     * coordinates. When a cube is centered on the origin (0, 0, 0) each of its position vectors is also a direction
     * vector from the origin. This direction vector is exactly what we need to get the corresponding texture value at
     * that specific cube's position. For this reason we only need to supply position vectors and don't need texture
     * coordinates. Hence, we set the incoming local position vector as the outcoming texture coordinate for
     * (interpolated) use in the fragment shader. The fragment shader then takes these as input to sample a samplerCube
    */
    fragTexCoord     = inPosition;
    /* Convert cube map coordinates into vulkan coordinate space
    */
    fragTexCoord.y  *= -1.0;
}