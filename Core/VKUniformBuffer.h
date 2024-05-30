#ifndef VK_UNIFORM_BUFFER_H
#define VK_UNIFORM_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
/* GLM library provides us with linear algebra related types like vectors and matrices. The GLM_FORCE_RADIANS definition 
 * is necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion
*/
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
/* The glm/gtc/matrix_transform.hpp header exposes functions that can be used to generate model transformations like 
 * glm::rotate, view transformations like glm::lookAt and projection transformations like glm::perspective
*/
#include <glm/gtc/matrix_transform.hpp>
#include "VKSwapChain.h"
#include "VKGenericBuffer.h"
#include "../Collections/Log/include/Log.h"
#include <vector>
/* The chrono standard library header exposes functions to do precise timekeeping
*/
#include <chrono>

using namespace Collections;

namespace Renderer {
    class VKUniformBuffer: protected virtual VKSwapChain,
                           protected virtual VKGenericBuffer {
        private:
            /* We should have multiple buffers, because multiple frames may be in flight at the same time and we don't 
             * want to update the buffer in preparation of the next frame while a previous one is still reading from it.
             * Thus, we need to have as many uniform buffers as we have frames in flight, and write to a uniform buffer 
             * that is not currently being read by the GPU
            */
            std::vector <VkBuffer> m_uniformBuffers;
            std::vector <VkDeviceMemory> m_uniformBuffersMemory;
            std::vector <void*> m_uniformBuffersMapped;
            /* Handle to the log object
            */
            static Log::Record* m_VKUniformBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 26;

        public:
            VKUniformBuffer (void) {
                m_VKUniformBufferLog = LOG_INIT (m_instanceId, 
                                                 static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                                 Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                 "./Build/Log/");
            }

            ~VKUniformBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Define the data we want the vertex shader to have in a C struct like below. This data will be copied to a 
             * VkBuffer and accessible through a uniform buffer object descriptor from the vertex shader. We can exactly 
             * match the definition in the shader using data types in GLM. The data in the matrices is binary compatible 
             * with the way the shader expects it, so we can later just memcpy a UniformBufferObject to a VkBuffer
             * 
             * Alignment requirements specifies how exactly the data in the C++ structure should match with the uniform 
             * definition in the shader. Vulkan expects the data in your structure to be aligned in memory in a specific 
             * way, for example:
             * 
             * (1) Scalars have to be aligned by N (= 4 bytes given 32 bit floats)
             * (2) A vec2 must be aligned by 2N (= 8 bytes)
             * (3) A vec3 or vec4 must be aligned by 4N (= 16 bytes)
             * (4) A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16
             * (5) A mat4 matrix must have the same alignment as a vec4
             * 
             * An example to show where alignment requirement are met and not met:
             * 
             * A shader with just three mat4 fields already meets the alignment requirements. 
             * struct UniformBufferObject {
             *      glm::mat4 model;
             *      glm::mat4 view;
             *      glm::mat4 proj;
             * };
             * 
             * As each mat4 is 4 x 4 x 4 = 64 bytes in size, model has an offset of 0, view has an offset of 64 and proj 
             * has an offset of 128. All of these are multiples of 16 and that's why it will work fine. Whereas the
             * below struct fails alignment requirements,
             * struct UniformBufferObject {
             *      glm::vec2 foo;
             *      glm::mat4 model;
             *      glm::mat4 view;
             *      glm::mat4 proj;
             * };
             * 
             * The new structure starts with a vec2 which is only 8 bytes in size and therefore throws off all of the 
             * offsets. Now model has an offset of 8, view an offset of 72 and proj an offset of 136, none of which are 
             * multiples of 16
             * 
             * To fix this problem we can use the alignas specifier introduced in C++11:
             * struct UniformBufferObject {
             *      glm::vec2 foo;
             *      alignas (16) glm::mat4 model;
             *      glm::mat4 view;
             *      glm::mat4 proj;
             * };
             * 
             * Luckily there is a way to not have to think about these alignment requirements most of the time. We can 
             * define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES right before including GLM. This will force GLM to use a version 
             * of vec2 and mat4 that has the alignment requirements already specified for us. If you add this definition 
             * then you can remove the alignas specifier. Unfortunately this method can break down if you start using 
             * nested structures. Consider the following definition in the C++ code:
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
             * In this case f2 will have an offset of 8 whereas it should have an offset of 16 since it is a nested 
             * structure. In this case you must specify the alignment yourself
             * struct UniformBufferObject {
             *      Foo f1;
             *      alignas (16) Foo f2;
             * };
             * 
             * These gotchas are a good reason to always be explicit about alignment. That way you won't be caught 
             * offguard by the strange symptoms of alignment error
            */
            struct UniformBufferObject {
                alignas (16) glm::mat4 model;
                alignas (16) glm::mat4 view;
                alignas (16) glm::mat4 proj;
            };
            
            void createUniformBuffers (void) {
                VkDeviceSize bufferSize = sizeof (UniformBufferObject);

                m_uniformBuffers.resize (MAX_FRAMES_IN_FLIGHT);
                m_uniformBuffersMemory.resize (MAX_FRAMES_IN_FLIGHT);
                m_uniformBuffersMapped.resize (MAX_FRAMES_IN_FLIGHT);

                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    /* We're going to copy new data to the uniform buffer every frame, so it doesn't really make any 
                     * sense to have a staging buffer. It would just add extra overhead in this case and likely degrade 
                     * performance instead of improving it
                    */
                    createGenericBuffer (bufferSize, 
                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                         m_uniformBuffers[i], 
                                         m_uniformBuffersMemory[i]);

                    /* We map the buffer right after creation using vkMapMemory to get a pointer to which we can write 
                     * the data later on. The buffer stays mapped to this pointer for the application's whole lifetime. 
                     * This technique is called "persistent mapping" and works on all Vulkan implementations. Not having 
                     * to map the buffer every time we need to update it increases performances, as mapping is not free
                    */
                    vkMapMemory (getLogicalDevice(), 
                                 m_uniformBuffersMemory[i], 
                                 0, 
                                 bufferSize, 
                                 0, 
                                 &m_uniformBuffersMapped[i]);
                }
            }

            /* Example: We're going to update the model, view and projection matrices every frame to make the geometry 
             * formed spin around in 3D
            */
            void updateUniformBuffer (uint32_t currentFrame) {
                /* Calculate the time in seconds since rendering has started with floating point accuracy
                */
                static auto startTime = std::chrono::high_resolution_clock::now();

                auto currentTime = std::chrono::high_resolution_clock::now();
                float time = std::chrono::duration <float, std::chrono::seconds::period> 
                             (currentTime - startTime).count();

                /* We will now define the model, view and projection transformations in the uniform buffer object. The 
                 * model rotation will be a simple rotation around the Z-axis using the time variable
                */
                UniformBufferObject ubo{};
                /* The glm::rotate function takes an existing transformation (we use an identity matrix since the geometry
                 * will be at the origin), rotation angle and rotation axis as parameters. The glm::mat4(1.0f) 
                 * constructor returns an identity matrix. Using a rotation angle of time * glm::radians(90.0f) 
                 * accomplishes the purpose of rotation 90 degrees per second
                */
                ubo.model = glm::rotate (glm::mat4 (1.0f), 
                                         time * glm::radians (90.0f), 
                                         glm::vec3 (0.0f, 0.0f, 1.0f));
                /* For the view transformation we've decided to look at the geometry from above at a 45 degree angle. 
                 * The glm::lookAt function takes the eye (camera) position, where you want to look at, in world space, 
                 * and up axis as parameters
                */
                ubo.view = glm::lookAt (glm::vec3 (2.0f, 2.0f, 2.0f), 
                                        glm::vec3 (0.0f, 0.0f, 0.0f), 
                                        glm::vec3 (0.0f, 0.0f, 1.0f));
                /* We will use a perspective projection with a 45 degree vertical field-of-view. The other parameters 
                 * are the aspect ratio, near and far view planes. It is important to use the current swap chain extent 
                 * to calculate the aspect ratio to take into account the new width and height of the window after a 
                 * resize
                */
                float aspectRatio = getSwapChainExtent().width / static_cast <float> (getSwapChainExtent().height);
                ubo.proj = glm::perspective (glm::radians (45.0f), 
                                             aspectRatio, 
                                             0.1f, 
                                             10.0f);
                /* GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
                 * The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the 
                 * projection matrix. If you don't do this, then the image will be rendered upside down
                 * 
                 * OpenGL                                       Vulkan
                 *              +Y                              -Y
                 *              |                               |
                 *              |                               |
                 *              |                               |
                 *              |-----------| +X                |-----------| +X
                 *             /                               /
                 *            /                               /
                 *           /                               /
                 *          +Z                              -Z
                 * Note that, because of the Y-flip we did in the projection matrix, the vertices are now being drawn in 
                 * counter-clockwise order instead of clockwise order. This causes backface culling to kick in and 
                 * prevents any geometry from being drawn. Go to the createGraphicsPipeline function and modify the 
                 * frontFace in VkPipelineRasterizationStateCreateInfo to correct this
                */
                ubo.proj[1][1] *= -1;

                /* All of the transformations are defined now, so we can copy the data in the uniform buffer object to 
                 * the current uniform buffer. This happens in exactly the same way as we did for vertex buffers, except 
                 * without a staging buffer. As noted earlier, we only map the uniform buffer once, so we can directly 
                 * write to it without having to map again
                */
                memcpy (m_uniformBuffersMapped[currentFrame], &ubo, sizeof (ubo));
                
                /* Note that, using an UBO this way may not be the most efficient way to pass frequently changing values 
                 * to the shader. A more efficient way to pass a small buffer of data to shaders are push constants
                */
            }

            std::vector <VkBuffer>& getUniformBuffers (void) {
                return m_uniformBuffers;
            }

            void cleanUp (void) {
                /* The uniform data will be used for all draw calls, so the buffer containing it should only be destroyed 
                 * when we stop rendering
                */
                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    vkDestroyBuffer (getLogicalDevice(), m_uniformBuffers[i], nullptr);
                    vkFreeMemory (getLogicalDevice(), m_uniformBuffersMemory[i], nullptr);
                }
            }
    };

    Log::Record* VKUniformBuffer::m_VKUniformBufferLog;
}   // namespace Renderer
#endif  // VK_UNIFORM_BUFFER_H