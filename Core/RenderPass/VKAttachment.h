#ifndef VK_ATTACHMENT_H
#define VK_ATTACHMENT_H

#include "../Image/VKImageMgr.h"
#include "VKRenderPassMgr.h"

namespace Core {
    class VKAttachment: protected virtual VKImageMgr,
                        protected virtual VKRenderPassMgr {
        private:
            Log::Record* m_VKAttachmentLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;
            
        public:
            VKAttachment (void) {
                m_VKAttachmentLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~VKAttachment (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* The VkAttachmentReference does not reference the attachment object directly, it references the index in 
             * the attachments array specified in VkRenderPassCreateInfo. This allows different sub passes to reference 
             * the same attachment
             *
             * The attachment reference layout tells vulkan what layout to transition the image to at the beginning of 
             * the sub pass for which this reference is defined. Or more to the point, it is the layout which the image 
             * will be in for the duration of the sub pass. Note that, vulkan will automatically transition the attachment
             * to this layout when the sub pass is started
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
            */
            void createAttachment (uint32_t imageInfoId, 
                                   uint32_t renderPassInfoId,
                                   e_imageType type,
                                   VkAttachmentDescriptionFlags flags,
                                   VkAttachmentLoadOp loadOp,           
                                   VkAttachmentStoreOp storeOp,
                                   VkAttachmentLoadOp stencilLoadOp,    
                                   VkAttachmentStoreOp stencilStoreOp,
                                   VkImageLayout initialLayout,         
                                   VkImageLayout finalLayout) {

                auto imageInfo      = getImageInfo (imageInfoId, type);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkAttachmentDescription attachment;
                attachment.flags          = flags;
                attachment.format         = imageInfo->params.format;
                attachment.samples        = imageInfo->params.sampleCount;
                attachment.loadOp         = loadOp;
                attachment.storeOp        = storeOp;
                attachment.stencilLoadOp  = stencilLoadOp;
                attachment.stencilStoreOp = stencilStoreOp;
                attachment.initialLayout  = initialLayout;
                attachment.finalLayout    = finalLayout;

                renderPassInfo->resource.attachments.push_back (attachment);
            }
    };
}   // namespace Core
#endif  // VK_ATTACHMENT_H