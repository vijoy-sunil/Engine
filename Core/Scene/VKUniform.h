#ifndef VK_UNIFORM_H
#define VK_UNIFORM_H

#include <glm/glm.hpp>

namespace Renderer {
    /* Define the data we want the vertex shader to have in a C struct like below. This data will be copied to a VkBuffer
     * and accessible through a uniform buffer object descriptor from the vertex shader. We can exactly match the 
     * definition in the shader using data types in GLM. The data in the matrices is binary compatible with the way the 
     * shader expects it, so we can later just memcpy a UniformBufferObject to a VkBuffer
     * 
     * Alignment requirements specifies how exactly the data in the C++ structure should match with the uniform definition
     * in the shader. Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:
     * 
     * (1) Scalars have to be aligned by N (= 4 bytes given 32 bit floats)
     * (2) A vec2 must be aligned by 2N (= 8 bytes)
     * (3) A vec3 or vec4 must be aligned by 4N (= 16 bytes)
     * (4) A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16
     * (5) A mat4 matrix must have the same alignment as a vec4
     * 
     * An example to show where alignment requirement are met and not met:
     * 
     * A shader with just three mat4 fields already meets the alignment requirements 
     * struct UniformBufferObject {
     *      glm::mat4 model;
     *      glm::mat4 view;
     *      glm::mat4 proj;
     * };
     * 
     * As each mat4 is 4 x 4 x 4 = 64 bytes in size, model has an offset of 0, view has an offset of 64 and proj has an 
     * offset of 128. All of these are multiples of 16 and that's why it will work fine. Whereas the below struct fails
     * alignment requirements,
     * struct UniformBufferObject {
     *      glm::vec2 foo;
     *      glm::mat4 model;
     *      glm::mat4 view;
     *      glm::mat4 proj;
     * };
     * 
     * The new structure starts with a vec2 which is only 8 bytes in size and therefore throws off all of the offsets. 
     * Now model has an offset of 8, view an offset of 72 and proj an offset of 136, none of which are multiples of 16
     * 
     * To fix this problem we can use the alignas specifier introduced in C++11:
     * struct UniformBufferObject {
     *      glm::vec2 foo;
     *      alignas (16) glm::mat4 model;
     *      glm::mat4 view;
     *      glm::mat4 proj;
     * };
     * 
     * Luckily there is a way to not have to think about these alignment requirements most of the time. We can define 
     * GLM_FORCE_DEFAULT_ALIGNED_GENTYPES right before including GLM. This will force GLM to use a version of vec2 and 
     * mat4 that has the alignment requirements already specified for us. If you add this definition then you can remove 
     * the alignas specifier. Unfortunately this method can break down if you start using nested structures. Consider 
     * the following definition in the C++ code:
     * struct Foo {
     *      glm::vec2 v;
     * };
     *  
     * struct UniformBufferObject {
     *      Foo f1;
     *      Foo f2;
     * };
     * 
     * And the following shader definition:
     * struct Foo {
     *      vec2 v;
     * };
     *  
     * layout (binding = 0) uniform UniformBufferObject {
     *      Foo f1;
     *      Foo f2;
     * } ubo;
     * 
     * In this case f2 will have an offset of 8 whereas it should have an offset of 16 since it is a nested structure. In 
     * this case you must specify the alignment yourself
     * struct UniformBufferObject {
     *      Foo f1;
     *      alignas (16) Foo f2;
     * };
     * 
     * These gotchas are a good reason to always be explicit about alignment. That way you won't be caught offguard by 
     * the strange symptoms of alignment error
    */
    struct MVPMatrixUBO {
        alignas (16) glm::mat4 model;
        alignas (16) glm::mat4 view;
        alignas (16) glm::mat4 projection;
    };

    struct FragShaderVarsPC {
        /* This texture id variable cycles through an array of textures at certain interval and the fragment shader 
         * replaces a texture (for example, the default texture) by sampling from the array of textures indexed by this 
         * variable using push constants
        */
        uint32_t texId;
    };
}   // namespace Renderer
#endif  // VK_UNIFORM_H