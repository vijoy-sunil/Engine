/* The vertex shader processes each incoming vertex. It takes its attributes, like world position, color, normal and
 * texture coordinates as input. The output is the final position in clip coordinates and the attributes that need to be
 * passed on to the fragment shader, like color and texture coordinates. These values will then be interpolated over the
 * fragments by the rasterizer to produce a smooth gradient
 *
 * Coordinate systems
 * (1) Local space
 * Local coordinates are the coordinates of your object relative to its local origin; they're the coordinates your object
 * begins in. The next step is to transform the local coordinates to world-space coordinates which are coordinates in
 * respect of a larger world
 *
 * (2) World space
 * If we would import all our objects directly in the application they would probably all be somewhere positioned inside
 * each other at the world's origin of (0,0,0) which is not what we want. We want to define a position for each object to
 * position them inside a larger world. The coordinates in world space are exactly what they sound like: the coordinates
 * of all your vertices relative to a (game) world. This is the coordinate space where you want your objects transformed
 * to in such a way that they're all scattered around the place. The coordinates of your object are transformed from
 * local to world space; this is accomplished with the model matrix via chaging it's position, rotation and scale
 *
 * (3) View space
 * The view space is what people usually refer to as the camera (it is sometimes also known as camera space or eye space).
 * The view space is the result of transforming your world-space coordinates to coordinates that are in front of the
 * user's view. The view space is thus the space as seen from the camera's point of view. This is usually accomplished
 * with a combination of translations and rotations to translate/rotate the scene so that certain items are transformed
 * to the front of the camera. These combined transformations are generally stored inside a view matrix that transforms
 * world coordinates to view space
 *
 * (4) Clip space
 * A clip coordinate is a four dimensional vector from the vertex shader that is subsequently turned into a normalized
 * device coordinate by dividing the whole vector by its last component. At the end of each vertex shader run, coordinates
 * are expected to be within a specific range and any coordinate that falls outside this range is clipped. Coordinates
 * that are clipped are discarded, so the remaining coordinates will end up as fragments visible on your screen. This is
 * also where clip space gets its name from. Because specifying all the visible coordinates to be within the range -1.0
 * and 1.0 isn't really intuitive, we specify our own coordinate set to work in and convert those back to NDC (normalized
 * device coordinates are homogeneous coordinates that map the frame buffer to -1.0 and +1.0 for all axes)
 *
 * normalized device coordinate looks like this (ex: 2D):
 *                                  (-1, -1)-------------------------(1,-1)
 *                                          |                       |
 *                                          |        (0, 0)         |
 *                                          |                       |
 *                                  (-1, 1) -------------------------(1, 1)
 *
 * To transform vertex coordinates from view to clip-space we define a so called projection matrix that specifies a range
 * of coordinates e.g. -1000 and 1000 in each dimension. The projection matrix then converts coordinates within this
 * specified range to normalized device coordinates (not directly, a step called Perspective Division sits in between).
 * All coordinates outside this range will not be mapped between -1.0 and 1.0 and therefore be clipped. With this range
 * we specified in the projection matrix, a coordinate of (1250, 500, 750) would not be visible, since the x coordinate
 * is out of range and thus gets converted to a coordinate higher than 1.0 in NDC and is therefore clipped.
 *
 * This viewing box a projection matrix creates is called a frustum and each coordinate that ends up inside this frustum
 * will end up on the user's screen. The total process to convert coordinates within a specified range to NDC that can
 * easily be mapped to 2D view-space coordinates is called projection since the projection matrix projects 3D coordinates
 * to the easy-to-map-to-2D normalized device coordinates
 *
 * Perspective division
 * Once all the vertices are transformed to clip space a final operation called perspective division is performed where
 * we divide the x, y and z components of the position vectors by the vector's homogeneous w component; perspective
 * division is what transforms the 4D clip space coordinates to 3D normalized device coordinates. This step is performed
 * automatically at the end of the vertex shader step
 *
 * The 'w' value
 * The projection matrix maps a given frustum range to clip space, but also manipulates the w value of each vertex
 * coordinate in such a way that the further away a vertex coordinate is from the viewer, the higher this w component
 * becomes. Once the coordinates are transformed to clip space they are in the range -w to w (anything outside this range
 * is clipped). The visible coordinates are expected to fall between the range -1.0 and 1.0 as the final vertex shader
 * output, thus once the coordinates are in clip space, perspective division is applied to the clip space coordinates as
 * shown below
 *                          out = {x/w, y/w, z/w}
 *
 * Each component of the vertex coordinate is divided by its w component giving smaller vertex coordinates the further
 * away a vertex is from the viewer. The resulting coordinates are then in normalized device space
 *
 * (5) Screen space
 * The viewport information is then used to map the normalized-device coordinates to screen coordinates where each
 * coordinate corresponds to a point on your screen. This process is called the viewport transform
*/

/* The #version directive must appear before anything else in a shader, save for whitespace and comments
*/
#version 450
/* The vertex shader takes input from a vertex buffer using the in keyword. The input variables are the vertex attributes.
 * They're properties that are specified per-vertex in the vertex buffer
 *
 * Note that the vertex shader inputs can specify the 'attribute index' that the particular input uses. For example,
 * layout (location = attribute index) in vec3 inAttribute;
 *
 * Whereas, the fragment shader outputs can specify the 'buffer index' that a particular output writes to. For example,
 * layout (location = output index) out vec4 outColor;
*/
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inTexId;
/* Add outputs from the vertex shader
*/
layout (location = 0) out vec2 fragTexCoord;
layout (location = 1) out uint fragTexId;
/* Note that the order of the uniform, in and out declarations doesn't matter. The binding directive is similar to the
 * location directive for attributes. We're going to reference this binding in the descriptor layout
*/
struct InstanceDataSSBO {
    mat4 modelMatrix;
    mat4 texIdLUT;
};

layout (binding = 0) readonly buffer InstanceData {
    InstanceDataSSBO instances[];
} instanceData;

layout (push_constant) uniform SceneDataVertPC {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} sceneDataVert;

/* The main function is invoked for every vertex, the built-in gl_VertexIndex variable contains the index of the current
 * vertex. This is usually an index into the vertex buffer
*/
void main (void) {
    /* We can directly output normalized device coordinates by outputting them as clip coordinates from the vertex shader
     * with the last component set to 1 using built-in variable gl_Position. That way the division to transform clip
     * coordinates to normalized device coordinates will not change anything. However, the last component of the clip
     * coordinates may not be 1 after model transform calculations, which will result in a division when converted to
     * the final normalized device coordinates on the screen
    */
    gl_Position  = sceneDataVert.projectionMatrix *
                   sceneDataVert.viewMatrix       *
                   instanceData.instances[gl_InstanceIndex].modelMatrix *
                   vec4 (inPosition, 1.0);

    fragTexCoord = inTexCoord;

    uint rowIdx  = inTexId / 4;
    uint colIdx  = inTexId % 4;
    fragTexId    = uint (instanceData.instances[gl_InstanceIndex].texIdLUT[rowIdx][colIdx]);
}
