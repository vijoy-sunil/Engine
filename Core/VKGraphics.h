#ifndef VK_GRAPHICS_H
#define VK_GRAPHICS_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSyncObjects.h"
#include "VKCmdBuffer.h"
#include "VKRecord.h"
#include "VKResizing.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKGraphics: protected virtual VKSyncObjects,
                      protected virtual VKCmdBuffer,
                      protected virtual VKRecord,
                      protected VKResizing {
        private:
            /* Handle to command pool
            */
            VkCommandPool m_commandPool;
            /* Handle to command buffers
            */
            std::vector <VkCommandBuffer> m_commandBuffers;
            /* To use the right objects (command buffers and sync objects) every frame, keep track of the current frame
            */
            uint32_t m_currentFrame;
            /* Handle to the log object
            */
            static Log::Record* m_VKGraphicsLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKGraphics (void) {
                m_currentFrame = 0;

                m_VKGraphicsLog = LOG_INIT (m_instanceId, 
                                            static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE),
                                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                            "./Build/Log/"); 
            }

            ~VKGraphics (void) {
                LOG_CLOSE (m_instanceId);
            }
            
        protected:
            /* Create command pool and command buffers
            */
            void readyCommandBuffers (void) {
                /* We will be recording a command buffer every frame, so we want to be able to reset and rerecord over 
                 * it. Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command 
                 * pool. And, we're going to record commands for drawing, which is why we've chosen the graphics queue 
                 * family
                */
                createCommandPool (VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 
                                   getGraphicsFamilyIndex(),
                                   m_commandPool);

                createCommandBuffers (m_commandPool, MAX_FRAMES_IN_FLIGHT, m_commandBuffers);
            }

            /* At a high level, rendering a frame in Vulkan consists of a common set of steps:
             * (1) Wait for the previous frame to finish
             * (2) Acquire an image from the swap chain
             * (3) Record a command buffer which draws the scene onto that image
             * (4) Update uniform buffer
             * (5) Submit the recorded command buffer into the queue
             * (6) Present the swap chain image
            */
            void graphicsOps (void) {
                /* (1)
                 * At the start of the frame, we want to wait until the previous frame has finished, so that the command 
                 * buffer and semaphores are available to use. The vkWaitForFences function takes an array of fences and 
                 * waits on the host for either any or all of the fences to be signaled before returning. The VK_TRUE we 
                 * pass here indicates that we want to wait for all fences, but in the case of a single one it doesn't 
                 * matter. This function also has a timeout parameter that we set to the maximum value of a 64 bit 
                 * unsigned integer, UINT64_MAX, which effectively disables the timeout
                 * 
                 * We need to make sure only one frame is being drawn/rendered at a time, why?
                 * We use a fence for waiting on the previous frame to finish, this is so that we don't draw more than 
                 * one frame at a time. Because we re-record the command buffer every frame, we cannot record the next 
                 * frame's work to the command buffer until the current frame has finished executing, as we don't want to 
                 * overwrite the current contents of the command buffer while the GPU is using it
                */
                vkWaitForFences (getLogicalDevice(), 1, &getInFlightFences()[m_currentFrame], VK_TRUE, UINT64_MAX);

                /* (2)
                 * The first two parameters of vkAcquireNextImageKHR are the logical device and the swap chain from which 
                 * we wish to acquire an image. The third parameter specifies a timeout in nanoseconds for an image to 
                 * become available. Using the maximum value of a 64 bit unsigned integer means we effectively disable 
                 * the timeout.
                 * 
                 * The next two parameters specify synchronization objects that are to be signaled when the presentation 
                 * engine is finished using the image. That's the point in time where we can start drawing to it
                 * 
                 * The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick 
                 * the VkFrameBuffer. It just returns the index of the next image that will be available at some point 
                 * notified by the semaphore
                */
                uint32_t imageIndex;
                VkResult result = vkAcquireNextImageKHR (getLogicalDevice(), 
                                                         getSwapChain(), 
                                                         UINT64_MAX, 
                                                         getImageAvailableSemaphores()[m_currentFrame], 
                                                         VK_NULL_HANDLE, 
                                                         &imageIndex);

                /* If the swap chain turns out to be out of date when attempting to acquire an image, then it is no longer 
                 * possible to present to it. Therefore we should immediately recreate the swap chain and try again in 
                 * the next graphicsOps call
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    LOG_WARNING (m_VKGraphicsLog) << "Failed to acquire swap chain image " 
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl; 
                    recreateSwapChain();
                    return;
                }
                /* You could also decide to recreate and return if the swap chain is suboptimal, but we've chosen to 
                 * proceed anyway in that case because we've already acquired an image. Both VK_SUCCESS and 
                 * VK_SUBOPTIMAL_KHR are considered "success" return codes here
                */
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    LOG_ERROR (m_VKGraphicsLog) << "Failed to acquire swap chain image " 
                                                << "[" << string_VkResult (result) << "]"
                                                << std::endl; 
                    throw std::runtime_error ("Failed to acquire swap chain image");
                }

                /* After waiting for fence, we need to manually reset the fence to the unsignaled state immediately after.
                 * But we delay it to upto this point to avoid deadlock on inFlightFence
                 *
                 * When vkAcquireNextImageKHR returns VK_ERROR_OUT_OF_DATE_KHR, we recreate the swapchain and then return 
                 * from graphicsOps. But before that happens, the current frame's fence was waited upon and reset. Since 
                 * we return immediately, no work is submitted for execution and the fence will never be signaled, causing
                 * vkWaitForFences to halt forever.
                 * 
                 * To overcome this, delay resetting the fence until after we know for sure we will be submitting work 
                 * with it. Thus, if we return early, the fence is still signaled and vkWaitForFences wont deadlock the 
                 * next time we use the same fence object
                */
                vkResetFences (getLogicalDevice(), 1, &getInFlightFences()[m_currentFrame]);

                /* (3)
                 * First, we call vkResetCommandBuffer on the command buffer to make sure it is able to be recorded. 
                 * Then, we use the recordCommandBuffer function to record the commands we want
                */
                vkResetCommandBuffer (m_commandBuffers[m_currentFrame], 0);
                recordCommandBuffer (m_commandBuffers[m_currentFrame], imageIndex, m_currentFrame);

                /* (4)
                 * Update uniform buffer before submitting the current frame
                */
                updateUniformBuffer (m_currentFrame);

                /* (5)
                 * Queue submission and synchronization is configured through parameters in the VkSubmitInfo structure
                */
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                /* The first three parameters specify which semaphores to wait on before execution begins and in which 
                 * stage(s) of the pipeline to wait. We want to wait with writing colors to the image until it's 
                 * available, so we're specifying the stage of the graphics pipeline that writes to the color attachment. 
                 * That means that theoretically the implementation can already start executing our vertex shader and 
                 * such while the image is not yet available
                 * 
                 * Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
                */
                VkSemaphore waitSemaphores[] = {getImageAvailableSemaphores()[m_currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                /* The next two parameters specify which command buffers to actually submit for execution
                */
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
                /* The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the 
                 * command buffer(s) have finished execution
                */
                VkSemaphore signalSemaphores[] = {getRenderFinishedSemaphores()[m_currentFrame]};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;
                /* The last parameter references an optional fence that will be signaled when the command buffers finish 
                 * execution. This allows us to know when it is safe for the command buffer to be reused, thus we want 
                 * to give it inFlightFence. Now on the next frame, the CPU will wait for this command buffer to finish 
                 * executing before it records new commands into it
                */
                result = vkQueueSubmit (getGraphicsQueue(), 
                                        1,
                                        &submitInfo,
                                        getInFlightFences()[m_currentFrame]);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsLog) << "Failed to submit draw command buffer " 
                                                << "[" << string_VkResult (result) << "]"
                                                << std::endl; 
                    throw std::runtime_error ("Failed to submit draw command buffer");                    
                }

                /* (6)
                 * After queueing all rendering commands and transitioning the image to the correct layout, it is time to 
                 * queue an image for presentation
                */
                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                /* The first two parameters specify which semaphores to wait on before presentation can happen, just like 
                 * VkSubmitInfo. Since we want to wait on the command buffer to finish execution, we take the semaphores 
                 * which will be signalled and wait on them, thus we use signalSemaphores
                */
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;
                /* The next two parameters specify the swap chains to present images to and the index of the image for 
                 * each swap chain
                */
                VkSwapchainKHR swapChains[] = {getSwapChain()};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                /* Applications that do not need per-swapchain results can use NULL for pResults. If non-NULL, each 
                 * entry in pResults will be set to the VkResult for presenting the swapchain corresponding to the same 
                 * index in pSwapchains
                 * 
                 * It's not necessary if you're only using a single swap chain, because you can simply use the return 
                 * value of the present function
                */
                presentInfo.pResults = nullptr;

                /* The vkQueuePresentKHR function returns the same values with the same meaning as vkAcquireNextImageKHR. 
                 * In this case we will also recreate the swap chain if it is suboptimal, because we want the best 
                 * possible result
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
                result = vkQueuePresentKHR (getPresentQueue(), &presentInfo);
                /* Why didn't we check 'framebufferResized' boolean after vkAcquireNextImageKHR?
                 * It is important to note that a signalled semaphore can only be destroyed by vkDeviceWaitIdle if it is 
                 * being waited on by a vkQueueSubmit. Since we are handling the resize explicitly using the boolean, 
                 * returning after vkAcquireNextImageKHR (thus calling vkDeviceWaitIdle) will make the semaphore signalled 
                 * but have nothing waiting on it
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFrameBufferResized()) {
                    LOG_WARNING (m_VKGraphicsLog) << "Failed to present swap chain image " 
                                                  << "[" << string_VkResult (result) << "]" 
                                                  << std::endl; 
                    setFrameBufferResized (false);
                    recreateSwapChain();
                }
                else if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsLog) << "Failed to present swap chain image " 
                                                << "[" << string_VkResult (result) << "]" 
                                                << std::endl;
                    throw std::runtime_error ("Failed to present swap chain image");
                }

                /* Update frame index to loop around MAX_FRAMES_IN_FLIGHT
                */
                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            void cleanUp (void) {
                /* Destroy command pool, note that command buffers will be automatically freed when their command pool 
                 * is destroyed, so we don't need explicit cleanup
                */
                vkDestroyCommandPool (getLogicalDevice(), m_commandPool, nullptr);
            }
    };

    Log::Record* VKGraphics::m_VKGraphicsLog;
}   // namespace Renderer
#endif  // VK_GRAPHICS_H