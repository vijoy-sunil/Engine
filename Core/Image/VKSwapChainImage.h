#ifndef VK_SWAP_CHAIN_IMAGE_H
#define VK_SWAP_CHAIN_IMAGE_H

#include "VKImageMgr.h"

using namespace Collections;

namespace Core {
    class VKSwapChainImage: protected virtual VKImageMgr {
        private:
            Log::Record* m_VKSwapChainImageLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++; 

            /* If the swap chain adequate conditions were met (see physical device support function) then the support is 
             * definitely sufficient, but there may still be many different modes of varying optimality. We'll need to 
             * find the right settings when creating the best possible swap chain. There are three types of settings to 
             * determine:
             * 
             * (1) Surface format (color depth)
             * (2) Presentation mode (conditions for "swapping" images to the screen)
             * (3) Swap extent (resolution of images in swap chain)
             */

            /* (1) Surface format
             * Each VkSurfaceFormatKHR entry contains a format and a colorSpace member
             *  
             * format: The format member specifies the color channels and types. For example, VK_FORMAT_B8G8R8A8_SRGB 
             * means that we store the B, G, R and alpha channels in that order with an 8 bit unsigned integer for a 
             * total of 32 bits per pixel. 
             * 
             * colorSpace: The colorSpace member indicates possible values of supported color spaces
             * 
             * For the color space we'll use SRGB if it is available, because it results in more accurate perceived 
             * colors. It is also pretty much the standard color space for images, one of the most common ones is 
             * VK_FORMAT_B8G8R8A8_SRGB
            */
            VkSurfaceFormatKHR getSwapSurfaceFormat (const std::vector <VkSurfaceFormatKHR>& availableFormats) {
                /* Choose the format and colorSpace from available formats (we have already populated this list)
                */
                for (auto const& availableFormat: availableFormats) {
                    if (availableFormat.format     == VK_FORMAT_B8G8R8A8_SRGB && 
                        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }
                /* If not it's okay to just settle with the first format that is specified
                */
                return availableFormats[0];
            }

            /* (2) Presenation mode
             * This represents the actual conditions for showing images to the screen
             * 
             * There are four possible modes available in Vulkan:
             * 
             * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right 
             * away, which may result in tearing
             * 
             * VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the 
             * queue when the display is refreshed and the program inserts rendered images at the back of the queue. If 
             * the queue is full then the program has to wait. This is most similar to vertical sync as found in modern 
             * games. The moment that the display is refreshed is known as "vertical blank"
             * 
             * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late 
             * and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the 
             * image is transferred right away when it finally arrives. This may result in visible tearing
             * 
             * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the 
             * application when the queue is full, the images that are already queued are simply replaced with the newer 
             * ones. This mode can be used to render frames as fast as possible while still avoiding tearing, resulting 
             * in fewer latency issues than standard vertical sync. This is commonly known as "triple buffering"
            */
            VkPresentModeKHR getSwapPresentMode (const std::vector <VkPresentModeKHR>& availablePresentModes) {
                /* VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern. It allows us to 
                 * avoid tearing while still maintaining a fairly low latency by rendering new images that are as 
                 * up-to-date as possible right until the vertical blank. On mobile devices, where energy usage is more 
                 * important, you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead
                 */
                for (auto const& availablePresentMode: availablePresentModes) {
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                        return availablePresentMode;
                }
                /* Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we return this otherwise
                */
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            /* (3) Swap extent
             * The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the 
             * resolution of the window that we're drawing to in pixels. The range of the possible resolutions is defined 
             * in the VkSurfaceCapabilitiesKHR structure (which we have already queried)
             * 
             * Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent 
             * member. However, some window managers do allow us to differ here and this is indicated by setting the 
             * width and height in currentExtent to a special value: the maximum value of uint32_t. In that case we'll 
             * pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds
             * 
             * GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution 
             * {width, height} that we specified earlier when creating the window is measured in screen coordinates. But 
             * Vulkan works with pixels, so the swap chain extent must be specified in pixels as well. Unfortunately, if 
             * you are using a high DPI display (like Apple's Retina display), screen coordinates don't correspond to 
             * pixels. Instead, due to the higher pixel density, the resolution of the window in pixel will be larger 
             * than the resolution in screen coordinates. So if Vulkan doesn't fix the swap extent for us, we can't just 
             * use the original {width, height}. Instead, we must use glfwGetFramebufferSize to query the resolution of 
             * the window in pixel before matching it against the minimum and maximum image extent
            */
            VkExtent2D getSwapExtent (uint32_t deviceInfoId, const VkSurfaceCapabilitiesKHR* capabilities) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                if (capabilities->currentExtent.width != std::numeric_limits <uint32_t> ::max())
                    return capabilities->currentExtent;
                    
                else {
                    int width, height;
                    glfwGetFramebufferSize (deviceInfo->resource.window, &width, &height);

                    VkExtent2D actualExtent = {
                        static_cast <uint32_t> (width),
                        static_cast <uint32_t> (height)
                    };

                    actualExtent.width = std::clamp (actualExtent.width, 
                                                     capabilities->minImageExtent.width, 
                                                     capabilities->maxImageExtent.width);
                    actualExtent.height = std::clamp (actualExtent.height, 
                                                     capabilities->minImageExtent.height, 
                                                     capabilities->maxImageExtent.height);

                    return actualExtent;
                }
            }

        public:
            VKSwapChainImage (void) {
                m_VKSwapChainImageLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKSwapChainImage (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Vulkan does not have the concept of a "default framebuffer", hence it requires an infrastructure that will 
             * own the buffers we will render to before we visualize them on the screen. This infrastructure is known as 
             * the swap chain and must be created explicitly in Vulkan. The swap chain is essentially a queue of images 
             * that are waiting to be presented to the screen
             * 
             * Our application will acquire such an image to draw to it, and then return it to the queue. How exactly the 
             * queue works and the conditions for presenting an image from the queue depend on how the swap chain is set 
             * up, but the general purpose of the swap chain is to synchronize the presentation of images with the 
             * refresh rate of the screen
            */
            void createSwapChainResources (uint32_t deviceInfoId, uint32_t imageInfoId) {
                auto deviceInfo       = getDeviceInfo (deviceInfoId);
                auto swapChainSupport = getSwapChainSupportDetails (deviceInfoId, deviceInfo->resource.phyDevice);

                auto surfaceFormat    = getSwapSurfaceFormat (swapChainSupport.formats);
                auto presentMode      = getSwapPresentMode   (swapChainSupport.presentModes);
                auto extent           = getSwapExtent        (deviceInfoId, &swapChainSupport.capabilities); 

                /* Aside from the above properties we also have to decide how many images we would like to have in the 
                 * swap chain. The implementation specifies the minimum number that it requires to function
                 *
                 * However, simply sticking to this minimum means that we (the application) may sometimes have to wait on 
                 * the driver to complete internal operations before we can acquire another image to render to. Therefore 
                 * it is recommended to request at least one more image than the minimum
                 * 
                 * Remember that we only specified a minimum number of images in the swap chain, so the implementation is 
                 * allowed to create a swap chain with more
                */
                uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
                /* Make sure the image count is within bounds, if the queried maxImageCount was '0', this means that there
                 * is no maximum
                */
                if (swapChainSupport.capabilities.maxImageCount > 0 && 
                    imageCount > swapChainSupport.capabilities.maxImageCount) 
                    imageCount = swapChainSupport.capabilities.maxImageCount;

                /* We are now ready to create the swap chain
                */
                VkSwapchainCreateInfoKHR createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* Specify which surface the swap chain should be tied to
                */
                createInfo.surface         = deviceInfo->resource.surface;
                createInfo.imageFormat     = surfaceFormat.format;
                createInfo.imageColorSpace = surfaceFormat.colorSpace;
                createInfo.presentMode     = presentMode;                
                createInfo.imageExtent     = extent;
                createInfo.minImageCount   = imageCount;              
                /* imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are 
                 * developing a stereoscopic 3D application
                */
                createInfo.imageArrayLayers = 1;
                /* The imageUsage bit field specifies what kind of operations we'll use the images in the swap chain for.
                 * If we're going to render directly to them, they're used as color attachment. It is also possible that 
                 * you'll render images to a separate image first to perform operations like post-processing. In that case
                 * you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer
                 * the rendered image to a swap chain image
                */
                createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                /* Next, we need to specify how to handle swap chain images that will be used across multiple queue 
                 * families. For example, if the graphics queue family is different from the presentation queue, we'll 
                 * be drawing on the images in the swap chain from the graphics queue and then submitting them on the 
                 * presentation queue
                */
                auto queueFamilyIndices = std::vector { 
                    deviceInfo->meta.graphicsFamilyIndex.value(),
                    deviceInfo->meta.presentFamilyIndex. value()
                };

                if (isQueueFamiliesUnique (queueFamilyIndices)) {
                    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = static_cast <uint32_t> (queueFamilyIndices.size());
                    createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
                }
                else {
                    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;
                    createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE;
                }

                /* We can specify that a certain transform should be applied to images in the swap chain if it is 
                 * supported, like a 90 degree clockwise rotation or horizontal flip. To specify that you do not want any 
                 * transformation, simply specify the current transformation
                */
                createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
                /* The compositeAlpha field specifies if the alpha channel should be used for blending with other windows 
                 * in the window system. You'll almost always want to simply ignore the alpha channel, hence 
                 * VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                */
                createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                /* If the clipped member is set to VK_TRUE then that means that we don't care about the color of pixels 
                 * that are obscured, for example because another window is in front of them. Unless you really need to 
                 * be able to read these pixels back and get predictable results, you'll get the best performance by 
                 * enabling clipping
                */
                createInfo.clipped = VK_TRUE;
                /* With Vulkan it's possible that your swap chain becomes invalid or unoptimized while your application is 
                 * running, for example because the window was resized. In that case the swap chain actually needs to be 
                 * recreated from scratch and a reference to the old one must be specified in this field
                 * 
                 * Note that, we are handling window resizing and swap chain recreation at a different place
                */
                createInfo.oldSwapchain = VK_NULL_HANDLE;

                VkSwapchainKHR swapChain;
                VkResult result = vkCreateSwapchainKHR (deviceInfo->resource.logDevice, 
                                                        &createInfo, 
                                                        VK_NULL_HANDLE, 
                                                        &swapChain);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSwapChainImageLog) << "Failed to create swap chain "
                                                      << "[" << imageInfoId << "]"
                                                      << " "
                                                      << "[" << deviceInfoId  << "]"
                                                      << " "
                                                      << "[" << string_VkResult (result) << "]" 
                                                      << std::endl;
                    throw std::runtime_error ("Failed to create swap chain");
                }

                uint32_t swapChainSize;
                vkGetSwapchainImagesKHR (deviceInfo->resource.logDevice, 
                                         swapChain, 
                                         &swapChainSize, 
                                         VK_NULL_HANDLE);

                std::vector <VkImage> swapChainImages (swapChainSize);
                vkGetSwapchainImagesKHR (deviceInfo->resource.logDevice, 
                                         swapChain, 
                                         &swapChainSize, 
                                         swapChainImages.data());

                auto imageInfo = getImageInfo (0, VOID_IMAGE);
                imageInfo->meta.width           = extent.width;
                imageInfo->meta.height          = extent.height;
                imageInfo->meta.mipLevels       = 1;
                imageInfo->params.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo->params.format        = createInfo.imageFormat;
                imageInfo->params.usage         = createInfo.imageUsage;
                imageInfo->params.sampleCount   = VK_SAMPLE_COUNT_1_BIT;
                imageInfo->params.sharingMode   = createInfo.imageSharingMode;
                imageInfo->params.aspect        = VK_IMAGE_ASPECT_COLOR_BIT;

                /* Iterate over all of the swap chain images and create image views
                */
                for (uint32_t i = 0; i < swapChainSize; i++) {
                    imageInfo->meta.id = imageInfoId + i;
                    createImageView (deviceInfoId, 
                                     imageInfo,
                                     SWAPCHAIN_IMAGE,
                                     0,
                                     swapChainImages[i]);
                }

                /* Save swap chain info to device info
                */
                deviceInfo->meta.swapChainSize          = swapChainSize;
                deviceInfo->resource.swapChain          = swapChain;
                deviceInfo->params.swapChainFormat      = createInfo.imageFormat;
                deviceInfo->params.swapChainPresentMode = createInfo.presentMode;
                deviceInfo->params.swapChainExtent      = createInfo.imageExtent;
                /* Restore id of void image since we will need it for subsequent calls to create swap chain resources
                */
                imageInfo->meta.id = 0;
            }
    };
}   // namespace Core
#endif  // VK_SWAP_CHAIN_IMAGE_H