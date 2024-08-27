#ifndef VK_UNIFORM_H
#define VK_UNIFORM_H

#include <glm/glm.hpp>

namespace Core {
    /* Alignment requirements specifies how exactly the data in the C++ structure should match with the uniform definition
     * in the shader. We can exactly match the definition in the shader using data types in GLM. The data in the matrices,
     * for example is binary compatible with the way the shader expects it, so we can later just memcpy this data 
     * structure to a VkBuffer. Vulkan expects the data in your structure to be aligned in memory in a specific way, 
     * for example:
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
     * 
     * When declaring UBOs/SSBOs, pretend that all 3-element vector types don't exist. This includes column-major 
     * matrices with 3 rows or row-major matrices with 3 columns. Pretend that the only types are scalars, 2, and 4 
     * element vectors (and matrices)
     * 
     * Reference: https://stackoverflow.com/questions/38172696/should-i-ever-use-a-vec3-inside-of-a-uniform-buffer-or-
     * shader-storage-buffer-o
     * 
     * Layout standards std140 vs std430
     * std430 - the default for push constants
     * std140 - the default for uniform buffers
     * 
     * Among the most important difference between these two standards is the fact that, in std140, arrays of types are 
     * not necessarily tightly packed. An array of floats will not be the equivalent to an array of floats in C/C++. The 
     * array stride (the bytes between array elements) is always rounded up to the size of a vec4 (ie: 16-bytes). So 
     * arrays will only match their C/C++ definitions if the type is a multiple of 16 bytes
     * 
     * For example, a mat3 may be padded internally to take 12 floats of space arranged as 
     * [x0, y0, z0, pad][x1, y1, z1, pad][x2, y2, z2, pad]
    */
    struct InstanceDataSSBO {
        glm::mat4 modelMatrix;
        alignas (16) glm::mat4 texIdLUT;
    };

    struct SceneDataVertPC {
        glm::mat4 viewMatrix;
        alignas (16) glm::mat4 projectionMatrix;  
    };
}   // namespace Core
#endif  // VK_UNIFORM_H