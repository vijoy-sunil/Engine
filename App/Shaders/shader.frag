/* The triangle that is formed by the positions from the vertex shader fills an area on the screen with fragments. The 
 * fragment shader is invoked on these fragments to produce a color and depth for the framebuffer (or framebuffers)
*/
#version 450
/* fragColor is an output from vertex shader, and an input to the fragment shader.
 * Note that the input variable does not necessarily have to use the same name, they will be linked together using the 
 * indexes specified by the location directives
*/
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

/* The main function is called for every fragment just like the vertex shader main function is called for every vertex
*/
void main (void) {
    /* Colors in GLSL are 4-component vectors with the R, G, B and alpha channels within the [0, 1] range. Unlike 
     * gl_Position in the vertex shader, there is no built-in variable to output a color for the current fragment. You 
     * have to specify your own output variable for each framebuffer where the layout(location = 0) modifier specifies the 
     * index of the framebuffer. The color red is written to this outColor variable that is linked to the first (and only) 
     * framebuffer at index 0.
     *
     * The values for fragColor will be automatically interpolated for the fragments between the three vertices, 
     * resulting in a smooth gradient
    */
    outColor = vec4 (fragColor, 1.0);
}