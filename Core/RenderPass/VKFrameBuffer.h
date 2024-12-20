#ifndef VK_FRAME_BUFFER_H
#define VK_FRAME_BUFFER_H

#include "VKRenderPassMgr.h"

namespace Core {
    class VKFrameBuffer: protected virtual VKRenderPassMgr {
        private:
            Log::Record* m_VKFrameBufferLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKFrameBuffer (void) {
                m_VKFrameBufferLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKFrameBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Render passes operate in conjunction with frame buffers. Frame buffers represent a collection of specific
             * memory attachments that a render pass instance uses. In other words, a frame buffer binds a VkImageView
             * with an attachment, and the frame buffer together with the render pass defines the render target
            */
            void createFrameBuffer (uint32_t deviceInfoId,
                                    uint32_t renderPassInfoId,
                                    const std::vector <VkImageView>& attachments) {

                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkFramebufferCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* Specify with which render pass the frame buffer needs to be compatible. You can only use a frame buffer
                 * with the render passes that it is compatible with, which roughly means that they use the same number
                 * and type of attachments
                */
                createInfo.renderPass = renderPassInfo->resource.renderPass;
                /* The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to
                 * the respective attachment descriptions in the render pass pAttachments array
                */
                createInfo.attachmentCount = static_cast <uint32_t> (attachments.size());
                createInfo.pAttachments    = attachments.data();
                createInfo.width           = deviceInfo->params.swapChainExtent.width;
                createInfo.height          = deviceInfo->params.swapChainExtent.height;
                createInfo.layers          = 1;

                VkFramebuffer frameBuffer;
                VkResult result = vkCreateFramebuffer (deviceInfo->resource.logDevice,
                                                       &createInfo,
                                                       VK_NULL_HANDLE,
                                                       &frameBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKFrameBufferLog) << "Failed to create frame buffer "
                                                   << "[" << renderPassInfoId << "]"
                                                   << " "
                                                   << "[" << deviceInfoId << "]"
                                                   << " "
                                                   << "[" << string_VkResult (result) << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Failed to create frame buffer");
                }

                renderPassInfo->resource.frameBuffers.push_back (frameBuffer);
            }

            void cleanUp (uint32_t deviceInfoId, uint32_t renderPassInfoId) {
                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                /* Destroy the frame buffers before the image views and render pass that they are based on
                */
                for (auto const& frameBuffer: renderPassInfo->resource.frameBuffers)
                    vkDestroyFramebuffer (deviceInfo->resource.logDevice, frameBuffer, VK_NULL_HANDLE);
                renderPassInfo->resource.frameBuffers.clear();
            }
    };
}   // namespace Core
#endif  // VK_FRAME_BUFFER_H