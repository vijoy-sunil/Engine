#ifndef VK_DESCRIPTOR_H
#define VK_DESCRIPTOR_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKUniformBuffer.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vector>

using namespace Collections;

namespace Renderer {
    /* We're now able to pass arbitrary attributes to the vertex shader for each vertex, but what about global variables?
     * (for example, the transformation matrix). We could include it as vertex data, but that's a waste of memory and it 
     * would require us to update the vertex buffer whenever the transformation changes. The transformation could easily 
     * change every single frame. This is where resource descriptors come in.
     * 
     * A descriptor is a way for shaders to freely access resources like buffers and images. There are many types of 
     * descriptors, (for example, uniform buffer objects (UBO)). The usage of descriptors consists of three parts:
     * 
     * (1) Specify a descriptor layout during pipeline creation
     * (2) Allocate a descriptor set from a descriptor pool
     * (3) Bind the descriptor set during rendering
    */
    class VKDescriptor: protected VKUniformBuffer {
        private:
            /* Descriptor bindings belonging to a set are combined into a single VkDescriptorSetLayout object
            */
            VkDescriptorSetLayout m_descriptorSetLayout;
            /* Handle to the descriptor pool
            */
            VkDescriptorPool m_descriptorPool;
            /* Handle to descriptor sets
            */
            std::vector <VkDescriptorSet> m_descriptorSets;
            /* Handle to the log object
            */
            static Log::Record* m_VKDescriptorLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++; 

        public:
            VKDescriptor (void) {
                m_VKDescriptorLog = LOG_INIT (m_instanceId, 
                                              static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                              Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                              "./Build/Log/");
            }

            ~VKDescriptor (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* (1)
             * The descriptor layout specifies the types of resources that are going to be accessed by the pipeline, just 
             * like a render pass specifies the types of attachments that will be accessed. We need to provide details 
             * about every descriptor binding used in the shaders for pipeline creation, just like we had to do for every 
             * vertex attribute and its location index, through a VkDescriptorSetLayoutBinding struct
            */
            void createDescriptorSetLayout (void) {
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                /* binding field specifies the binding number of this entry and corresponds to a resource of the same 
                 * binding number in the shader stages
                */
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                /* It is possible for the shader variable to represent an array of uniform buffer objects, and 
                 * descriptorCount specifies the number of values in the array. This could be used to specify a 
                 * transformation for each of the bones in a skeleton for skeletal animation, for example
                */
                uboLayoutBinding.descriptorCount = 1;
                /* We also need to specify in which shader stages the descriptor is going to be referenced. In our case, 
                 * we're only referencing the descriptor from the vertex shader. The stageFlags field can be a combination
                 * of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS
                */
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                /* The pImmutableSamplers field is only relevant for image sampling related descriptors
                */
                uboLayoutBinding.pImmutableSamplers = nullptr;

                /* We are now ready to create a descriptor set layout. The vkCreateDescriptorSetLayout function accepts a 
                 * VkDescriptorSetLayoutCreateInfo with the array of bindings
                */
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &uboLayoutBinding;

                VkResult result = vkCreateDescriptorSetLayout (getLogicalDevice(), 
                                                               &layoutInfo, 
                                                               nullptr, 
                                                               &m_descriptorSetLayout);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorLog) << "Failed to create descriptor set layout "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to create descriptor set layout");
                }

                /* Note that, we need to specify the descriptor set layout during pipeline creation to tell Vulkan which 
                 * descriptors the shaders will be using. Descriptor set layouts are specified in the pipeline layout 
                 * object
                */
            }

            /* Descriptor sets can't be created directly, they must be allocated from a pool like command buffers. The 
             * equivalent for descriptor sets is unsurprisingly called a descriptor pool
            */
            void createDescriptorPool (void) {
                /* We first need to describe which descriptor types our descriptor sets are going to contain and how many 
                 * of them. We will allocate one of these descriptors for every frame
                */
                VkDescriptorPoolSize poolSize{};
                poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSize.descriptorCount = static_cast <uint32_t> (MAX_FRAMES_IN_FLIGHT);

                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                /* Aside from the maximum number of individual descriptors that are available, we also need to specify 
                 * the maximum number of descriptor sets that may be allocated from the pool
                */
                poolInfo.maxSets = static_cast <uint32_t> (MAX_FRAMES_IN_FLIGHT);
                /* The structure has an optional flag similar to command pools that determines if individual descriptor 
                 * sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch 
                 * the descriptor set after creating it, so we don't need this flag. You can leave flags to its default 
                 * value of 0
                */
                poolInfo.flags = 0;

                VkResult result = vkCreateDescriptorPool (getLogicalDevice(), 
                                                          &poolInfo, 
                                                          nullptr, 
                                                          &m_descriptorPool);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorLog) << "Failed to create descriptor pool "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to create descriptor pool");
                }
            }

            /* (2)
             * A descriptor set specifies the actual buffer or image resources that will be bound to the descriptors, just 
             * like a framebuffer specifies the actual image views to bind to render pass attachments. In short, we will 
             * actually bind the VkBuffers to the uniform buffer descriptors so that the shader can access them. The 
             * descriptor set is then bound for the drawing commands just like the vertex buffers and framebuffer
            */
            void createDescriptorSets (void) {
                std::vector <VkDescriptorSetLayout> layouts (MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

                /* A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct. You need to 
                 * specify the descriptor pool to allocate from, the number of descriptor sets to allocate, and the 
                 * descriptor layout to base them on
                */
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool;
                allocInfo.descriptorSetCount = static_cast <uint32_t> (MAX_FRAMES_IN_FLIGHT);
                /* We will create one descriptor set for each frame in flight, all with the same layout. Unfortunately we 
                 * do need all the copies of the layout because the function expects an array matching the number of sets
                */
                allocInfo.pSetLayouts = layouts.data();

                m_descriptorSets.resize (MAX_FRAMES_IN_FLIGHT);
                /* The call to vkAllocateDescriptorSets will allocate descriptor sets, each with 'X' number of uniform 
                 * buffer descriptor ('X' specified during pool creation)
                */
                VkResult result = vkAllocateDescriptorSets (getLogicalDevice(), 
                                                            &allocInfo, 
                                                            m_descriptorSets.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorLog) << "Failed to allocate descriptor sets "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to allocate descriptor sets");
                }   

                /* The descriptor sets have been allocated now, but the descriptors within still need to be configured. 
                 * We'll now add a loop to populate every descriptor
                */        
                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    /* Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a 
                     * VkDescriptorBufferInfo struct. This structure specifies the buffer and the region within it that 
                     * contains the data for the descriptor
                    */
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.buffer = getUniformBuffers()[i];
                    bufferInfo.offset = 0;
                    /* If you're overwriting the whole buffer, like we are in this case, then it is also possible to use 
                     * the VK_WHOLE_SIZE value for the range
                    */
                    bufferInfo.range = sizeof (UniformBufferObject);

                    /* The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes 
                     * an array of VkWriteDescriptorSet structs as parameter
                    */
                    VkWriteDescriptorSet descriptorWrite{};
                    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    /* The two fields below specify the descriptor set to update and the binding. We gave our uniform 
                     * buffer binding index 0
                    */
                    descriptorWrite.dstSet = m_descriptorSets[i];
                    descriptorWrite.dstBinding = 0;
                    /* Remember that descriptors can be arrays, so we also need to specify the first index in the array 
                     * that we want to update. We're not using an array, so the index is simply 0
                    */
                    descriptorWrite.dstArrayElement = 0;
                    /* We need to specify the type of descriptor again. It's possible to update multiple descriptors at 
                     * once in an array, starting at index dstArrayElement. The descriptorCount field specifies how many 
                     * array elements you want to update
                    */
                    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrite.descriptorCount = 1;  
                    /* The pBufferInfo field is used for descriptors that refer to buffer data, pImageInfo is used for 
                     * descriptors that refer to image data, and pTexelBufferView is used for descriptors that refer to 
                     * buffer views
                    */
                    descriptorWrite.pBufferInfo = &bufferInfo;
                    descriptorWrite.pImageInfo = nullptr;
                    descriptorWrite.pTexelBufferView = nullptr;  

                    /* The updates are applied using vkUpdateDescriptorSets. It accepts two kinds of arrays as parameters, 
                     * an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet. The latter can be used to 
                     * copy descriptors to each other, as its name implies
                     * 
                     * Note that vkUpdateDescriptorSets doesn't copy a buffer into the descriptor set, but rather gives 
                     * the descriptor set a pointer to the buffer described by VkDescriptorBufferInfo. So then 
                     * vkUpdateDescriptorSets doesn't need to be called more than once for a descriptor set, since 
                     * modifying the buffer that a descriptor set points to will update what the descriptor set sees
                    */
                    vkUpdateDescriptorSets (getLogicalDevice(), 1, &descriptorWrite, 0, nullptr); 

                    /* All that remains is to update the recordCommandBuffer function to actually bind the right 
                     * descriptor set for each frame to the descriptors in the shader with vkCmdBindDescriptorSets
                    */                
                }
            }

            VkDescriptorSetLayout& getDescriptorSetLayout (void) {
                return m_descriptorSetLayout;
            }

            std::vector <VkDescriptorSet>& getDescriptorSets (void) {
                return m_descriptorSets;
            }

            void cleanUp (void) {
                /* You don't need to explicitly clean up descriptor sets, because they will be automatically freed when 
                 * the descriptor pool is destroyed
                */
                vkDestroyDescriptorPool (getLogicalDevice(), m_descriptorPool, nullptr);
                /* The descriptor layout should stick around while we may create new graphics pipelines i.e. until the 
                 * program ends
                */
                vkDestroyDescriptorSetLayout (getLogicalDevice(), m_descriptorSetLayout, nullptr);
            }
    };   

    Log::Record* VKDescriptor::m_VKDescriptorLog; 
}   // namespace Renderer
#endif  // VK_DESCRIPTOR_H