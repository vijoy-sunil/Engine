#ifndef VK_DEPTH_STENCIL_H
#define VK_DEPTH_STENCIL_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Renderer {
    class VKDepthStencil: protected virtual VKPipelineMgr {
        private:
            static Log::Record* m_VKDepthStencilLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKDepthStencil (void) {
                m_VKDepthStencilLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKDepthStencil (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createDepthStencilState (uint32_t pipelineInfoId,
                                          VkBool32 depthTestEnable,
                                          VkBool32 depthWriteEnable,
                                          VkBool32 depthBoundsTestEnable,
                                          float minDepthBounds, 
                                          float maxDepthBounds,
                                          VkBool32 stencilTestEnable,
                                          VkStencilOpState front,
                                          VkStencilOpState back) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);

                VkPipelineDepthStencilStateCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                /* The depthTestEnable field specifies if the depth of new fragments should be compared to the depth 
                 * buffer to see if they should be discarded. The depthWriteEnable field specifies if the new depth of 
                 * fragments that pass the depth test should actually be written to the depth buffer
                */
                createInfo.depthTestEnable  = depthTestEnable;
                createInfo.depthWriteEnable = depthWriteEnable;       
                /* The depthCompareOp field specifies the comparison that is performed to keep or discard fragments. We
                 * are sticking to the convention of lower depth = closer, so the depth of new fragments should be less
                */
                createInfo.depthCompareOp = VK_COMPARE_OP_LESS;  
                /* The depthBoundsTestEnable, minDepthBounds and maxDepthBounds fields are used for the optional depth 
                 * bound test. Basically, this allows you to only keep fragments that fall within the specified depth 
                 * range
                */
                createInfo.depthBoundsTestEnable = depthBoundsTestEnable;
                createInfo.minDepthBounds        = minDepthBounds;
                createInfo.maxDepthBounds        = maxDepthBounds;   
                /* The last three fields configure stencil buffer operations. If you want to use these operations, then 
                 * you will have to make sure that the format of the depth/stencil image contains a stencil component
                */
                createInfo.stencilTestEnable = stencilTestEnable;
                createInfo.front             = front;
                createInfo.back              = back;

                pipelineInfo->state.depthStencil = createInfo;
            }
    };

    Log::Record* VKDepthStencil::m_VKDepthStencilLog;
}   // namespace Renderer
#endif  // VK_DEPTH_STENCIL_H