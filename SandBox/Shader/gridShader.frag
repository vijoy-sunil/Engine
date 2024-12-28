/* Reference: https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/
*/
#version 450

layout (location = 0) in  vec3 fragNearPoint;
layout (location = 1) in  vec3 fragFarPoint;
layout (location = 0) out vec4 outColor;
/* Since the plane is drawn on the entire viewport, we now have some problems when drawing other objets in the scene.
 * What we need to do to fix that is to manually calculate and output the depth for every fragment. To do this we are
 * going to need the view and projection matrix in fragment shader as well
 */
layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneData;

/* Use the scale variable to set the distance between the grid lines
*/
const float SCALE            = 1.0;
const float HIGHLIGHT_MARGIN = 1.0;
const float NEAR_PLANE       = 0.01;
const float FAR_PLANE        = 100.0;

/* To draw lines instead of just a uniform color, compute the 3D position on the actual xz plane using the near point and
 * far point calculated earlier and use that position to determine if the point is actually on a line or on the void of
 * the grid. For anti aliasing, we are going to use screen-space partial derivatives to compute the line width and
 * falloff
*/
vec4 grid (vec3 fragPosition, float scale, bool drawAxis) {
    vec2 coord      = fragPosition.xz * scale;
    vec2 derivative = fwidth (coord);
    vec2 grid       = abs (fract (coord - 0.5) - 0.5)/derivative;

    float line      = min (grid.x, grid.y);
    float minimumX  = min (derivative.x, 1);
    float minimumZ  = min (derivative.y, 1);

    vec4 color      = vec4 (0.1, 0.1, 0.1, 1.0 - min (line, 1.0));
    /* Axis highlight for X and Z axes
    */
    if (drawAxis) {
        if (fragPosition.z > -HIGHLIGHT_MARGIN * minimumZ &&
            fragPosition.z <  HIGHLIGHT_MARGIN * minimumZ)       color.x = 1.0;

        if (fragPosition.x > -HIGHLIGHT_MARGIN * minimumX &&
            fragPosition.x <  HIGHLIGHT_MARGIN * minimumX)       color.z = 1.0;
    }
    return color;
}

/* Manually calculate and output the depth for every fragment
*/
float computeDepth (vec3 fragPosition) {
    vec4 clipSpacePosition = sceneData.projectionMatrix *
                             sceneData.viewMatrix       *
                             vec4 (fragPosition, 1.0);
    return clipSpacePosition.z/clipSpacePosition.w;
}

/* Add a fading effect so that the grid looks a little better when it’s far away. To do this, we actually need to use
 * the linear depth to determine the alpha of the lines (the more far away they are, the more transparent they will be).
 * To get the linear depth we will need our near and far plane values
*/
float computeLinearDepth (vec3 fragPosition) {
    vec4 clipSpacePosition = sceneData.projectionMatrix *
                             sceneData.viewMatrix       *
                             vec4 (fragPosition, 1.0);
    /* Clip space depth between -1.0 and 1.0
    */
    float clipSpaceDepth   = (clipSpacePosition.z/clipSpacePosition.w) * 2.0 - 1.0;
    /* Linear depth between near plane and far plane values
    */
    float linearDepth      = (2.0 * FAR_PLANE * NEAR_PLANE)/
                             (FAR_PLANE + NEAR_PLANE - clipSpaceDepth * (FAR_PLANE - NEAR_PLANE));
    /* Normalize result
    */
    return linearDepth/FAR_PLANE;
}

void main (void) {
    /* Since the plane is now drawn on the entire viewport, we need to make some calculation to make sure it’s only
     * visible when needed, meaning we only want to see the plane on the floor (when y = 0). We are going to use the far
     * and near point calculated earlier to check if the plane intersects with the floor. Given the parametric equation
     * of a line,
     *              y = (far point[y] - near point[y]) * t + near point[y]
     *
     * If we isolate 't' and set y = 0,
     *              t = -near point[y]/(far point[y] - near point[y])
     *
     * Note that, we need the plane to be visible only when t > 0
    */
    float t           = -fragNearPoint.y/(fragFarPoint.y - fragNearPoint.y);

    vec3 fragPosition = (fragFarPoint - fragNearPoint) * t + fragNearPoint;
    gl_FragDepth      = computeDepth (fragPosition);

    float linearDepth = computeLinearDepth (fragPosition);
    float fading      = max (0, (0.5 - linearDepth));

    outColor          = (grid (fragPosition, SCALE, true)  +
                         grid (fragPosition, SCALE, true)) * float (t > 0);
    outColor.a       *= fading;
}