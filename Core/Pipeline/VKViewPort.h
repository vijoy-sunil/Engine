#ifndef VK_VIEW_PORT_H
#define VK_VIEW_PORT_H

#include "VKPipelineMgr.h"

namespace Core {
    class VKViewPort: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKViewPortLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKViewPort (void) {
                m_VKViewPortLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
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
                 * with the new values. Note that, it's is possible to use multiple viewports and scissor rectangles on 
                 * some graphics cards, so the structure members reference an array of them
                */
                VkPipelineViewportStateCreateInfo createInfo;
                createInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                createInfo.pNext         = VK_NULL_HANDLE;
                createInfo.flags         = 0;
                createInfo.viewportCount = 1;
                createInfo.pViewports    = VK_NULL_HANDLE;
                createInfo.scissorCount  = 1;
                createInfo.pScissors     = VK_NULL_HANDLE;

                pipelineInfo->state.viewPort = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_VIEW_PORT_H