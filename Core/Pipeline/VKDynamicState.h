#ifndef VK_DYNAMIC_STATE_H
#define VK_DYNAMIC_STATE_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Core {
    class VKDynamicState: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKDynamicStateLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKDynamicState (void) {
                m_VKDynamicStateLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKDynamicState (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createDynamicState (uint32_t pipelineInfoId,  
                                     const std::vector <VkDynamicState>& dynamicStates) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* The graphics pipeline in Vulkan is almost completely immutable, so you must recreate the pipeline from 
                 * scratch if you want to change shaders, bind different framebuffers or change the blend function. The 
                 * disadvantage is that you'll have to create a number of pipelines that represent all of the different 
                 * combinations of states you want to use in your rendering operations. However, because all of the 
                 * operations you'll be doing in the pipeline are known in advance, the driver can optimize for it much 
                 * better
                 * 
                 * However, a limited amount of the state can actually be changed without recreating the pipeline at 
                 * draw time. Examples are the size of the viewport, line width and blend constants. If you want to use 
                 * dynamic state and keep these properties out, then you'll have to fill in a 
                 * VkPipelineDynamicStateCreateInfo structure
                 * 
                 * This will cause the configuration of these values to be ignored and you will be able (and required) 
                 * to specify the data at drawing time. This results in a more flexible setup and is very common for 
                 * things like viewport and scissor state.
                 * 
                 * Viewport
                 * A viewport basically describes the region of the framebuffer that the output will be rendered to. This 
                 * will almost always be (0, 0) to (width, height). Remember that the size of the swap chain and its 
                 * images may differ from the width and height of the window
                 * viewport.width  = static_cast <float> (swapChainExtent.width);
                 * viewport.height = static_cast <float> (swapChainExtent.height);
                 * 
                 * Scissor rectangle
                 * While viewports define the transformation from the image to the framebuffer, scissor rectangles define 
                 * in which regions pixels will actually be stored. Any pixels outside the scissor rectangles will be 
                 * discarded by the rasterizer. They function like a filter rather than a transformation. So if we wanted 
                 * to draw to the entire framebuffer, we would specify a scissor rectangle that covers it entirely
                 * 
                 * Dynamic state allows us set up the actual viewport(s) and scissor rectangle(s) up at drawing time
                */
                VkPipelineDynamicStateCreateInfo createInfo;
                createInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                createInfo.pNext             = VK_NULL_HANDLE;
                createInfo.flags             = 0;
                createInfo.dynamicStateCount = static_cast <uint32_t> (dynamicStates.size());
                createInfo.pDynamicStates    = dynamicStates.data();

                pipelineInfo->state.dynamicState = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_DYNAMIC_STATE_H