#ifndef VK_SWAP_CHAIN_H
#define VK_SWAP_CHAIN_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKLogDevice.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKSwapChain: protected VKLogDevice {
        private:
            /* Handle to the swap chain
            */
            VkSwapchainKHR m_swapChain;
            /* Handle to images in the swap chain
            */
            std::vector <VkImage> m_swapChainImages;
            /* Hanlde to swap chain 'format' member from 'VkSurfaceFormatKHR' surface format, and extent
            */
            VkFormat m_swapChainImageFormat;
            VkExtent2D m_swapChainExtent;
            /* Handle to the log object
            */
            static Log::Record* m_VKSwapChainLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 1; 
            /* If the swapChainAdequate conditions were met (see checkPhysicalDeviceSupport) then the support is 
             * definitely sufficient, but there may still be many different modes of varying optimality. We'll need to 
             * find the right settings when creating the best possible swap chain. There are three types of settings to 
             * determine:
             * 
             * (1) Surface format (color depth)
             * (2) Presentation mode (conditions for "swapping" images to the screen)
             * (3) Swap extent (resolution of images in swap chain)
             */

            /* (1) Surface format
             * Note that we'll pass the formats member of the SwapChainSupportDetails struct as argument to this function
             *
             * Each VkSurfaceFormatKHR entry contains a format and a colorSpace member.
             *  
             * format: The format member specifies the color channels and types. For example, VK_FORMAT_B8G8R8A8_SRGB 
             * means that we store the B, G, R and alpha channels in that order with an 8 bit unsigned integer for a 
             * total of 32 bits per pixel. 
             * 
             * colorSpace: The colorSpace member indicates if the SRGB color space is supported or not using the 
             * VK_COLOR_SPACE_SRGB_NONLINEAR_KHR flag. 
             * 
             * For the color space we'll use SRGB if it is available, because it results in more accurate perceived 
             * colors. It is also pretty much the standard color space for images, like the textures we'll use later on. 
             * Because of that we should also use an SRGB color format, of which one of the most common ones is 
             * VK_FORMAT_B8G8R8A8_SRGB
            */
            VkSurfaceFormatKHR pickSwapSurfaceFormat (const std::vector <VkSurfaceFormatKHR>& availableFormats) {
                /* Choose the format and colorSpace from available formats (we have already populated this list)
                */
                for (const auto& availableFormat: availableFormats) {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }
                /* If not it's okay to just settle with the first format that is specified
                */
                return availableFormats [0];
            }

            /* (2) Presenation mode
             * This represents the actual conditions for showing images to the screen
             * 
             * There are four possible modes available in Vulkan:
             * 
             * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right 
             * away, which may result in tearing.
             * 
             * VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the 
             * queue when the display is refreshed and the program inserts rendered images at the back of the queue. If 
             * the queue is full then the program has to wait. This is most similar to vertical sync as found in modern 
             * games. The moment that the display is refreshed is known as "vertical blank".
             * 
             * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late 
             * and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the 
             * image is transferred right away when it finally arrives. This may result in visible tearing.
             * 
             * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the 
             * application when the queue is full, the images that are already queued are simply replaced with the newer 
             * ones. This mode can be used to render frames as fast as possible while still avoiding tearing, resulting 
             * in fewer latency issues than standard vertical sync. This is commonly known as "triple buffering".
            */
            VkPresentModeKHR pickSwapPresentMode (const std::vector <VkPresentModeKHR>& availablePresentModes) {
                /* VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern. It allows us to 
                 * avoid tearing while still maintaining a fairly low latency by rendering new images that are as 
                 * up-to-date as possible right until the vertical blank. On mobile devices, where energy usage is more 
                 * important, you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead
                 */
                for (const auto& availablePresentMode: availablePresentModes) {
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
             * pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds.
             * 
             * GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution 
             * {WIDTH, HEIGHT} that we specified earlier when creating the window is measured in screen coordinates. But 
             * Vulkan works with pixels, so the swap chain extent must be specified in pixels as well. Unfortunately, if 
             * you are using a high DPI display (like Apple's Retina display), screen coordinates don't correspond to 
             * pixels. Instead, due to the higher pixel density, the resolution of the window in pixel will be larger 
             * than the resolution in screen coordinates. So if Vulkan doesn't fix the swap extent for us, we can't just 
             * use the original {WIDTH, HEIGHT}. Instead, we must use glfwGetFramebufferSize to query the resolution of 
             * the window in pixel before matching it against the minimum and maximum image extent.
            */
            VkExtent2D pickSwapExtent (const VkSurfaceCapabilitiesKHR& capabilities) {
                if (capabilities.currentExtent.width != std::numeric_limits <uint32_t> ::max())
                    return capabilities.currentExtent;
                    
                else {
                    int width, height;
                    glfwGetFramebufferSize (getWindow(), &width, &height);

                    VkExtent2D actualExtent = {
                        static_cast <uint32_t> (width),
                        static_cast <uint32_t> (height)
                    };

                    actualExtent.width = std::clamp (actualExtent.width, 
                                                     capabilities.minImageExtent.width, 
                                                     capabilities.maxImageExtent.width);
                    actualExtent.height = std::clamp (actualExtent.height, 
                                                     capabilities.minImageExtent.height, 
                                                     capabilities.maxImageExtent.height);

                    return actualExtent;
                }
            }

        public:
            VKSwapChain (void) {
                m_VKSwapChainLog = LOG_INIT (m_instanceId, 
                                             Log::VERBOSE, 
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
                LOG_INFO (m_VKSwapChainLog) << "Constructor called" << std::endl; 
            }

            ~VKSwapChain (void) {
                LOG_INFO (m_VKSwapChainLog) << "Destructor called" << std::endl;
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Just checking if a swap chain is available is not sufficient, because it may not actually be compatible 
             * with our window surface. Creating a swap chain also involves a lot more settings than instance and device 
             * creation, so we need to query for some more details before we're able to proceed. There are basically 
             * three kinds of properties we need to check: 
             * 
             * (1) Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
             * (2) Surface formats (pixel format, color space)
             * (3) Available presentation modes
             * 
             * This struct will be populated later
            */
            struct SwapChainSupportDetails {
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector <VkSurfaceFormatKHR> formats;
                std::vector <VkPresentModeKHR> presentModes;
            };

            SwapChainSupportDetails checkSwapChainSupport (VkPhysicalDevice physicalDevice) {
                SwapChainSupportDetails details;
                /* (1)
                */
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physicalDevice, 
                                                           getSurface(), 
                                                           &details.capabilities);
                /* (2)
                */
                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, getSurface(), &formatCount, nullptr);
                if (formatCount != 0) {
                    details.formats.resize (formatCount);
                    vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, 
                                                          getSurface(), 
                                                          &formatCount, 
                                                          details.formats.data());
                }
                /* (3)
                */
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, getSurface(), &presentModeCount, nullptr);
                if (presentModeCount != 0) {
                    details.presentModes.resize (presentModeCount);
                    vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, 
                                                               getSurface(), 
                                                               &presentModeCount, 
                                                               details.presentModes.data());
                }
                return details;
            }

            /* Vulkan does not have the concept of a "default framebuffer", hence it requires an infrastructure that will 
             * own the buffers we will render to before we visualize them on the screen. This infrastructure is known as 
             * the swap chain and must be created explicitly in Vulkan. The swap chain is essentially a queue of images 
             * that are waiting to be presented to the screen. 
             * 
             * Our application will acquire such an image to draw to it, and then return it to the queue. How exactly the 
             * queue works and the conditions for presenting an image from the queue depend on how the swap chain is set 
             * up, but the general purpose of the swap chain is to synchronize the presentation of images with the 
             * refresh rate of the screen
            */
            void createSwapChain (void) {
                SwapChainSupportDetails swapChainSupport = checkSwapChainSupport (getPhysicalDevice());

                VkSurfaceFormatKHR surfaceFormat = pickSwapSurfaceFormat (swapChainSupport.formats);
                VkPresentModeKHR presentMode = pickSwapPresentMode (swapChainSupport.presentModes);
                VkExtent2D extent = pickSwapExtent (swapChainSupport.capabilities); 

                /* Aside from the above properties we also have to decide how many images we would like to have in the 
                 * swap chain. The implementation specifies the minimum number that it requires to function.
                 *
                 * However, simply sticking to this minimum means that we (the application) may sometimes have to wait on 
                 * the driver to complete internal operations before we can acquire another image to render to. Therefore 
                 * it is recommended to request at least one more image than the minimum
                 * 
                 * Remember that we only specified a minimum number of images in the swap chain, so the implementation is 
                 * allowed to create a swap chain with more
                */
                uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
                /* Make sure the imageCount is within bounds
                 * if the queried maxImageCount was '0', this means that there is no maximum
                */
                if (swapChainSupport.capabilities.maxImageCount > 0 && 
                    imageCount > swapChainSupport.capabilities.maxImageCount) 
                    imageCount = swapChainSupport.capabilities.maxImageCount;

                /* We are now ready to create the swap chain
                */
                VkSwapchainCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                /* Specify which surface the swap chain should be tied to
                */
                createInfo.surface = getSurface();
                createInfo.imageFormat = surfaceFormat.format;
                createInfo.imageColorSpace = surfaceFormat.colorSpace;
                createInfo.presentMode = presentMode;
                createInfo.imageExtent = extent;
                createInfo.minImageCount = imageCount;
                /* imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are 
                 * developing a stereoscopic 3D application
                */
                createInfo.imageArrayLayers = 1;
                /* The imageUsage bit field specifies what kind of operations we'll use the images in the swap chain for.
                 * Here, we're going to render directly to them, which means that they're used as color attachment. It is 
                 * also possible that you'll render images to a separate image first to perform operations like 
                 * post-processing. In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use 
                 * a memory operation to transfer the rendered image to a swap chain image
                */
                createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                /* Next, we need to specify how to handle swap chain images that will be used across multiple queue 
                 * families. That will be the case in our application if the graphics queue family is different from the 
                 * presentation queue. We'll be drawing on the images in the swap chain from the graphics queue and then 
                 * submitting them on the presentation queue
                */
                QueueFamilyIndices indices = checkQueueFamilySupport (getPhysicalDevice());
                uint32_t queueFamilyIndices[] = { 
                    indices.graphicsFamily.value(), 
                    indices.presentFamily.value()
                };

                /* If the queue families differ, then we'll be using the concurrent mode (Images can be used across 
                 * multiple queue families without explicit ownership transfers.) Concurrent mode requires you to specify 
                 * in advance between which queue families ownership will be shared using the queueFamilyIndexCount and 
                 * pQueueFamilyIndices parameters
                */
                if (indices.graphicsFamily != indices.presentFamily) {
                    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = 2;
                    createInfo.pQueueFamilyIndices = queueFamilyIndices;
                }
                /* If the graphics queue family and presentation queue family are the same, which will be the case on most 
                 * hardware, then we should stick to exclusive mode (An image is owned by one queue family at a time and 
                 * ownership must be explicitly transferred before using it in another queue family. This option offers 
                 * the best performance.)
                */
                else {
                    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;
                    createInfo.pQueueFamilyIndices = nullptr;
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
                 * recreated from scratch and a reference to the old one must be specified in this field.
                 * 
                 * We will handle window resizing and swap chain recreation later
                */
                createInfo.oldSwapchain = VK_NULL_HANDLE;

                VkResult result = vkCreateSwapchainKHR (getLogicalDevice(), &createInfo, nullptr, &m_swapChain);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSwapChainLog) << "Failed to create swap chain" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create swap chain");
                }

                /* Retrieve image handles from swap chain. Again, remember that we only specified a minimum number of 
                 * images in the swap chain, so the implementation is allowed to create a swap chain with more. That's 
                 * why we'll first query the final number of images with vkGetSwapchainImagesKHR
                */
                vkGetSwapchainImagesKHR (getLogicalDevice(), m_swapChain, &imageCount, nullptr);
                m_swapChainImages.resize (imageCount);
                vkGetSwapchainImagesKHR (getLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

                /* Save format and extent
                */
                m_swapChainImageFormat = surfaceFormat.format;
                m_swapChainExtent = extent;
            }

            VkSwapchainKHR getSwapChain (void) {
                return m_swapChain;
            }

            std::vector <VkImage> getSwapChainImages (void) {
                return m_swapChainImages;
            }

            VkFormat getSwapChainImageFormat (void) {
                return m_swapChainImageFormat;
            }

            VkExtent2D getSwapChainExtent (void) {
                return m_swapChainExtent;
            }

            void cleanUp (void) {
                /* Destroy swap chain
                */
                vkDestroySwapchainKHR (getLogicalDevice(), m_swapChain, nullptr);
            }
    };

    Log::Record* VKSwapChain::m_VKSwapChainLog;
}   // namespace Renderer
#endif  // VK_SWAP_CHAIN_H