/* The geometry that is formed by the positions from the vertex shader fills an area on the screen with fragments. The 
 * fragment shader is invoked on these fragments to produce a color and depth for the framebuffer (or framebuffers)
*/
#version 450
/* Note that the input variable does not necessarily have to use the same name, they will be linked together using the 
 * indexes specified by the location directives
*/
layout (location = 0) in  vec3 fragColor;
layout (location = 1) in  vec2 fragTexCoord;
layout (location = 0) out vec4 outColor;
/* A combined image sampler descriptor is represented in GLSL by a sampler uniform
*/
layout (binding = 1) uniform sampler2D texSampler;

/* The main function is called for every fragment just like the vertex shader main function is called for every vertex
*/
void main (void) {
    /* Colors in GLSL are 4-component vectors with the R, G, B and alpha channels within the [0, 1] range. Unlike 
     * gl_Position in the vertex shader, there is no built-in variable to output a color for the current fragment. You 
     * have to specify your own output variable for each framebuffer where the layout modifier specifies the index of 
     * the framebuffer. The color is written to the variable that is linked to the framebuffer at index specified above
     *
     * The values for fragColor will be automatically interpolated for the fragments resulting in a smooth gradient
     *
     * Just like the per vertex colors, the fragTexCoord values will be smoothly interpolated across the area of the 
     * geometry by the rasterizer. We can visualize this by having the fragment shader output the texture coordinates as 
     * colors. Note that, visualizing data using colors is the shader programming equivalent of printf debugging, for 
     * lack of a better option
     * outColor = vec4 (fragTexCoord, 0.0, 1.0);
    */

    /* Textures are sampled using the built-in texture function. It takes a sampler and coordinate as arguments. The 
     * sampler automatically takes care of the filtering and transformations in the background
    */
    outColor = texture (texSampler, fragTexCoord);
}