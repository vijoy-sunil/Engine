/* The vertex shader processes each incoming vertex. It takes its attributes, like world position, color, normal and 
 * texture coordinates as input. The output is the final position in clip coordinates and the attributes that need to be 
 * passed on to the fragment shader, like color and texture coordinates. These values will then be interpolated over the 
 * fragments by the rasterizer to produce a smooth gradient
 * 
 * clip coordinates: A clip coordinate is a four dimensional vector from the vertex shader that is subsequently turned 
 * into a normalized device coordinate by dividing the whole vector by its last component. These normalized device 
 * coordinates are homogeneous coordinates that map the framebuffer to a [-1, 1] by [-1, 1] coordinate system
 *
 * normalized device coordinate looks like this (ex: 2D):
 *                                  (-1, -1)-------------------------(1,-1)
 *                                          |                       |
 *                                          |        (0, 0)         |
 *                                          |                       |
 *                                  (-1, 1) -------------------------(1, 1)
 * you'll notice that the sign of the Y coordinates is now flipped
*/

/* The #version directive must appear before anything else in a shader, save for whitespace and comments
*/
#version 450

/* The vertex shader takes input from a vertex buffer using the in keyword. The inPosition and inColor variables are 
 * vertex attributes. They're properties that are specified per-vertex in the vertex buffer.
 *
 * Note that the vertex shader inputs can specify the 'attribute index' that the particular input uses, 
 * layout(location = attribute index) in vec3 position;
 * Whereas, the fragment shader outputs can specify the 'buffer index' that a particular output writes to,
 * layout(location = output index) out vec4 outColor;
*/
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

/* Add an output for color to the vertex shader
*/
layout(location = 0) out vec3 fragColor;

/* The main function is invoked for every vertex, the built-in gl_VertexIndex variable contains the index of the current 
 * vertex. This is usually an index into the vertex buffer, but in our case it will be an index into a hardcoded array of 
 * vertex data
*/
void main() {
    /* We can directly output normalized device coordinates by outputting them as clip coordinates from the vertex shader 
     * with the last component set to 1 using built-in variable gl_Position. That way the division to transform clip 
     * coordinates to normalized device coordinates will not change anything
    */
    gl_Position = vec4 (inPosition, 0.0, 1.0);
    fragColor = inColor;
}
