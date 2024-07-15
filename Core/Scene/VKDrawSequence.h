#ifndef VK_DRAW_SEQUENCE_H
#define VK_DRAW_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Buffer/VKUniformBuffer.h"
#include "../Cmds/VKCmdBuffer.h"
#include "../Cmds/VKCmds.h"
#include "../Model/VKModelMatrix.h"
#include "VKUniforms.h"
#include "VKTransform.h"
#include "VKCameraMgr.h"
#include "VKSyncObjects.h"
#include "VKResizing.h"

using namespace Collections;

namespace Renderer {
    class VKDrawSequence: protected virtual VKWindow,
                          protected virtual VKUniformBuffer,
                          protected virtual VKCmdBuffer,
                          protected virtual VKCmds,
                          protected virtual VKModelMatrix,
                          protected virtual VKCameraMgr,
                          protected virtual VKSyncObjects,
                          protected VKResizing {
        private:
            /* Data to be handed off between sequences are packed into this struct and saved to the pool
            */
            struct HandOffInfo {
                struct Id {
                    std::vector <uint32_t> inFlightFenceInfos;
                    std::vector <uint32_t> imageAvailableSemaphoreInfos;
                    std::vector <uint32_t> renderDoneSemaphoreInfos;
                } id;

                struct Resource {
                    VkCommandPool commandPool;
                    std::vector <VkCommandBuffer> commandBuffers;
                    TransformInfo transformInfo;
                } resource;
            };
            std::map <uint32_t, HandOffInfo> m_handOffInfoPool{};

            static Log::Record* m_VKDrawSequenceLog;
            const size_t m_instanceId = g_collectionsId++;
            /* To use the right objects (command buffers and sync objects) every frame, keep track of the current frame
            */
            uint32_t m_currentFrame;

            void deleteHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end()) {
                    m_handOffInfoPool.erase (handOffInfoId);
                    return;
                }

                LOG_ERROR (m_VKDrawSequenceLog) << "Failed to delete hand off info "
                                                << "[" << handOffInfoId << "]"          
                                                << std::endl;
                throw std::runtime_error ("Failed to delete hand off info"); 
            }

        public:
            VKDrawSequence (void) {
                m_currentFrame = 0;

                m_VKDrawSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKDrawSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

            void runSequence (uint32_t modelInfoId, 
                              uint32_t renderPassInfoId,
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId,
                              uint32_t resourceId, 
                              uint32_t handOffInfoId,
                              bool refreshModelTransform, bool refreshCameraTransform) {

                auto modelInfo   = getModelInfo   (modelInfoId);
                auto cameraInfo  = getCameraInfo  (cameraInfoId);
                auto handOffInfo = getHandOffInfo (handOffInfoId);
                auto deviceInfo  = getDeviceInfo();
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
                auto inFlightFenceInfoId = handOffInfo->id.inFlightFenceInfos[m_currentFrame];
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
                 * The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick 
                 * the VkFrameBuffer. It just returns the index of the next image that will be available at some point 
                 * notified by the semaphore
                */
                uint32_t swapChainImageId;
                auto imageAvailableSemaphoreInfoId = handOffInfo->id.imageAvailableSemaphoreInfos[m_currentFrame];
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
                                                      << "[" << string_VkResult (result) << "]"
                                                      << std::endl; 
                    recreateSwapChainDeps (modelInfoId, 
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
                                                    << "[" << string_VkResult (result) << "]"
                                                    << std::endl; 
                    throw std::runtime_error ("Failed to acquire swap chain image");
                }
                /* After waiting for fence, we need to manually reset the fence to the unsignaled state immediately after.
                 * But we delay it to upto this point to avoid deadlock on inFlightFence
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
                if (refreshModelTransform == true) {
                    createModelMatrix (modelInfoId, 
                                       handOffInfo->resource.transformInfo.model.translate,
                                       handOffInfo->resource.transformInfo.model.rotateAxis, 
                                       handOffInfo->resource.transformInfo.model.rotateAngleDeg,
                                       handOffInfo->resource.transformInfo.model.scale);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - CAMERA TRANSFORM                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                if (refreshCameraTransform == true) {
                    createViewMatrix (cameraInfoId, 
                                      handOffInfo->resource.transformInfo.camera.position,
                                      handOffInfo->resource.transformInfo.camera.center,
                                      handOffInfo->resource.transformInfo.camera.upVector);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - UPDATE UNIFORMS                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                MVPMatrixUBO mvpMatrix;
                mvpMatrix.model      = modelInfo->meta.modelMatrix;
                mvpMatrix.view       = cameraInfo->meta.viewMatrix;
                mvpMatrix.projection = cameraInfo->meta.projectionMatrix;

                updateUniformBuffer (modelInfo->id.uniformBufferInfos[m_currentFrame],
                                     sizeof (MVPMatrixUBO),
                                     &mvpMatrix);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - RECORD AND SUBMIT                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* First, we call vkResetCommandBuffer on the command buffer to make sure it is able to be recorded
                */
                vkResetCommandBuffer (handOffInfo->resource.commandBuffers[m_currentFrame], 0);
                beginRecording       (handOffInfo->resource.commandBuffers[m_currentFrame], 0, nullptr);
                /* Define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operation for 
                 * the attachments. Note that, the order of clearValues should be identical to the order of your 
                 * attachments
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
                beginRenderPass      (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      renderPassInfoId,
                                      resourceId,
                                      swapChainImageId,
                                      clearValues);

                bindPipeline         (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      pipelineInfoId,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS);

                auto secondaryViewPorts = std::vector <VkViewport> {};
                setViewPorts         (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      resourceId,
                                      0,
                                      secondaryViewPorts);

                auto secondaryScissors = std::vector <VkRect2D> {};
                setScissors          (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      resourceId,
                                      0,
                                      secondaryScissors);

                auto vertexBufferInfoIdsToBind = std::vector {
                    modelInfo->id.vertexBufferInfo
                };
                auto offsets = std::vector <VkDeviceSize> {
                    0
                };
                bindVertexBuffers    (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      vertexBufferInfoIdsToBind,
                                      0,
                                      offsets);

                bindIndexBuffer      (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      modelInfo->id.indexBufferInfo,
                                      0,
                                      VK_INDEX_TYPE_UINT32);

                auto descriptorSetsToBind = std::vector {
                    modelInfo->resource.descriptorSets[m_currentFrame]
                };        
                bindDescriptorSets   (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      0,
                                      descriptorSetsToBind,
                                      pipelineInfoId,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS);

                drawIndexed          (handOffInfo->resource.commandBuffers[m_currentFrame],
                                      modelInfo->meta.indicesCount,
                                      1, 0, 0, 0);

                endRenderPass        (handOffInfo->resource.commandBuffers[m_currentFrame]);
                endRecording         (handOffInfo->resource.commandBuffers[m_currentFrame]);  

                VkSubmitInfo drawOpsSubmitInfo{};
                drawOpsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                /* The first three parameters specify which semaphores to wait on before execution begins and in which 
                 * stage(s) of the pipeline to wait. We want to wait with writing colors to the image until it's 
                 * available, so we're specifying the stage of the graphics pipeline that writes to the color attachment. 
                 * That means that theoretically the implementation can already start executing our vertex shader and 
                 * such while the image is not yet available
                 * 
                 * Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
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
                drawOpsSubmitInfo.pCommandBuffers    = &handOffInfo->resource.commandBuffers[m_currentFrame];
                /* The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the 
                 * command buffer(s) have finished execution
                */
                auto renderDoneSemaphoreInfoId = handOffInfo->id.renderDoneSemaphoreInfos[m_currentFrame];
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
                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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
                /* Applications that do not need per-swapchain results can use NULL for pResults. If non-NULL, each 
                 * entry in pResults will be set to the VkResult for presenting the swapchain corresponding to the same 
                 * index in pSwapchains
                 * 
                 * It's not necessary if you're only using a single swap chain, because you can simply use the return 
                 * value of the present function
                */
                presentInfo.pResults = nullptr;

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
                 * the associated swapchain image
                */
                result = vkQueuePresentKHR (deviceInfo->unique[resourceId].presentQueue, &presentInfo);
                /* Why didn't we check 'framebufferResized' boolean after vkAcquireNextImageKHR?
                 * It is important to note that a signalled semaphore can only be destroyed by vkDeviceWaitIdle if it is 
                 * being waited on by a vkQueueSubmit. Since we are handling the resize explicitly using the boolean, 
                 * returning after vkAcquireNextImageKHR (thus calling vkDeviceWaitIdle) will make the semaphore signalled 
                 * but have nothing waiting on it
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFrameBufferResized()) {
                    LOG_WARNING (m_VKDrawSequenceLog) << "Failed to present swap chain image " 
                                                      << "[" << string_VkResult (result) << "]" 
                                                      << std::endl; 
                    setFrameBufferResized (false);
                    recreateSwapChainDeps (modelInfoId, 
                                           renderPassInfoId,
                                           resourceId);
                }
                else if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDrawSequenceLog) << "Failed to present swap chain image " 
                                                    << "[" << string_VkResult (result) << "]" 
                                                    << std::endl;
                    throw std::runtime_error ("Failed to present swap chain image");
                } 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - UPDATE CURRENT FRAME COUNT                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                m_currentFrame = (m_currentFrame + 1) % g_maxFramesInFlight;        
            }

            void readyHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end()) {
                    LOG_ERROR (m_VKDrawSequenceLog) << "Hand off info id already exists "
                                                    << "[" << handOffInfoId << "]"
                                                    << std::endl;
                    throw std::runtime_error ("Hand off info id already exists");
                }

                HandOffInfo info{};
                m_handOffInfoPool[handOffInfoId] = info;
            }

            HandOffInfo* getHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end())
                    return &m_handOffInfoPool[handOffInfoId];
                
                LOG_ERROR (m_VKDrawSequenceLog) << "Failed to find hand off info "
                                                << "[" << handOffInfoId << "]"
                                                << std::endl;
                throw std::runtime_error ("Failed to find hand off info");
            }

            void dumpHandOffInfoPool (void) {
                LOG_INFO (m_VKDrawSequenceLog) << "Dumping hand off info pool"
                                               << std::endl;

                for (auto const& [key, val]: m_handOffInfoPool) {
                    LOG_INFO (m_VKDrawSequenceLog) << "Hand off info id " 
                                                   << "[" << key << "]"
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "In flight fence info ids" 
                                                   << std::endl;
                    for (auto const& infoId: val.id.inFlightFenceInfos) 
                    LOG_INFO (m_VKDrawSequenceLog) << "[" << infoId << "]"
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Image available semaphore info ids" 
                                                   << std::endl;
                    for (auto const& infoId: val.id.imageAvailableSemaphoreInfos) 
                    LOG_INFO (m_VKDrawSequenceLog) << "[" << infoId << "]"
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Render done semaphore info ids" 
                                                   << std::endl;
                    for (auto const& infoId: val.id.renderDoneSemaphoreInfos) 
                    LOG_INFO (m_VKDrawSequenceLog) << "[" << infoId << "]"
                                                   << std::endl;  

                    LOG_INFO (m_VKDrawSequenceLog) << "Command buffers count " 
                                                   << "[" << val.resource.commandBuffers.size() << "]"
                                                   << std::endl;      

                    LOG_INFO (m_VKDrawSequenceLog) << "Model transform info" 
                                                   << std::endl;  
                    LOG_INFO (m_VKDrawSequenceLog) << "Translate "
                                                   << "[" << val.resource.transformInfo.model.translate.x << ", "
                                                          << val.resource.transformInfo.model.translate.y << ", "
                                                          << val.resource.transformInfo.model.translate.z
                                                   << "]"  
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Rotate axis "
                                                   << "[" << val.resource.transformInfo.model.rotateAxis.x << ", "
                                                          << val.resource.transformInfo.model.rotateAxis.y << ", "
                                                          << val.resource.transformInfo.model.rotateAxis.z
                                                   << "]"  
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Scale "
                                                   << "[" << val.resource.transformInfo.model.scale.x << ", "
                                                          << val.resource.transformInfo.model.scale.y << ", "
                                                          << val.resource.transformInfo.model.scale.z
                                                   << "]"  
                                                   << std::endl;                                                   

                    LOG_INFO (m_VKDrawSequenceLog) << "Rotate angle degrees "
                                                   << "[" << val.resource.transformInfo.model.rotateAngleDeg << "]" 
                                                   << std::endl; 

                    LOG_INFO (m_VKDrawSequenceLog) << "Camera transform info" 
                                                   << std::endl;   
                    LOG_INFO (m_VKDrawSequenceLog) << "Position "
                                                   << "[" << val.resource.transformInfo.camera.position.x << ", "
                                                          << val.resource.transformInfo.camera.position.y << ", "
                                                          << val.resource.transformInfo.camera.position.z
                                                   << "]"  
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Center "
                                                   << "[" << val.resource.transformInfo.camera.center.x << ", "
                                                          << val.resource.transformInfo.camera.center.y << ", "
                                                          << val.resource.transformInfo.camera.center.z
                                                   << "]"  
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Up vector "
                                                   << "[" << val.resource.transformInfo.camera.upVector.x << ", "
                                                          << val.resource.transformInfo.camera.upVector.y << ", "
                                                          << val.resource.transformInfo.camera.upVector.z
                                                   << "]"  
                                                   << std::endl;  

                    LOG_INFO (m_VKDrawSequenceLog) << "FOV degrees "
                                                   << "[" << val.resource.transformInfo.camera.fovDeg << "]" 
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Near plane "
                                                   << "[" << val.resource.transformInfo.camera.nearPlane << "]" 
                                                   << std::endl;

                    LOG_INFO (m_VKDrawSequenceLog) << "Far plane "
                                                   << "[" << val.resource.transformInfo.camera.farPlane << "]" 
                                                   << std::endl;                                                   
                }              
            }

            void cleanUp (uint32_t handOffInfoId) {
                deleteHandOffInfo (handOffInfoId);
            }
    };

    Log::Record* VKDrawSequence::m_VKDrawSequenceLog;
}   // namespace Renderer
#endif  // VK_DRAW_SEQUENCE_H