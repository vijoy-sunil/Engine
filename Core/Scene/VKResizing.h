#ifndef VK_RESIZING_H
#define VK_RESIZING_H

#include "../Model/VKModelMgr.h"
#include "../Image/VKSwapChainImage.h"
#include "../Image/VKDepthImage.h"
#include "../Image/VKMultiSampleImage.h"
#include "../RenderPass/VKFrameBuffer.h"

using namespace Collections;

namespace Renderer {
    class VKResizing: protected virtual VKModelMgr,
                      protected virtual VKSwapChainImage,
                      protected virtual VKDepthImage,
                      protected virtual VKMultiSampleImage,
                      protected virtual VKFrameBuffer {
        private:
            static Log::Record* m_VKResizingLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKResizing (void) {
                m_VKResizingLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);                
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
             * recreated as well, and so is the case with its attachments
             * 
             * Note that we don't recreate the renderpass here for simplicity. In theory it can be possible for the swap 
             * chain image format to change during an applications' lifetime, e.g. when moving a window from an standard 
             * range to an high dynamic range monitor. This may require the application to recreate the renderpass to 
             * make sure the change between dynamic ranges is properly reflected
            */
            void recreateSwapChainDeps (uint32_t modelInfoId, 
                                        uint32_t renderPassInfoId,
                                        uint32_t resourceId) {

                auto modelInfo      = getModelInfo (modelInfoId);
                auto deviceInfo     = getDeviceInfo();

                /* There is another case where a swap chain may become out of date and that is a special kind of window 
                 * resizing: window minimization. This case is special because it will result in a frame buffer size of 0.
                 * We will handle that by pausing until the window is in the foreground again
                */
                int width = 0, height = 0;
                glfwGetFramebufferSize (deviceInfo->unique[resourceId].window, &width, &height);
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize (deviceInfo->unique[resourceId].window, &width, &height);
                    /* This function puts the calling thread to sleep until at least one event is available in the event 
                     * queue
                    */
                    glfwWaitEvents();
                }

                /* We first call vkDeviceWaitIdle, because we shouldn't touch resources that may still be in use
                */
                vkDeviceWaitIdle (deviceInfo->shared.logDevice);
                /* Make sure that the old versions of these objects are cleaned up before recreating them
                */
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY FRAME BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */               
                VKFrameBuffer::cleanUp (renderPassInfoId);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Frame buffers " 
                                           << "[" << renderPassInfoId << "]"
                                           << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MULTI SAMPLE RESOURCES                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */                                                 
                VKImageMgr::cleanUp (modelInfo->id.multiSampleImageInfo, MULTISAMPLE_IMAGE);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Multi sample resources " 
                                           << "[" << modelInfo->id.multiSampleImageInfo << "]"
                                           << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEPTH RESOURCES                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */                                                 
                VKImageMgr::cleanUp (modelInfo->id.depthImageInfo, DEPTH_IMAGE);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Depth resources " 
                                           << "[" << modelInfo->id.depthImageInfo << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN RESOURCES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */                                                 
                for (uint32_t i = 0; i < deviceInfo->unique[resourceId].swapChain.size; i++) {
                    uint32_t swapChainImageInfoId = modelInfo->id.swapChainImageInfoBase + i;
                    VKImageMgr::cleanUp (swapChainImageInfoId, SWAPCHAIN_IMAGE);
                    LOG_INFO (m_VKResizingLog) << "[DELETE] Swap chain resources " 
                                               << "[" << swapChainImageInfoId << "]"
                                               << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDeviceMgr::cleanUp (resourceId);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Swap chain "
                                           << "[" << resourceId << "]"
                                           << std::endl;                                                                 
                /* Note that in get swap extent method we already query the new window resolution (using 
                 * glfwGetFramebufferSize to get the resolution of the surface in pixels) to make sure that the swap 
                 * chain images have the (new) correct size
                */
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SWAP CHAIN RESOURCES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSwapChainResources (modelInfo->id.swapChainImageInfoBase, resourceId);
                LOG_INFO (m_VKResizingLog) << "[OK] Swap chain resources " 
                                           << "[" << modelInfo->id.swapChainImageInfoBase << "]"
                                           << " "
                                           << "[" << resourceId << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEPTH RESOURCES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthResources (modelInfo->id.depthImageInfo, resourceId);
                LOG_INFO (m_VKResizingLog) << "[OK] Depth resources " 
                                           << "[" << modelInfo->id.depthImageInfo << "]"
                                           << " "
                                           << "[" << resourceId << "]"
                                           << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG MULTI SAMPLE RESOURCES                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleResources (modelInfo->id.multiSampleImageInfo, resourceId);
                LOG_INFO (m_VKResizingLog) << "[OK] Multi sample resources " 
                                           << "[" << modelInfo->id.multiSampleImageInfo << "]"
                                           << " "
                                           << "[" << resourceId << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG FRAME BUFFERS                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto multiSampleImageInfo = getImageInfo (modelInfo->id.multiSampleImageInfo,   MULTISAMPLE_IMAGE);
                auto depthImageInfo       = getImageInfo (modelInfo->id.depthImageInfo,         DEPTH_IMAGE);

                for (uint32_t i = 0; i < deviceInfo->unique[resourceId].swapChain.size; i++) {
                    uint32_t swapChainImageInfoId = modelInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, SWAPCHAIN_IMAGE);

                    auto attachments = std::vector {
                        multiSampleImageInfo->resource.imageView,
                        depthImageInfo->resource.imageView,
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (renderPassInfoId, resourceId, attachments);                    
                    LOG_INFO (m_VKResizingLog) << "[OK] Frame buffer " 
                                               << "[" << renderPassInfoId << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl; 
                }
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
                 * 
                 * VK_ERROR_OUT_OF_DATE_KHR
                 * The swap chain has become incompatible with the surface and can no longer be used for rendering. 
                 * Usually happens after a window resize
                 * 
                 * VK_SUBOPTIMAL_KHR
                 * The swap chain can still be used to successfully present to the surface, but the surface properties 
                 * are no longer matched exactly
                */
            }
    };

    Log::Record* VKResizing::m_VKResizingLog;
}   // namespace Renderer
#endif  // VK_RESIZING_H