#ifndef VK_VIEW_PORT_H
#define VK_VIEW_PORT_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Renderer {
    class VKViewPort: protected virtual VKPipelineMgr {
        private:
            static Log::Record* m_VKViewPortLog;
            const size_t m_instanceId = g_collectionsId++;
            
        public:
            VKViewPort (void) {
                m_VKViewPortLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKViewPort (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createViewPortState (uint32_t pipelineInfoId) {
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the 
                 * VkPipelineViewportStateCreateInfo struct. This makes the viewport and scissor rectangle for this 
                 * pipeline immutable. Any changes required to these values would require a new pipeline to be created 
                 * with the new values
                 * 
                 * VkViewport viewport{};
                 * viewport.x        = 0.0f;
                 * viewport.y        = 0.0f;
                 * viewport.width    = static_cast <float> (swapChainExtent.width);
                 * viewport.height   = static_cast <float> (swapChainExtent.height);
                 * viewport.minDepth = 0.0f;
                 * viewport.maxDepth = 1.0f;
                 * 
                 * VkRect2D scissor{};
                 * scissor.offset = {0, 0};
                 * scissor.extent = swapChainExtent;
                 * 
                 * VkPipelineViewportStateCreateInfo viewportState{};
                 * viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                 * viewportState.viewportCount = 1;
                 * viewportState.pViewports    = &viewport;
                 * viewportState.scissorCount  = 1;
                 * viewportState.pScissors     = &scissor;
                 * 
                 * It's is possible to use multiple viewports and scissor rectangles on some graphics cards, so the 
                 * structure members reference an array of them
                */
                VkPipelineViewportStateCreateInfo createInfo{};
                createInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                createInfo.viewportCount = 1;
                createInfo.scissorCount  = 1;

                pipelineInfo->state.viewPort = createInfo;
            }
    };

    Log::Record* VKViewPort::m_VKViewPortLog;
}   // namespace Renderer
#endif  // VK_VIEW_PORT_H