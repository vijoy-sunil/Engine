/* Reference: https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/
*/
#version 450

layout (location = 0) out vec3 fragNearPoint;
layout (location = 1) out vec3 fragFarPoint;

layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneData;

/* Coordinates in X-Y clipped space
*/
vec3 gridPlane[6] = vec3[](
    vec3 ( 1.0,  1.0, 0.0), vec3 (-1.0, -1.0, 0.0), vec3 (-1.0,  1.0, 0.0),
    vec3 (-1.0, -1.0, 0.0), vec3 ( 1.0,  1.0, 0.0), vec3 ( 1.0, -1.0, 0.0)
);

vec3 unProjectPoint (float x, float y, float z, mat4 viewMatrix, mat4 projectionMatrix) {
    mat4 viewInverse       = inverse (viewMatrix);
    mat4 projectionInverse = inverse (projectionMatrix);
    vec4 unProjectedPoint  = viewInverse * projectionInverse * vec4 (x, y, z, 1.0);

    return unProjectedPoint.xyz/unProjectedPoint.w;
}

void main (void) {
    /* We need to use the grid coordinate directly like if it was in clipped space coordinate and unproject it to get the
     * 3D world space coordinates. Since we want our points to be at infinity, we need to unproject it on the the near
     * (z = 0) and far (z = 1) planes. At this point, we will have an infinite plane covering the entire viewport
    */
    vec3 inPosition = gridPlane[gl_VertexIndex];
    gl_Position     = vec4 (inPosition, 1.0);

    fragNearPoint   = unProjectPoint (inPosition.x, inPosition.y, 0.0,
                                      sceneData.viewMatrix,
                                      sceneData.projectionMatrix);

    fragFarPoint    = unProjectPoint (inPosition.x, inPosition.y, 1.0,
                                      sceneData.viewMatrix,
                                      sceneData.projectionMatrix);
}