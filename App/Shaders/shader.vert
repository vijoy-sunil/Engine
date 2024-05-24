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

/* Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you 
 * can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices (see pipeline)
*/
vec2 positions[3] = vec2[](
    vec2 (0.0, -0.5),
    vec2 (0.5, 0.5),
    vec2 (-0.5, 0.5)
);

/* Specify a distinct color for each of the three vertices
*/
vec3 colors[3] = vec3[](
    vec3 (1.0, 0.0, 0.0),
    vec3 (0.0, 1.0, 0.0),
    vec3 (0.0, 0.0, 1.0)
);

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
    gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
