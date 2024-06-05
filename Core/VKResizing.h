#ifndef VK_RESIZING_H
#define VK_RESIZING_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKFrameBuffer.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKResizing: protected virtual VKFrameBuffer {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKResizingLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
             
        public:
            VKResizing (void) {
                m_VKResizingLog = LOG_INIT (m_instanceId, 
                                            static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                            "./Build/Log/");
            }

            ~VKResizing (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* It is possible for the window surface to change such that the swap chain is no longer compatible with it. 
             * One of the reasons that could cause this to happen is the size of the window changing. We have to catch 
             * these events and recreate the swap chain and all of the creation functions for the objects that depend on 
             * the swap chain or the window size. The image views need to be recreated because they are based directly 
             * on the swap chain images. And, the framebuffers directly depend on the swap chain images, and thus must be 
             * recreated as well
             * 
             * Note that we don't recreate the renderpass here for simplicity. In theory it can be possible for the swap 
             * chain image format to change during an applications' lifetime, e.g. when moving a window from an standard 
             * range to an high dynamic range monitor. This may require the application to recreate the renderpass to 
             * make sure the change between dynamic ranges is properly reflected
            */
            void recreateSwapChain (void) {
                /* There is another case where a swap chain may become out of date and that is a special kind of window 
                 * resizing: window minimization. This case is special because it will result in a frame buffer size of 0.
                 * We will handle that by pausing until the window is in the foreground again
                */
                int width = 0, height = 0;
                glfwGetFramebufferSize (getWindow(), &width, &height);
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize (getWindow(), &width, &height);
                    /* This function puts the calling thread to sleep until at least one event is available in the event 
                     * queue
                    */
                    glfwWaitEvents();
                }

                /* We first call vkDeviceWaitIdle, because we shouldn't touch resources that may still be in use
                */
                vkDeviceWaitIdle (getLogicalDevice());
                /* Make sure that the old versions of these objects are cleaned up before recreating them
                */
                VKFrameBuffer::cleanUp();
                VKImageView::cleanUp();
                VKSwapChain::cleanUp();
                /* Note that in chooseSwapExtent we already query the new window resolution to make sure that the swap 
                 * chain images have the (new) right size, so there's no need to modify chooseSwapExtent (remember that 
                 * we already had to use glfwGetFramebufferSize get the resolution of the surface in pixels when creating 
                 * the swap chain)
                */
                createSwapChain();
                createImageViews();
                createFrameBuffers();
                /* That's all it takes to recreate the swap chain! However, the disadvantage of this approach is that we 
                 * need to stop all rendering before creating the new swap chain. It is possible to create a new swap 
                 * chain while drawing commands on an image from the old swap chain are still in-flight. You need to pass 
                 * the previous swap chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct and destroy 
                 * the old swap chain as soon as you've finished using it
                */

                /* How do we figure out when swap chain recreation is necessary and call our new recreateSwapChain 
                 * function?
                 * Luckily, Vulkan will usually just tell us that the swap chain is no longer adequate during 
                 * presentation. The vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the following 
                 * special values to indicate this:
                 * VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer 
                 * be used for rendering. Usually happens after a window resize.
                 * VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the 
                 * surface properties are no longer matched exactly
                */
            }
    };

    Log::Record* VKResizing::m_VKResizingLog;
}   // namespace Renderer
#endif  // VK_RESIZING_H