/* The geometry that is formed by the vertices from the vertex shader fills an area on the screen with fragments. The
 * fragment shader is invoked on these fragments to produce a color and depth for the frame buffer (or frame buffers)
*/
#version 450
/* This extension adds a "nonuniform" type qualifier and constructor, which is required by the Vulkan API to be used when
 * indexing descriptor bindings with an index that is not dynamically uniform. This extension also allows arrays of
 * resources declared using unsized arrays to become run-time sized arrays
 *
 * The 'require' behavior causes the named extension to work, if the implementation does not support the extension,
 * it fails
*/
#extension GL_EXT_nonuniform_qualifier: require
/* Note that the input variable does not necessarily have to use the same name, they will be linked together using the
 * indexes specified by the location directives
*/
layout (location = 0) in vec2 fragTexCoord;
/* In the general case, there is not a 1:1 mapping between a vertex and a fragment. By default, the associated data per
 * vertex is interpolated across the primitive to generate the corresponding associated data per fragment. Using the flat
 * keyword, no interpolation is done, so every fragment generated during the rasterization of that particular primitive
 * will get the same data. Since primitives are usually defined by more than one vertex, this means that the data from
 * only one vertex is used in that case (this is called the provoking vertex)
*/
layout (location = 1) flat in uint fragTexId;
layout (location = 0) out vec4 outColor;
/* A combined image sampler descriptor is represented in GLSL by a sampler* uniform (where * is the type of a texture,
 * such a 1D, 2D, Cube, etc.). Whereas, the GLSL type sampler (no *) represents a descriptor of the form
 * VK_DESCRIPTOR_TYPE_SAMPLER
 *
 * Run time sized array
 * An array whose size is specified in its declaration or determined by its initializer is 'explicitly-sized'. An array
 * whose size is not specified in a declaration is 'unsized'. Unsized arrays can either be implicitly sized or
 * run-time sized. A 'run-time sized' array has its size determined by a buffer or descriptor set bound via the API
 *
 * Note that, only the final binding in a descriptor set can have a variable size
*/
layout (set = 1, binding = 0) uniform sampler2D texSampler[];

/* The main function is called for every fragment just like the vertex shader main function is called for every vertex
*/
void main (void) {
    /* Colors in GLSL are 4-component vectors with the R, G, B and alpha channels within the [0, 1] range. Unlike
     * gl_Position in the vertex shader, there is no built-in variable to output a color for the current fragment. You
     * have to specify your own output variable for each frame buffer where the layout modifier specifies the index of
     * the frame buffer. The color is written to the variable that is linked to the frame buffer at index specified above
     *
     * The texture coordinate values will be smoothly interpolated across the area of the geometry by the rasterizer. We
     * can visualize this by having the fragment shader output the texture coordinates as colors. Note that, visualizing
     * data using colors is the shader programming equivalent of printf debugging, for lack of a better option. For
     * example, outColor = vec4 (fragTexCoord, 0.0, 1.0);
    */
    /* Textures are sampled using the built-in texture function. It takes a sampler and coordinate as arguments. The
     * sampler automatically takes care of the filtering and transformations in the background
    */
    outColor = texture (texSampler[fragTexId], fragTexCoord);
}