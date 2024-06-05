#ifndef VK_IMAGE_VIEW_H
#define VK_IMAGE_VIEW_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSwapChain.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKImageView: protected virtual VKSwapChain {
        private:
            /* Vector to store imageViews for images in swap chain
            */
            std::vector <VkImageView> m_imageViews;
            /* Handle to the log object
            */
            static Log::Record* m_VKImageViewLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++; 

        public:
            VKImageView (void) {
                m_VKImageViewLog = LOG_INIT (m_instanceId, 
                                             static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
            }

            ~VKImageView (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* To use any VkImage, including those in the swap chain, in the render pipeline we have to create a 
             * VkImageView object. An image view is quite literally a view into an image. It describes how to access the 
             * image and which part of the image to access
            */
            void createImageViews (void) {
                /* resize the list to fit all of the image views we'll be creating
                */
                m_imageViews.resize (getSwapChainImages().size());
                /* iterate over all of the swap chain images and populate the struct VkImageViewCreateInfo
                */
                for (size_t i = 0; i < getSwapChainImages().size(); i++) {
                    VkImageViewCreateInfo createInfo{};
                    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    createInfo.image = getSwapChainImages()[i];
                    /* The viewType and format fields specify how the image data should be interpreted (ex: 1D/2D/3D 
                     * textures)
                    */
                    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    createInfo.format = getSwapChainImageFormat();
                    /* The components field allows you to swizzle (mix) the color channels around. ex: you can map all of 
                     * the channels to the red channel for a monochrome texture by setting all channels to 
                     * VK_COMPONENT_SWIZZLE_R. For now we will set it to default mapping
                    */
                    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                    /* The subresourceRange field describes what the image's purpose is and which part of the image should 
                     * be accessed. Here, our images will be used as color targets without any mipmapping levels or 
                     * multiple layers
                    */
                    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    createInfo.subresourceRange.baseMipLevel = 0;
                    createInfo.subresourceRange.levelCount = 1;
                    createInfo.subresourceRange.baseArrayLayer = 0;
                    createInfo.subresourceRange.layerCount = 1;

                    VkResult result = vkCreateImageView (getLogicalDevice(), 
                                                         &createInfo, 
                                                         nullptr,
                                                         &m_imageViews[i]);
                    if (result != VK_SUCCESS) {
                        LOG_ERROR (m_VKImageViewLog) << "Failed to create image views" 
                                                     << " " 
                                                     << result 
                                                     << std::endl;
                        throw std::runtime_error ("Failed to create image views");
                    }
                }
            }

            std::vector <VkImageView> getImageViews (void) {
                return m_imageViews;
            }

            void cleanUp (void) {
                /* Unlike images, the image views were explicitly created by us, so we need to destroy them
                */
                for (auto imageView : m_imageViews)
                    vkDestroyImageView (getLogicalDevice(), imageView, nullptr);
            }
    };

    Log::Record* VKImageView::m_VKImageViewLog;
}   // namespace Renderer
#endif  // VK_IMAGE_VIEW_H