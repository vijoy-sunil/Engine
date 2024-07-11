#ifndef VK_PIPELINE_LAYOUT_H
#define VK_PIPELINE_LAYOUT_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Renderer {
    class VKPipelineLayout: protected virtual VKPipelineMgr {
        private:
            static Log::Record* m_VKPipelineLayoutLog;
            const size_t m_instanceId = g_collectionsId++;
            
        public:
            VKPipelineLayout (void) {
                m_VKPipelineLayoutLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKPipelineLayout (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createPipelineLayout (uint32_t pipelineInfoId) {      
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                auto deviceInfo   = getDeviceInfo();
                /* You can use uniform values in shaders, which are globals similar to dynamic state variables that can be 
                 * changed at drawing time to alter the behavior of your shaders without having to recreate them. They are 
                 * commonly used to pass the transformation matrix to the vertex shader, or to create texture samplers in 
                 * the fragment shader. Push constants are another way of passing dynamic values to shaders
                 * 
                 * These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout 
                 * object
                */
                VkPipelineLayoutCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                /* setLayoutCount is the number of descriptor sets included in the pipeline layout and pSetLayouts is a 
                 * pointer to an array of VkDescriptorSetLayout objects, meaning, it's possible to specify multiple 
                 * descriptor set layouts here
                 * 
                 * A use case would be, to put descriptors that vary per-object and descriptors that are shared into 
                 * separate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls
                 * which is potentially more efficient
                */
                createInfo.setLayoutCount         = static_cast <uint32_t> 
                                                    (pipelineInfo->resource.descriptorSetLayouts.size());
                createInfo.pSetLayouts            = pipelineInfo->resource.descriptorSetLayouts.data();
                createInfo.pushConstantRangeCount = static_cast <uint32_t> 
                                                    (pipelineInfo->resource.pushConstantRanges.size()); 
                createInfo.pPushConstantRanges    = pipelineInfo->resource.pushConstantRanges.data();

                VkPipelineLayout layout;
                VkResult result =  vkCreatePipelineLayout (deviceInfo->shared.logDevice, 
                                                           &createInfo, 
                                                           nullptr, 
                                                           &layout);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKPipelineLayoutLog) << "Failed to create pipeline layout " 
                                                      << "[" << pipelineInfoId << "]"
                                                      << " "
                                                      << "[" << string_VkResult (result) << "]" 
                                                      << std::endl;
                    throw std::runtime_error ("Failed to create pipeline layout");
                }
                pipelineInfo->resource.layout = layout;
            }
    };

    Log::Record* VKPipelineLayout::m_VKPipelineLayoutLog;
}   // namespace Renderer
#endif  // VK_PIPELINE_LAYOUT_H