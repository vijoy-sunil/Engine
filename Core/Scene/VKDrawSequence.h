#ifndef VK_DRAW_SEQUENCE_H
#define VK_DRAW_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Model/VKModelMatrix.h"
#include "../Buffer/VKUniformBuffer.h"
#include "../Cmd/VKCmdBuffer.h"
#include "../Cmd/VKCmd.h"
#include "VKCameraMgr.h"
#include "VKSyncObject.h"
#include "VKResizing.h"

using namespace Collections;

namespace Renderer {
    class VKDrawSequence: protected virtual VKWindow,
                          protected virtual VKModelMatrix,
                          protected virtual VKUniformBuffer,
                          protected virtual VKCmdBuffer,
                          protected virtual VKCmd,
                          protected virtual VKCameraMgr,
                          protected virtual VKSyncObject,
                          protected VKResizing {
        private:
            /* To use the right objects (command buffers, sync objects etc.) every frame, keep track of the current frame 
             * in flight
            */
            uint32_t m_currentFrameInFlight;

            static Log::Record* m_VKDrawSequenceLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKDrawSequence (void) {
                m_currentFrameInFlight = 0;

                m_VKDrawSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKDrawSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void runSequence (uint32_t modelInfoId, 
                              uint32_t renderPassInfoId,
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId,
                              uint32_t resourceId, 
                              uint32_t sceneInfoId,
                              bool refreshModelTransform, bool refreshCameraTransform) {

                auto modelInfo  = getModelInfo  (modelInfoId);
                auto cameraInfo = getCameraInfo (cameraInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);
                auto deviceInfo = getDeviceInfo();
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - WAIT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* At the start of the frame, we want to wait until the previous frame has finished, so that the command 
                 * buffer and semaphores are available to use. The vkWaitForFences function takes an array of fences and 
                 * waits on the host for either any or all of the fences to be signaled before returning. The VK_TRUE we 
                 * pass here indicates that we want to wait for all fences, but in the case of a single one it doesn't 
                 * matter. This function also has a timeout parameter that we set to the maximum value of a 64 bit 
                 * unsigned integer, UINT64_MAX, which effectively disables the timeout
                 * 
                 * We use a fence for waiting on the previous frame to finish, this is so that we don't draw more than 
                 * one frame at a time. Because we re-record the command buffer every frame, we cannot record the next 
                 * frame's work to the command buffer until the current frame has finished executing, as we don't want to 
                 * overwrite the current contents of the command buffer while the GPU is using it
                */
                uint32_t inFlightFenceInfoId = sceneInfo->id.inFlightFenceInfoBase + m_currentFrameInFlight;
                vkWaitForFences (deviceInfo->shared.logDevice, 
                                 1, 
                                 &getFenceInfo (inFlightFenceInfoId, FEN_IN_FLIGHT)->resource.fence, 
                                 VK_TRUE, 
                                 UINT64_MAX);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - ACQUIRE SWAP CHAIN IMAGE                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* The first two parameters of vkAcquireNextImageKHR are the logical device and the swap chain from which 
                 * we wish to acquire an image. The third parameter specifies a timeout in nanoseconds for an image to 
                 * become available. Using the maximum value of a 64 bit unsigned integer means we effectively disable 
                 * the timeout
                 * 
                 * The next two parameters specify synchronization objects that are to be signaled when the presentation 
                 * engine is finished using the image. That's the point in time where we can start drawing to it
                 * 
                 * The index refers to the VkImage in our swap chain images array. We're going to use that index to pick 
                 * the framebuffer. It just returns the index of the next image that will be available at some point 
                 * notified by the semaphore
                */
                uint32_t swapChainImageId;
                uint32_t imageAvailableSemaphoreInfoId = sceneInfo->id.imageAvailableSemaphoreInfoBase +
                                                         m_currentFrameInFlight;
                VkResult result = vkAcquireNextImageKHR (deviceInfo->shared.logDevice, 
                                                         deviceInfo->unique[resourceId].swapChain.swapChain, 
                                                         UINT64_MAX, 
                                                         getSemaphoreInfo (imageAvailableSemaphoreInfoId, 
                                                                           SEM_IMAGE_AVAILABLE)->resource.semaphore, 
                                                         VK_NULL_HANDLE, 
                                                         &swapChainImageId);
                /* If the swap chain turns out to be out of date when attempting to acquire an image, then it is no longer 
                 * possible to present to it. Therefore we should immediately recreate the swap chain and its dependents
                 * and try again
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    LOG_WARNING (m_VKDrawSequenceLog) << "Failed to acquire swap chain image "
                                                      << "[" << resourceId << "]"
                                                      << " " 
                                                      << "[" << string_VkResult (result) << "]"
                                                      << std::endl; 
                    recreateSwapChainDeps (sceneInfoId, 
                                           renderPassInfoId,
                                           resourceId);
                    return;
                }
                /* You could also decide to recreate and return if the swap chain is suboptimal, but we've chosen to 
                 * proceed anyway in that case because we've already acquired an image. Both VK_SUCCESS and 
                 * VK_SUBOPTIMAL_KHR are considered "success" return codes here
                */
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    LOG_ERROR (m_VKDrawSequenceLog) << "Failed to acquire swap chain image "
                                                    << "[" << resourceId << "]"
                                                    << " "
                                                    << "[" << string_VkResult (result) << "]"
                                                    << std::endl; 
                    throw std::runtime_error ("Failed to acquire swap chain image");
                }
                /* After waiting for fence, we need to manually reset the fence to the unsignaled state immediately after.
                 * But we delay it to upto this point to avoid deadlock on the in flight fence
                 *
                 * When vkAcquireNextImageKHR returns VK_ERROR_OUT_OF_DATE_KHR, we recreate the swap chain and its 
                 * dependents and then return. But before that happens, the current frame's fence was waited upon and 
                 * reset. Since we return immediately, no work is submitted for execution and the fence will never be 
                 * signaled, causing vkWaitForFences to halt forever
                 * 
                 * To overcome this, delay resetting the fence until after we know for sure we will be submitting work 
                 * with it. Thus, if we return early, the fence is still signaled and vkWaitForFences wont deadlock the 
                 * next time we use the same fence object
                */
                vkResetFences (deviceInfo->shared.logDevice, 
                               1, 
                               &getFenceInfo (inFlightFenceInfoId, FEN_IN_FLIGHT)->resource.fence);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - MODEL TRANSFORM                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                if (refreshModelTransform == true)
                    createModelMatrix (modelInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - CAMERA TRANSFORM                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                if (refreshCameraTransform == true)
                    createViewMatrix (cameraInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - UPDATE UNIFORMS                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                PerModelDataUBO perModelData;
                perModelData.model = modelInfo->transform.model;

                updateUniformBuffer (sceneInfo->id.uniformBufferInfoBase + m_currentFrameInFlight,
                                     sizeof (PerModelDataUBO),
                                     &perModelData);

                SceneDataVertPC sceneDataVert;
                sceneDataVert.texId      = sceneInfo->meta.texId;
                sceneDataVert.view       = cameraInfo->transform.view;
                sceneDataVert.projection = cameraInfo->transform.projection;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - RECORD AND SUBMIT                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* First, we call vkResetCommandBuffer on the command buffer to make sure it is able to be recorded
                */
                vkResetCommandBuffer (sceneInfo->resource.commandBuffers[m_currentFrameInFlight], 0);
                beginRecording       (sceneInfo->resource.commandBuffers[m_currentFrameInFlight], 0, VK_NULL_HANDLE);
                /* Define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR. Note that, the order of clear values 
                 * should be identical to the order of your attachments
                 * 
                 * The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane 
                 * and 0.0 at the near view plane. The initial value at each point in the depth buffer should be the 
                 * furthest possible depth, which is 1.0
                */
                auto clearValues = std::vector {
                    /* Attachment 0
                    */
                    VkClearValue {
                        /* Black with 100% opacity
                        */
                        {{0.0f, 0.0f, 0.0f, 1.0f}}
                    },
                    /* Attachment 1
                    */
                    VkClearValue {
                        /* clear value for the depth aspect and stencil aspect of the depth/stencil attachment
                        */
                        {{1.0f, 0}}
                    }
                };
                beginRenderPass      (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      renderPassInfoId,
                                      swapChainImageId,
                                      resourceId,
                                      clearValues);

                bindPipeline         (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      pipelineInfoId,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS);

                updatePushConstants  (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      pipelineInfoId,
                                      VK_SHADER_STAGE_VERTEX_BIT,
                                      0, sizeof (SceneDataVertPC), &sceneDataVert);

                auto secondaryViewPorts = std::vector <VkViewport> {};
                setViewPorts         (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      resourceId,
                                      0,
                                      secondaryViewPorts);

                auto secondaryScissors = std::vector <VkRect2D> {};
                setScissors          (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      resourceId,
                                      0,
                                      secondaryScissors);

                auto vertexBufferInfoIdsToBind = std::vector {
                    modelInfo->id.vertexBufferInfo
                };
                auto vertexBufferOffsets = std::vector <VkDeviceSize> {
                    0
                };
                bindVertexBuffers    (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      0,
                                      vertexBufferInfoIdsToBind,
                                      vertexBufferOffsets);

                bindIndexBuffer      (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      modelInfo->id.indexBufferInfo,
                                      0,
                                      VK_INDEX_TYPE_UINT32);

                auto descriptorSetsToBind = std::vector {
                    sceneInfo->resource.descriptorSets[m_currentFrameInFlight]
                };        
                bindDescriptorSets   (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      pipelineInfoId,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      0,
                                      descriptorSetsToBind);

                drawIndexed          (sceneInfo->resource.commandBuffers[m_currentFrameInFlight],
                                      modelInfo->meta.indicesCount,
                                      1, 0, 0, 0);

                endRenderPass        (sceneInfo->resource.commandBuffers[m_currentFrameInFlight]);
                endRecording         (sceneInfo->resource.commandBuffers[m_currentFrameInFlight]);  

                VkSubmitInfo drawOpsSubmitInfo;
                drawOpsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                drawOpsSubmitInfo.pNext = VK_NULL_HANDLE;
                /* The first three parameters specify which semaphores to wait on before execution begins and in which 
                 * stage(s) of the pipeline to wait. We want to wait with writing colors to the image until it's 
                 * available, so we're specifying the stage of the graphics pipeline that writes to the color attachment. 
                 * That means that theoretically the implementation can already start executing our vertex shader and 
                 * such while the image is not yet available
                 * 
                 * Each entry in the wait stages array corresponds to the semaphore with the same index in the wait
                 * semaphores array
                */
                auto waitSemaphores = std::vector {
                    getSemaphoreInfo (imageAvailableSemaphoreInfoId, SEM_IMAGE_AVAILABLE)->resource.semaphore 
                };
                auto waitStages     = std::vector <VkPipelineStageFlags> {
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                };
                drawOpsSubmitInfo.waitSemaphoreCount = static_cast <uint32_t> (waitSemaphores.size());
                drawOpsSubmitInfo.pWaitSemaphores    = waitSemaphores.data();
                drawOpsSubmitInfo.pWaitDstStageMask  = waitStages.data();
                drawOpsSubmitInfo.commandBufferCount = 1;
                drawOpsSubmitInfo.pCommandBuffers    = &sceneInfo->resource.commandBuffers[m_currentFrameInFlight];
                /* The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the 
                 * command buffer(s) have finished execution
                */
                uint32_t renderDoneSemaphoreInfoId = sceneInfo->id.renderDoneSemaphoreInfoBase + m_currentFrameInFlight;
                auto signalSemaphores = std::vector {
                    getSemaphoreInfo (renderDoneSemaphoreInfoId, SEM_RENDER_DONE)->resource.semaphore 
                };
                drawOpsSubmitInfo.signalSemaphoreCount = static_cast <uint32_t> (signalSemaphores.size());
                drawOpsSubmitInfo.pSignalSemaphores    = signalSemaphores.data();
                /* The last parameter references an optional fence that will be signaled when the command buffers finish 
                 * execution. This allows us to know when it is safe for the command buffer to be reused, thus we want 
                 * to give it the in flight fence. Now on the next frame, the CPU will wait for this command buffer to 
                 * finish executing before it records new commands into it
                */
                result = vkQueueSubmit (deviceInfo->unique[resourceId].graphicsQueue, 
                                        1,
                                        &drawOpsSubmitInfo,
                                        getFenceInfo (inFlightFenceInfoId, FEN_IN_FLIGHT)->resource.fence);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDrawSequenceLog) << "Failed to submit draw ops command buffer "
                                                    << "[" << resourceId << "]"
                                                    << " " 
                                                    << "[" << string_VkResult (result) << "]"
                                                    << std::endl; 
                    throw std::runtime_error ("Failed to submit draw ops command buffer");                    
                } 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - PRESENT                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* After queueing all rendering commands and transitioning the image to the correct layout, it is time to 
                 * queue an image for presentation
                */
                VkPresentInfoKHR presentInfo;
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.pNext = VK_NULL_HANDLE;
                /* The first two parameters specify which semaphores to wait on before presentation can happen, just like 
                 * VkSubmitInfo. Since we want to wait on the command buffer to finish execution, we take the semaphores 
                 * which will be signalled and wait on them
                */
                presentInfo.waitSemaphoreCount = static_cast <uint32_t> (signalSemaphores.size());;
                presentInfo.pWaitSemaphores    = signalSemaphores.data();
                /* The next two parameters specify the swap chains to present images to and the index of the image for 
                 * each swap chain
                */
                auto swapChains = std::vector {
                    deviceInfo->unique[resourceId].swapChain.swapChain
                };
                presentInfo.swapchainCount = static_cast <uint32_t> (swapChains.size());
                presentInfo.pSwapchains    = swapChains.data();
                presentInfo.pImageIndices  = &swapChainImageId;
                /* Applications that do not need per swap chain results can use null for pResults. If non-null, each 
                 * entry in pResults will be set to the VkResult for presenting the swap chain corresponding to the same 
                 * index in pSwapchains
                 * 
                 * It's not necessary if you're only using a single swap chain, because you can simply use the return 
                 * value of the present function
                */
                presentInfo.pResults = VK_NULL_HANDLE;

                /* The vkQueuePresentKHR function returns the same values with the same meaning as vkAcquireNextImageKHR. 
                 * In this case we will also recreate the swap chain and its dependents if it is suboptimal, because we 
                 * want the best possible result
                 * 
                 * Note that, the presentation engine isn't guaranteed to act in concert with the queue it’s on, even if 
                 * it’s on a graphics queue. vkAcquireNextImageKHR returns when the presentation engine knows which index 
                 * will be used next, but provides no guarantee that it’s actually synchronized with the display and 
                 * finished with the resources from the last VkQueuePresentKHR with that index
                 * 
                 * You should use both the semaphore and the fence to ensure that it is safe to reuse resources, by 
                 * waiting on the fence before re-recording any command buffers or updating any buffers or descriptors 
                 * associated with that index, and waiting on the semaphore when submitting any stage that depends on 
                 * the associated swap chain image
                */
                result = vkQueuePresentKHR (deviceInfo->unique[resourceId].presentQueue, &presentInfo);
                /* Why didn't we check framebuffer resized boolean after vkAcquireNextImageKHR?
                 * It is important to note that a signalled semaphore can only be destroyed by vkDeviceWaitIdle if it is 
                 * being waited on by a vkQueueSubmit. Since we are handling the resize explicitly using the boolean, 
                 * returning after vkAcquireNextImageKHR (thus calling vkDeviceWaitIdle) will make the semaphore signalled 
                 * but have nothing waiting on it
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFrameBufferResized()) {
                    LOG_WARNING (m_VKDrawSequenceLog) << "Failed to present swap chain image "
                                                      << "[" << resourceId << "]"
                                                      << " " 
                                                      << "[" << string_VkResult (result) << "]" 
                                                      << std::endl; 
                    setFrameBufferResized (false);
                    recreateSwapChainDeps (sceneInfoId, 
                                           renderPassInfoId,
                                           resourceId);
                }
                else if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDrawSequenceLog) << "Failed to present swap chain image "
                                                    << "[" << resourceId << "]"
                                                    << " " 
                                                    << "[" << string_VkResult (result) << "]" 
                                                    << std::endl;
                    throw std::runtime_error ("Failed to present swap chain image");
                } 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - UPDATE CURRENT FRAME IN FLIGHT COUNT                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                m_currentFrameInFlight = (m_currentFrameInFlight + 1) % g_maxFramesInFlight;        
            }
    };

    Log::Record* VKDrawSequence::m_VKDrawSequenceLog;
}   // namespace Renderer
#endif  // VK_DRAW_SEQUENCE_H