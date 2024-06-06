#ifndef VK_FRAME_BUFFER_H
#define VK_FRAME_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKImageView.h"
#include "VKRenderPass.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKFrameBuffer: protected VKImageView,
                         protected virtual VKRenderPass {
        private:
            /* Handle to framebuffers
            */
            std::vector <VkFramebuffer> m_framebuffers;
            /* Handle to the log object
            */
            static Log::Record* m_VKFrameBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
            
        public:
            VKFrameBuffer (void) {
                m_VKFrameBufferLog = LOG_INIT (m_instanceId, 
                                               static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                               Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                               "./Build/Log/"); 
            }

            ~VKFrameBuffer (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Render passes operate in conjunction with framebuffers. Framebuffers represent a collection of specific 
             * memory attachments that a render pass instance uses. In other words, a frame buffer binds a VkImageView 
             * with an attachment, and the frame buffer together with the render pass defines the render target. 
             * 
             * However, the image that we have to use for the attachment depends on which image the swap chain returns 
             * when we retrieve one for presentation. That means that we have to create a framebuffer for all of the 
             * images in the swap chain and use the one that corresponds to the retrieved image at drawing time
            */
            void createFrameBuffers (void) {
                /* Resize the container to hold all of the framebuffers
                */
                m_framebuffers.resize (getImageViews().size());
                /* Iterate through the image views and create framebuffers from them
                */
                for (size_t i = 0; i < getImageViews().size(); i++) {
                    VkImageView attachments[] = {
                        getImageViews()[i]
                    };

                    VkFramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    /* specify with which renderPass the framebuffer needs to be compatible. You can only use a 
                     * framebuffer with the render passes that it is compatible with, which roughly means that they use 
                     * the same number and type of attachments
                    */
                    framebufferInfo.renderPass = getRenderPass();
                    /* The attachmentCount and pAttachments parameters specify the VkImageView objects that should be 
                     * bound to the respective attachment descriptions in the render pass pAttachment array
                    */
                    framebufferInfo.attachmentCount = 1;
                    framebufferInfo.pAttachments = attachments;
                    framebufferInfo.width = getSwapChainExtent().width;
                    framebufferInfo.height = getSwapChainExtent().height;
                    /* layers refers to the number of layers in image arrays. Our swap chain images are single images, so 
                     * the number of layers is 1
                    */
                    framebufferInfo.layers = 1;

                    VkResult result = vkCreateFramebuffer (getLogicalDevice(), 
                                                           &framebufferInfo, 
                                                           nullptr, 
                                                           &m_framebuffers[i]);
                    if (result != VK_SUCCESS) {
                        LOG_ERROR (m_VKFrameBufferLog) << "Failed to create framebuffers " 
                                                       << "[" << string_VkResult (result) << "]"
                                                       << std::endl;
                        throw std::runtime_error ("Failed to create framebuffers");
                    }
                }
            }

            std::vector <VkFramebuffer> getFrameBuffers (void) {
                return m_framebuffers;
            }

            void cleanUp (void) {
                /* Destroy the framebuffers before the image views and render pass that they are based on
                */
                for (auto framebuffer : m_framebuffers)
                    vkDestroyFramebuffer (getLogicalDevice(), framebuffer, nullptr);                
            }
    };

    Log::Record* VKFrameBuffer::m_VKFrameBufferLog;
}   // namespace Renderer
#endif  // VK_FRAME_BUFFER_H