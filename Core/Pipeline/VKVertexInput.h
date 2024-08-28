#ifndef VK_VERTEX_INPUT_H
#define VK_VERTEX_INPUT_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Core {
    class VKVertexInput: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKVertexInputLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKVertexInput (void) {
                m_VKVertexInputLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKVertexInput (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createVertexInputState (uint32_t pipelineInfoId, 
                                         const std::vector <VkVertexInputBindingDescription>& bindingDescriptions,
                                         const std::vector <VkVertexInputAttributeDescription>& attributeDescriptions) {
                                        
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be 
                 * passed to the vertex shader. It describes this in roughly two ways:
                 * 
                 * Bindings: spacing between data and whether the data is per-vertex or per-instance (instancing is the 
                 * practice of rendering multiple copies of the same mesh in a scene at once. This technique is primarily 
                 * used for objects such as trees, grass, or buildings which can be represented as repeated geometry 
                 * without appearing unduly repetitive)
                 * 
                 * Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them 
                 * from and at which offset
                */
                VkPipelineVertexInputStateCreateInfo createInfo;
                createInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                createInfo.pNext                           = VK_NULL_HANDLE;
                createInfo.flags                           = 0;
                createInfo.vertexBindingDescriptionCount   = static_cast <uint32_t> (bindingDescriptions.size());
                createInfo.pVertexBindingDescriptions      = bindingDescriptions.data();
                createInfo.vertexAttributeDescriptionCount = static_cast <uint32_t> (attributeDescriptions.size());
                createInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();   

                pipelineInfo->state.vertexInput = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_VERTEX_INPUT_H