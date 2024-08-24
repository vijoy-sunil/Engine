#ifndef VK_DESCRIPTOR_SET_LAYOUT_H
#define VK_DESCRIPTOR_SET_LAYOUT_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Core {
    class VKDescriptorSetLayout: protected virtual VKPipelineMgr {
        private:
            static Log::Record* m_VKDescriptorSetLayoutLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKDescriptorSetLayout (void) {
                m_VKDescriptorSetLayoutLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKDescriptorSetLayout (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkDescriptorSetLayoutBinding getLayoutBinding (uint32_t bindingNumber,
                                                           uint32_t descriptorCount,
                                                           VkDescriptorType descriptorType,
                                                           VkShaderStageFlags shaderStages,
                                                           const VkSampler* immutableSamplers) {
                VkDescriptorSetLayoutBinding layoutBinding;
                /* The binding field specifies the binding number of this entry and corresponds to a resource of the same 
                 * binding number in the shader stages
                */
                layoutBinding.binding = bindingNumber;
                /* It is possible for the shader variable to represent an array of descriptors, and descriptorCount 
                 * specifies the number of values in the array. This could be used to specify a transformation for each of
                 * the bones in a skeleton for skeletal animation, for example
                */
                layoutBinding.descriptorCount = descriptorCount;
                layoutBinding.descriptorType  = descriptorType;
                /* We also need to specify in which shader stages the descriptor is going to be referenced. The stageFlags
                 * field can be a combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS
                */
                layoutBinding.stageFlags = shaderStages;
                /* If descriptorType specifies a VK_DESCRIPTOR_TYPE_SAMPLER or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                 * type descriptor, then pImmutableSamplers can be used to initialize a set of immutable samplers. 
                 * Immutable samplers are permanently bound into the set layout and must not be changed
                 * 
                 * If pImmutableSamplers is not null, then it is a pointer to an array of sampler handles that will be 
                 * copied into the set layout and used for the corresponding binding. If pImmutableSamplers is null, then
                 * the sampler slots are dynamic and sampler handles must be bound into descriptor sets using this layout
                */
                layoutBinding.pImmutableSamplers = immutableSamplers;
                return layoutBinding;
            }

            /* The descriptor layout specifies the types of resources that are going to be accessed by the pipeline, just 
             * like a render pass specifies the types of attachments that will be accessed. We need to provide details 
             * about every descriptor binding used in the shaders for pipeline creation, just like we had to do for every 
             * vertex attribute and its location index, through a VkDescriptorSetLayoutBinding struct
            */
            void createDescriptorSetLayout (uint32_t pipelineInfoId,
                                            uint32_t deviceInfoId,
                                            const std::vector <VkDescriptorSetLayoutBinding>& layoutBindings,
                                            const std::vector <VkDescriptorBindingFlags>& bindingFlags,
                                            VkDescriptorSetLayoutCreateFlags layoutCreateFlags) {

                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                auto deviceInfo   = getDeviceInfo   (deviceInfoId);
                /* Specify descriptor set layout binding properties
                */
                VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo;
                bindingFlagsCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                bindingFlagsCreateInfo.pNext         = VK_NULL_HANDLE;
                bindingFlagsCreateInfo.pBindingFlags = bindingFlags.data();
                bindingFlagsCreateInfo.bindingCount  = static_cast <uint32_t> (bindingFlags.size());

                VkDescriptorSetLayoutCreateInfo createInfo;
                createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                createInfo.pNext        = &bindingFlagsCreateInfo;
                createInfo.flags        = layoutCreateFlags;
                createInfo.bindingCount = static_cast <uint32_t> (layoutBindings.size());
                createInfo.pBindings    = layoutBindings.data();

                VkDescriptorSetLayout descriptorSetLayout;
                VkResult result = vkCreateDescriptorSetLayout (deviceInfo->resource.logDevice, 
                                                               &createInfo, 
                                                               VK_NULL_HANDLE, 
                                                               &descriptorSetLayout);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDescriptorSetLayoutLog) << "Failed to create descriptor set layout "
                                                           << "[" << pipelineInfoId << "]"
                                                           << " "
                                                           << "[" << string_VkResult (result) << "]"
                                                           << std::endl;
                    throw std::runtime_error ("Failed to create descriptor set layout");
                }

                pipelineInfo->resource.descriptorSetLayouts.push_back (descriptorSetLayout);
                /* Note that, we need to specify the descriptor set layout during pipeline creation to tell Vulkan which 
                 * descriptors the shaders will be using. Descriptor set layouts are specified in the pipeline layout 
                 * object
                */
            }
    };

    Log::Record* VKDescriptorSetLayout::m_VKDescriptorSetLayoutLog;
}   // namespace Core
#endif  // VK_DESCRIPTOR_SET_LAYOUT_H