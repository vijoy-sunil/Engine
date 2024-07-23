#ifndef VK_ATTACHMENT_H
#define VK_ATTACHMENT_H

#include "VKRenderPassMgr.h"
#include "../Image/VKImageMgr.h"

using namespace Collections;

namespace Renderer {
    class VKAttachment: protected virtual VKRenderPassMgr,
                        protected virtual VKImageMgr {
        private:
            static Log::Record* m_VKAttachmentLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKAttachment (void) {
                m_VKAttachmentLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
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
                VkAttachmentReference attachmentReference{};
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
            */
            void createMultiSampleAttachment (uint32_t renderPassInfoId, uint32_t imageInfoId) {
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto imageInfo      = getImageInfo (imageInfoId, MULTISAMPLE_IMAGE);

                VkAttachmentDescription attachment{};
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                /* For multi-sampled rendering in Vulkan, the multi-sampled image is treated separately from the final 
                 * single-sampled image; this provides separate control over what values need to reach memory, since - 
                 * like the depth buffer - the multi-sampled image may only need to be accessed during the processing of 
                 * a tile. For this reason, if the multi-sampled image is not required after the render pass, it can be 
                 * created with VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT and bound to an allocation created with 
                 * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT. The multi-sampled attachment storeOp can then be set to 
                 * VK_ATTACHMENT_STORE_OP_DONT_CARE in the VkAttachmentDescription, so that (at least on tiled renderers)
                 * the full multi-sampled attachment does not need to be written to memory, which can save a lot of 
                 * bandwidth
                */
                attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                renderPassInfo->resource.attachments.push_back (attachment);
            }

            void createDepthStencilAttachment (uint32_t renderPassInfoId, uint32_t imageInfoId) {
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto imageInfo      = getImageInfo (imageInfoId, DEPTH_IMAGE);

                /* The format should be the same as the depth image itself. We don't care about storing the depth data 
                 * (storeOp), because it will not be used after drawing has finished. This may allow the hardware to 
                 * perform additional optimizations. 
                */
                VkAttachmentDescription attachment{};
                attachment.format          = imageInfo->params.format;
                attachment.samples         = imageInfo->params.sampleCount;
                attachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.storeOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 

                renderPassInfo->resource.attachments.push_back (attachment);                 
            }

            /* Note that, multisampled images cannot be presented directly. We first need to resolve them to a regular
             * image
            */
            void createResolveAttachment (uint32_t renderPassInfoId, uint32_t imageInfoId) {
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto imageInfo      = getImageInfo (imageInfoId, SWAPCHAIN_IMAGE);

                VkAttachmentDescription attachment{};
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;       

                renderPassInfo->resource.attachments.push_back (attachment);         
            }            
    };

    Log::Record* VKAttachment::m_VKAttachmentLog;
}   // namespace Renderer
#endif  // VK_ATTACHMENT_H