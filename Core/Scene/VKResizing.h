#ifndef VK_RESIZING_H
#define VK_RESIZING_H

#include "../Image/VKSwapChainImage.h"
#include "../Image/VKDepthImage.h"
#include "../Image/VKMultiSampleImage.h"
#include "../RenderPass/VKFrameBuffer.h"
#include "VKSceneMgr.h"

namespace Core {
    class VKResizing: protected virtual VKSwapChainImage,
                      protected virtual VKDepthImage,
                      protected virtual VKMultiSampleImage,
                      protected virtual VKFrameBuffer,
                      protected virtual VKSceneMgr {
        private:
            Log::Record* m_VKResizingLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKResizing (void) {
                m_VKResizingLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
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
             * on the swap chain images. And, the frame buffers directly depend on the swap chain images, and thus must
             * be recreated as well, and so is the case with its attachments
             *
             * Note that we don't recreate the render pass here for simplicity. In theory it can be possible for the swap
             * chain image format to change during an applications' lifetime, e.g. when moving a window from an standard
             * range to an high dynamic range monitor. This may require the application to recreate the render pass to
             * make sure the change between dynamic ranges is properly reflected
            */
            void recreateSwapChainDeps (uint32_t deviceInfoId,
                                        uint32_t renderPassInfoId,
                                        uint32_t sceneInfoId) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);
                /* We first call vkDeviceWaitIdle, because we shouldn't touch resources that may still be in use
                */
                vkDeviceWaitIdle (deviceInfo->resource.logDevice);
                /* Make sure that the old versions of these objects are cleaned up before recreating them
                */
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY FRAME BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKFrameBuffer::cleanUp (deviceInfoId, renderPassInfoId);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Frame buffers "
                                           << "[" << renderPassInfoId << "]"
                                           << " "
                                           << "[" << deviceInfoId << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MULTI SAMPLE RESOURCES                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (deviceInfoId, sceneInfo->id.multiSampleImageInfo, MULTI_SAMPLE_IMAGE);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Multi sample resources "
                                           << "[" << sceneInfo->id.multiSampleImageInfo << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEPTH RESOURCES                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (deviceInfoId, sceneInfo->id.depthImageInfo, DEPTH_IMAGE);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Depth resources "
                                           << "[" << sceneInfo->id.depthImageInfo << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN RESOURCES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->params.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    VKImageMgr::cleanUp (deviceInfoId, swapChainImageInfoId, SWAP_CHAIN_IMAGE);
                    LOG_INFO (m_VKResizingLog) << "[DELETE] Swap chain resources "
                                               << "[" << swapChainImageInfoId << "]"
                                               << " "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDeviceMgr::cleanUpSwapChain (deviceInfoId);
                LOG_INFO (m_VKResizingLog) << "[DELETE] Swap chain "
                                           << "[" << deviceInfoId << "]"
                                           << std::endl;
                /* Note that in get swap extent method we already query the new window resolution (using
                 * glfwGetFramebufferSize to get the resolution of the surface in pixels) to make sure that the swap
                 * chain images have the (new) correct size
                */
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SWAP CHAIN RESOURCES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSwapChainResources (deviceInfoId, sceneInfo->id.swapChainImageInfoBase);
                LOG_INFO (m_VKResizingLog) << "[OK] Swap chain resources "
                                           << "[" << sceneInfo->id.swapChainImageInfoBase << "]"
                                           << " "
                                           << "[" << deviceInfoId << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEPTH RESOURCES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthResources (deviceInfoId, sceneInfo->id.depthImageInfo);
                LOG_INFO (m_VKResizingLog) << "[OK] Depth resources "
                                           << "[" << sceneInfo->id.depthImageInfo << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG MULTI SAMPLE RESOURCES                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleResources (deviceInfoId, sceneInfo->id.multiSampleImageInfo);
                LOG_INFO (m_VKResizingLog) << "[OK] Multi sample resources "
                                           << "[" << sceneInfo->id.multiSampleImageInfo << "]"
                                           << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG FRAME BUFFERS                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto multiSampleImageInfo = getImageInfo (sceneInfo->id.multiSampleImageInfo, MULTI_SAMPLE_IMAGE);
                auto depthImageInfo       = getImageInfo (sceneInfo->id.depthImageInfo,       DEPTH_IMAGE);

                for (uint32_t i = 0; i < deviceInfo->params.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, SWAP_CHAIN_IMAGE);

                    auto attachments = std::vector {
                        multiSampleImageInfo->resource.imageView,
                        depthImageInfo->resource.imageView,
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (deviceInfoId, renderPassInfoId, attachments);
                    LOG_INFO (m_VKResizingLog) << "[OK] Frame buffer "
                                               << "[" << renderPassInfoId << "]"
                                               << " "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                }
                /* That's all it takes to recreate the swap chain! However, the disadvantage of this approach is that we
                 * need to stop all rendering before creating the new swap chain. It is possible to create a new swap
                 * chain while drawing commands on an image from the old swap chain are still in-flight. You need to pass
                 * the previous swap chain to the old swap chain field in the VkSwapchainCreateInfoKHR struct and destroy
                 * the old swap chain as soon as you've finished using it
                */

                /* How do we figure out when swap chain recreation is necessary?
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
}   // namespace Core
#endif  // VK_RESIZING_H