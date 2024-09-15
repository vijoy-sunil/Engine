#ifndef VK_ATTACHMENT_H
#define VK_ATTACHMENT_H

#include "VKRenderPassMgr.h"
#include "../Image/VKImageMgr.h"

namespace Core {
    class VKAttachment: protected virtual VKRenderPassMgr,
                        protected virtual VKImageMgr {
        private:
            Log::Record* m_VKAttachmentLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKAttachment (void) {
                m_VKAttachmentLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKAttachment (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* The VkAttachmentReference does not reference the attachment object directly, it references the index in 
             * the attachments array specified in VkRenderPassCreateInfo. This allows different subpasses to reference the 
             * same attachment
             *
             * The layout specifies which layout we would like the attachment to have during a subpass that uses this 
             * reference. Vulkan will automatically transition the attachment to this layout when the subpass is started
            */
            VkAttachmentReference getAttachmentReference (uint32_t attachmentIndex, VkImageLayout layout) {
                VkAttachmentReference attachmentReference;
                attachmentReference.attachment = attachmentIndex;
                attachmentReference.layout     = layout;
                return attachmentReference;
            }
            
            /* Attachments are "offscreen" rendering targets. All this means is that instead of making your picture appear
             * on your display, you render it to some other place -- an FBO. Before you can do this, you have to create 
             * and configure the FBO. Part of that configuration is adding a color attachment -- a buffer to hold the 
             * per-pixel color information of the rendered picture. Maybe you stop there, or maybe you also add a depth 
             * attachment. If you are rendering 3D geometry, and you want it to look correct, you'll likely have to add 
             * this depth attachment
             * 
             * Note that, multisampled images cannot be presented directly. We first need to resolve them to a regular
             * image
            */
            void createMultiSampleAttachment (uint32_t imageInfoId, uint32_t renderPassInfoId) {
                auto imageInfo      = getImageInfo (imageInfoId, MULTISAMPLE_IMAGE);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkAttachmentDescription attachment;
                attachment.flags          = g_renderPassSettings.multiSampleAttachment.flags;
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = g_renderPassSettings.multiSampleAttachment.loadOp;
                attachment.storeOp        = g_renderPassSettings.multiSampleAttachment.storeOp;
                attachment.stencilLoadOp  = g_renderPassSettings.multiSampleAttachment.stencilLoadOp;
                attachment.stencilStoreOp = g_renderPassSettings.multiSampleAttachment.stencilStoreOp;
                attachment.initialLayout  = g_renderPassSettings.multiSampleAttachment.initialLayout;
                attachment.finalLayout    = g_renderPassSettings.multiSampleAttachment.finalLayout;

                renderPassInfo->resource.attachments.push_back (attachment);
            }

            void createDepthStencilAttachment (uint32_t imageInfoId, uint32_t renderPassInfoId) {
                auto imageInfo      = getImageInfo (imageInfoId, DEPTH_IMAGE);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                /* The format should be the same as the depth image itself. We don't care about storing the depth data 
                 * (storeOp), because it will not be used after drawing has finished. This may allow the hardware to 
                 * perform additional optimizations
                */
                VkAttachmentDescription attachment;
                attachment.flags          = g_renderPassSettings.depthStencilAttachment.flags;
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = g_renderPassSettings.depthStencilAttachment.loadOp;
                attachment.storeOp        = g_renderPassSettings.depthStencilAttachment.storeOp;
                attachment.stencilLoadOp  = g_renderPassSettings.depthStencilAttachment.stencilLoadOp;
                attachment.stencilStoreOp = g_renderPassSettings.depthStencilAttachment.stencilStoreOp;
                attachment.initialLayout  = g_renderPassSettings.depthStencilAttachment.initialLayout;
                attachment.finalLayout    = g_renderPassSettings.depthStencilAttachment.finalLayout; 

                renderPassInfo->resource.attachments.push_back (attachment);
            }

            void createColorAttachment (uint32_t imageInfoId, uint32_t renderPassInfoId) {
                auto imageInfo      = getImageInfo (imageInfoId, SWAPCHAIN_IMAGE);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkAttachmentDescription attachment;
                attachment.flags          = g_renderPassSettings.colorAttachment.flags;
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = g_renderPassSettings.colorAttachment.loadOp;
                attachment.storeOp        = g_renderPassSettings.colorAttachment.storeOp;
                attachment.stencilLoadOp  = g_renderPassSettings.colorAttachment.stencilLoadOp;
                attachment.stencilStoreOp = g_renderPassSettings.colorAttachment.stencilStoreOp;
                attachment.initialLayout  = g_renderPassSettings.colorAttachment.initialLayout;
                attachment.finalLayout    = g_renderPassSettings.colorAttachment.finalLayout;

                renderPassInfo->resource.attachments.push_back (attachment);
            }            
    };
}   // namespace Core
#endif  // VK_ATTACHMENT_H