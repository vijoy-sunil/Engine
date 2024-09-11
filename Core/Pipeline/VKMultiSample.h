#ifndef VK_MULTI_SAMPLE_H
#define VK_MULTI_SAMPLE_H

#include "VKPipelineMgr.h"
#include "../Image/VKImageMgr.h"

namespace Core {
    class VKMultiSample: protected virtual VKPipelineMgr,
                         protected virtual VKImageMgr {
        private:
            Log::Record* m_VKMultiSampleLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKMultiSample (void) {
                m_VKMultiSampleLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKMultiSample (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createMultiSampleState (uint32_t imageInfoId, 
                                         uint32_t pipelineInfoId,
                                         VkBool32 sampleShadingEnable, 
                                         float minSampleShading) {

                auto imageInfo    = getImageInfo    (imageInfoId, MULTISAMPLE_IMAGE);                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);

                VkPipelineMultisampleStateCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* There are certain limitations of our current MSAA implementation which may impact the quality of the 
                 * output image in more detailed scenes. For example, we're currently not solving potential problems 
                 * caused by shader aliasing, i.e. MSAA only smoothens out the edges of geometry but not the interior 
                 * filling. This may lead to a situation when you get a smooth polygon rendered on screen but the applied 
                 * texture will still look aliased if it contains high contrasting colors. One way to approach this 
                 * problem is to enable sample shading which will improve the image quality even further, though at an 
                 * additional performance cost
                 * 
                 * Note that, we need to enable sample shading when creating the logic device in addition to enabling it
                 * in the pipeline
                */
                createInfo.sampleShadingEnable   = sampleShadingEnable;
                /* Set min fraction for sample shading. Note that value closer to one makes it smoother
                */
                createInfo.minSampleShading      = minSampleShading;
                createInfo.rasterizationSamples  = imageInfo->params.sampleCount;
                createInfo.pSampleMask           = VK_NULL_HANDLE; 
                createInfo.alphaToCoverageEnable = VK_FALSE; 
                createInfo.alphaToOneEnable      = VK_FALSE; 

                pipelineInfo->state.multiSample  = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_MULTI_SAMPLE_H