#ifndef VK_COLOR_BLEND_H
#define VK_COLOR_BLEND_H

#include "VKPipelineMgr.h"

namespace Core {
    class VKColorBlend: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKColorBlendLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKColorBlend (void) {
                m_VKColorBlendLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKColorBlend (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* After a fragment shader has returned a color, it needs to be combined with the color that is already in 
             * the framebuffer. This transformation is known as color blending and there are two ways to do it:
             * (1) Mix the old and new value to produce a final color
             * (2) Combine the old and new value using a bitwise operation
             * 
             * There are two types of structs to configure color blending. The first struct, 
             * VkPipelineColorBlendAttachmentState contains the configuration per attached framebuffer and the second 
             * struct, VkPipelineColorBlendStateCreateInfo contains the global color blending settings
            */
            VkPipelineColorBlendAttachmentState getColorBlendAttachment (VkBool32 blendEnable) {
                VkPipelineColorBlendAttachmentState attachment;
                /* This per-framebuffer struct allows you to configure the first way of color blending (if set to true) 
                 * using the formula configured using the struct members. If blendEnable is set to VK_FALSE, then the new 
                 * color from the fragment shader is passed through unmodified
                */
                attachment.blendEnable = blendEnable;
                /* The formula:
                 * finalColor.rgb = 
                 * (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb)
                 * 
                 * finalColor.a = 
                 * (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a)
                 * 
                 * The resulting color is AND'd with the colorWriteMask to determine which channels are actually passed 
                 * through
                 * finalColor = finalColor & colorWriteMask;
                 * 
                 * The most common way to use color blending is to implement alpha blending, where we want the new color
                 * to be blended with the old color based on its opacity
                 * finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor
                 * finalColor.a   = newAlpha.a
                */
                attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
                attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; 
                attachment.colorBlendOp        = VK_BLEND_OP_ADD; 
                attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
                attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  
                attachment.alphaBlendOp        = VK_BLEND_OP_ADD;   
                attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | 
                                                 VK_COLOR_COMPONENT_G_BIT | 
                                                 VK_COLOR_COMPONENT_B_BIT | 
                                                 VK_COLOR_COMPONENT_A_BIT;
                return attachment;
            }

            void createColorBlendState (uint32_t pipelineInfoId,
                                        VkBool32 logicOpEnable,
                                        VkLogicOp logicOp,
                                        const std::vector <float>& blendConstants,
                                        const std::vector <VkPipelineColorBlendAttachmentState>& attachments) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* The second structure references the array of structures for all of the framebuffers and allows you to 
                 * set blend constants that you can use as blend factors between them
                */
                VkPipelineColorBlendStateCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* If you want to use the second method of blending (bitwise combination), then you should set 
                 * logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. Note that 
                 * this will automatically disable the first method, as if you had set blendEnable to VK_FALSE for every 
                 * attached framebuffer. However, the colorWriteMask will also be used in this mode to determine which 
                 * channels in the framebuffer will actually be affected
                */
                createInfo.logicOpEnable   = logicOpEnable;
                createInfo.logicOp         = logicOp; 
                createInfo.attachmentCount = static_cast <uint32_t> (attachments.size());
                createInfo.pAttachments    = attachments.data();
                /* blendConstants is a pointer to an array of four values used as the R, G, B, and A components of the 
                 * blend constant that are used in blending, depending on the blend factor
                */
                createInfo.blendConstants[0] = blendConstants[0]; 
                createInfo.blendConstants[1] = blendConstants[1]; 
                createInfo.blendConstants[2] = blendConstants[2]; 
                createInfo.blendConstants[3] = blendConstants[3];

                pipelineInfo->state.colorBlend = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_COLOR_BLEND_H