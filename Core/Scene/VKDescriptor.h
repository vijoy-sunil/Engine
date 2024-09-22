#ifndef VK_DESCRIPTOR_H
#define VK_DESCRIPTOR_H

#include "../Pipeline/VKPipelineMgr.h"
#include "VKSceneMgr.h"

namespace Core {
    /* We're now able to pass arbitrary attributes to the vertex shader for each vertex, but what about global variables?
     * (for example, the transformation matrix). We could include it as vertex data, but that's a waste of memory and it 
     * would require us to update the vertex buffer whenever the transformation changes. The transformation could easily 
     * change every single frame. This is where resource descriptors come in
     * 
     * A descriptor is a way for shaders to freely access resources like buffers and images. There are many types of 
     * descriptors, (for example, uniform buffer objects (UBO), combined image samplers etc.). The usage of descriptors 
     * consists of three parts:
     * 
     * (1) Specify a descriptor layout during pipeline creation
     * (2) Allocate a descriptor set from a descriptor pool
     * (3) Bind the descriptor set during rendering
    */
    class VKDescriptor: protected virtual VKPipelineMgr,
                        protected virtual VKSceneMgr {
        private:
            Log::Record* m_VKDescriptorLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++; 

        public:
            VKDescriptor (void) {
                m_VKDescriptorLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKDescriptor (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkDescriptorPoolSize getPoolSize (VkDescriptorType descriptorType,
                                              uint32_t descriptorCount) {
                
                VkDescriptorPoolSize poolSize;
                poolSize.type            = descriptorType;
                poolSize.descriptorCount = descriptorCount;
                return poolSize;
            }

            /* Descriptor sets can't be created directly, they must be allocated from a pool like command buffers. The 
             * equivalent for descriptor sets is unsurprisingly called a descriptor pool. A descriptor pool is a big heap
             * of available UBOs, textures, storage buffers, etc that can be used when instantiating descriptor sets. This
             * allows you to allocate a big heap of types ahead of time so that later on you don't have to ask the gpu to 
             * do expensive allocations
            */
            void createDescriptorPool (uint32_t deviceInfoId,
                                       uint32_t sceneInfoId,
                                       const std::vector <VkDescriptorPoolSize>& poolSizes,
                                       uint32_t maxDescriptorSets,
                                       VkDescriptorPoolCreateFlags poolCreateFlags) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);

                VkDescriptorPoolCreateInfo createInfo;
                createInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                createInfo.pNext         = VK_NULL_HANDLE;
                createInfo.flags         = poolCreateFlags;
                createInfo.poolSizeCount = static_cast <uint32_t> (poolSizes.size());
                createInfo.pPoolSizes    = poolSizes.data();
                /* Aside from the maximum number of individual descriptors that are available, we also need to specify 
                 * the maximum number of descriptor sets that may be allocated from the pool
                */
                createInfo.maxSets = maxDescriptorSets;

                /* Inadequate descriptor pools are a good example of a problem that the validation layers will not catch.
                 * As of Vulkan 1.1, vkAllocateDescriptorSets may fail with the error code VK_ERROR_POOL_OUT_OF_MEMORY if 
                 * the pool is not sufficiently large, but the driver may also try to solve the problem internally. This 
                 * means that sometimes (depending on hardware, pool size and allocation size) the driver will let us get 
                 * away with an allocation that exceeds the limits of our descriptor pool. Other times, 
                 * vkAllocateDescriptorSets will fail and return VK_ERROR_POOL_OUT_OF_MEMORY. This can be particularly 
                 * frustrating if the allocation succeeds on some machines, but fails on others
                 * 
                 * Since Vulkan shifts the responsiblity for the allocation to the driver, it is no longer a strict 
                 * requirement to only allocate as many descriptors of a certain type 
                 * (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, etc.) as specified by the corresponding descriptorCount 
                 * members for the creation of the descriptor pool. However, it remains best practise to do so
                */
                VkDescriptorPool descriptorPool;
                VkResult result = vkCreateDescriptorPool (deviceInfo->resource.logDevice, 
                                                          &createInfo, 
                                                          VK_NULL_HANDLE, 
                                                          &descriptorPool);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorLog) << "Failed to create descriptor pool "
                                                  << "[" << sceneInfoId << "]"
                                                  << " "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to create descriptor pool");
                }
                sceneInfo->resource.descriptorPool = descriptorPool;
            }

            /* A descriptor set specifies the actual buffer or image resources that will be bound to the descriptors, just 
             * like a frame buffer specifies the actual image views to bind to render pass attachments. In short, we will 
             * actually bind the resource to the  descriptors so that the shader can access them. The descriptor set is 
             * then bound for the drawing commands just like the vertex buffers and frame buffer
            */
            void createDescriptorSets (uint32_t deviceInfoId,
                                       uint32_t pipelineInfoId,
                                       uint32_t sceneInfoId,
                                       uint32_t descriptorSetLayoutId,
                                       uint32_t descriptorSetCount) {

                auto deviceInfo   = getDeviceInfo   (deviceInfoId);
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                auto sceneInfo    = getSceneInfo    (sceneInfoId);

                if (descriptorSetLayoutId >= pipelineInfo->resource.descriptorSetLayouts.size()) {
                    LOG_ERROR (m_VKDescriptorLog) << "Invalid descriptor set layout id " 
                                                  << "[" << descriptorSetLayoutId << "]"
                                                  << "->"
                                                  << "[" << pipelineInfo->resource.descriptorSetLayouts.size() << "]"
                                                  << std::endl; 
                    throw std::runtime_error ("Invalid descriptor set layout id");
                }
                /* A descriptor set layout defines the structure of a descriptor set, a template of sorts. Think of a 
                 * class or struct in C or C++, it says "I am made out of, 3 UBOs, a texture sampler, etc". It's analogous
                 * to going
                 * 
                 * struct MyDesc {
                 *      Buffer MyBuffer[3];
                 *      Texture MyTex;
                 * };
                 * 
                 * struct MyOtherDesc {
                 *      Buffer MyBuffer;
                 * };
                 * 
                 * Whereas, a descriptor set is an actual instance of a descriptor, as defined by a descriptor set layout.
                 * Using the class/struct analogy, it's like going MyDesc DescInstance();
                */
                std::vector <VkDescriptorSetLayout> layouts (descriptorSetCount, 
                                                    pipelineInfo->resource.descriptorSetLayouts[descriptorSetLayoutId]);

                /* A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct. You need to 
                 * specify the descriptor pool to allocate from, the number of descriptor sets to allocate, and the 
                 * descriptor layout to base them on
                */
                VkDescriptorSetAllocateInfo allocInfo;
                allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.pNext              = VK_NULL_HANDLE;
                allocInfo.descriptorPool     = sceneInfo->resource.descriptorPool;
                allocInfo.descriptorSetCount = descriptorSetCount;
                allocInfo.pSetLayouts        = layouts.data();

                std::vector <VkDescriptorSet> descriptorSets (descriptorSetCount);
                VkResult result = vkAllocateDescriptorSets (deviceInfo->resource.logDevice, 
                                                            &allocInfo, 
                                                            descriptorSets.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorLog) << "Failed to allocate descriptor sets "
                                                  << "[" << sceneInfoId << "]"
                                                  << " "
                                                  << "[" << pipelineInfoId << "]"
                                                  << " "
                                                  << "[" << descriptorSetLayoutId << "]"
                                                  << " "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to allocate descriptor sets");
                }   
                sceneInfo->resource.descriptorSets = descriptorSets;
            }

            /* Descriptors that refer to buffers, like a uniform buffer descriptor, are configured with a 
             * VkDescriptorBufferInfo struct. This structure specifies the buffer and the region within it that contains
             * the data for the descriptor
            */
            VkDescriptorBufferInfo getDescriptorBufferInfo (VkBuffer buffer,
                                                            VkDeviceSize offset, 
                                                            VkDeviceSize range) {
                
                VkDescriptorBufferInfo descriptorBufferInfo;
                descriptorBufferInfo.buffer = buffer;
                descriptorBufferInfo.offset = offset;
                /* If you're overwriting the whole buffer, like we are in this case, then it is also possible to use the 
                 * VK_WHOLE_SIZE value for the range
                */
                descriptorBufferInfo.range  = range;
                return descriptorBufferInfo;
            }

            /* Bind the actual image and sampler resources to the descriptors in the descriptor set. The resources for a 
             * combined image sampler structure, for example, must be specified in a VkDescriptorImageInfo struct, just 
             * like the buffer resource for a uniform buffer descriptor is specified in a VkDescriptorBufferInfo struct
            */
            VkDescriptorImageInfo getDescriptorImageInfo (VkSampler sampler,
                                                          VkImageView imageView, 
                                                          VkImageLayout imageLayout) {
                
                VkDescriptorImageInfo descriptorImageInfo;
                descriptorImageInfo.sampler     = sampler;
                descriptorImageInfo.imageView   = imageView;
                descriptorImageInfo.imageLayout = imageLayout;
                return descriptorImageInfo;
            }

            VkWriteDescriptorSet getWriteBufferDescriptorSetInfo (VkDescriptorType descriptorType,
                                                                  VkDescriptorSet descriptorSet,
                                                                  const std::vector <VkDescriptorBufferInfo>& 
                                                                                    descriptorInfos,
                                                                  uint32_t bindingNumber,
                                                                  uint32_t arrayElement,
                                                                  uint32_t descriptorCount) {

                VkWriteDescriptorSet writeDescriptorSet;
                writeDescriptorSet.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet.pNext      = VK_NULL_HANDLE;
                /* The two fields below specify the binding and the descriptor set to update
                */
                writeDescriptorSet.dstBinding      = bindingNumber;
                writeDescriptorSet.dstSet          = descriptorSet;
                writeDescriptorSet.dstArrayElement = arrayElement;
                /* We need to specify the type of descriptor again. It's possible to update multiple descriptors at once 
                 * in an array, starting at index dstArrayElement. The descriptorCount field specifies how many array 
                 * elements you want to update
                */
                writeDescriptorSet.descriptorType  = descriptorType;
                writeDescriptorSet.descriptorCount = descriptorCount;  
                /* The pBufferInfo field is used for descriptors that refer to buffer data, pImageInfo is used for 
                 * descriptors that refer to image data, and pTexelBufferView is used for descriptors that refer to buffer
                 * views
                */
                writeDescriptorSet.pBufferInfo      = descriptorInfos.data();
                writeDescriptorSet.pImageInfo       = VK_NULL_HANDLE;
                writeDescriptorSet.pTexelBufferView = VK_NULL_HANDLE;
                return writeDescriptorSet;
            }

            VkWriteDescriptorSet getWriteImageDescriptorSetInfo (VkDescriptorType descriptorType,
                                                                 VkDescriptorSet descriptorSet,
                                                                 const std::vector <VkDescriptorImageInfo>& 
                                                                                   descriptorInfos,
                                                                 uint32_t bindingNumber,
                                                                 uint32_t arrayElement,
                                                                 uint32_t descriptorCount) {
                VkWriteDescriptorSet writeDescriptorSet;
                writeDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet.pNext            = VK_NULL_HANDLE;
                writeDescriptorSet.dstBinding       = bindingNumber;
                writeDescriptorSet.dstSet           = descriptorSet;
                writeDescriptorSet.dstArrayElement  = arrayElement;
                writeDescriptorSet.descriptorType   = descriptorType;
                writeDescriptorSet.descriptorCount  = descriptorCount;  
                writeDescriptorSet.pBufferInfo      = VK_NULL_HANDLE;
                writeDescriptorSet.pImageInfo       = descriptorInfos.data();
                writeDescriptorSet.pTexelBufferView = VK_NULL_HANDLE;
                return writeDescriptorSet;
            }

            /* The descriptor sets have been allocated now, but the descriptors within still need to be configured
            */
            void updateDescriptorSets (uint32_t deviceInfoId, 
                                       const std::vector <VkWriteDescriptorSet>& writeDescriptorSets) {

                auto deviceInfo   = getDeviceInfo (deviceInfoId);
                /* The updates are applied using vkUpdateDescriptorSets. It accepts two kinds of arrays as parameters, 
                 * an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet. The latter can be used to 
                 * copy descriptors to each other, as its name implies
                 * 
                 * Note that vkUpdateDescriptorSets doesn't copy a buffer, for example, into the descriptor set, but 
                 * rather gives the descriptor set a pointer to the buffer described by VkDescriptorBufferInfo. So then 
                 * vkUpdateDescriptorSets doesn't need to be called more than once for a descriptor set, since 
                 * modifying the buffer that a descriptor set points to will update what the descriptor set sees
                */
                vkUpdateDescriptorSets (deviceInfo->resource.logDevice, 
                                        static_cast <uint32_t> (writeDescriptorSets.size()),
                                        writeDescriptorSets.data(), 
                                        0, 
                                        VK_NULL_HANDLE);                
            }

            void cleanUp (uint32_t deviceInfoId, uint32_t sceneInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);
                /* You don't need to explicitly clean up descriptor sets, because they will be automatically freed when 
                 * the descriptor pool is destroyed
                */
                vkDestroyDescriptorPool (deviceInfo->resource.logDevice, 
                                         sceneInfo->resource.descriptorPool, 
                                         VK_NULL_HANDLE);
            }
    };
}   // namespace Core
#endif  // VK_DESCRIPTOR_H