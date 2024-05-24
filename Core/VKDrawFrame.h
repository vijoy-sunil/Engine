#ifndef VK_DRAW_FRAME_H
#define VK_DRAW_FRAME_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKCmdBuffer.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKDrawFrame: protected VKCmdBuffer {
        private:
            /* To use the right objects (command buffers and sync objects) every frame, keep track of the current frame
            */
            uint32_t m_currentFrame;
            /* We'll need one semaphore to signal that an image has been acquired from the swapchain and is ready for 
             * rendering, another one to signal that rendering has finished and presentation can happen, and a fence to 
             * make sure only one frame is rendering at a time, but since we can handle multiple frames in flight, each 
             * frame should have its own set of semaphores and fence
            */
            std::vector <VkSemaphore> m_imageAvailableSemaphores;
            std::vector <VkSemaphore> m_renderFinishedSemaphores;
            std::vector <VkFence> m_inFlightFences;
            /* Handle to the log object
            */
            static Log::Record* m_VKDrawFrameLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 6;

        public:
            VKDrawFrame (void) {
                m_currentFrame = 0;

                m_VKDrawFrameLog = LOG_INIT (m_instanceId, 
                                             Log::VERBOSE, 
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
                LOG_INFO (m_VKDrawFrameLog) << "Constructor called" << std::endl; 
            }

            ~VKDrawFrame (void) {
                LOG_INFO (m_VKDrawFrameLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }
            
        protected:
            /* A core design philosophy in Vulkan is that synchronization of execution on the GPU is explicit. The order 
             * of operations is up to us to define using various synchronization primitives which tell the driver the 
             * order we want things to run in. This means that many Vulkan API calls which start executing work on the 
             * GPU are asynchronous, the functions will return before the operation has finished and there are a number 
             * of events that we need to order explicitly
            */
            void createSyncObjects (void) {
                /* A semaphore is used to add order between queue operations. Queue operations refer to the work we 
                 * submit to a queue, either in a command buffer or from within a function. Examples of queues are the 
                 * graphics queue and the presentation queue. Semaphores are used both to order work inside the same 
                 * queue and between different queues
                 * 
                 * The way we use a semaphore to order queue operations is by providing the same semaphore as a 'signal' 
                 * semaphore in one queue operation and as a 'wait' semaphore in another queue operation. For example, 
                 * lets say we have semaphore S and queue operations A and B that we want to execute in order. What we 
                 * tell Vulkan is that operation A will 'signal' semaphore S when it finishes executing, and operation B 
                 * will 'wait' on semaphore S before it begins executing. When operation A finishes, semaphore S will be 
                 * signaled, while operation B wont start until S is signaled. After operation B begins executing, 
                 * semaphore S is automatically reset back to being unsignaled, allowing it to be used again
                 * 
                 * Note that, the waiting only happens on the GPU. The CPU continues running without blocking
                */
                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                /* A fence has a similar purpose, in that it is used to synchronize execution, but it is for ordering the 
                 * execution on the CPU, otherwise known as the host. Simply put, if the host needs to know when the GPU 
                 * has finished something, we use a fence.
                 * 
                 * Whenever we submit work to execute, we can attach a fence to that work. When the work is finished, the 
                 * fence will be signaled. Then we can make the host wait for the fence to be signaled, guaranteeing that 
                 * the work has finished before the host continues
                 * 
                 * Fences must be reset manually to put them back into the unsignaled state. This is because fences are 
                 * used to control the execution of the host, and so the host gets to decide when to reset the fence. 
                 * Contrast this to semaphores which are used to order work on the GPU without the host being involved
                */
                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                /* On the first frame we call drawFrame(), which immediately waits on inFlightFence to be signaled. 
                 * inFlightFence is only signaled after a frame has finished rendering, yet since this is the first frame, 
                 * there are no previous frames in which to signal the fence! Thus vkWaitForFences() blocks indefinitely, 
                 * waiting on something which will never happen. To combat this, create the fence in the signaled state, 
                 * so that the first call to vkWaitForFences() returns immediately since the fence is already signaled
                */
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                /* Create required semaphores and fences
                */
                m_imageAvailableSemaphores.resize (getMaxFramesInFlight());
                m_renderFinishedSemaphores.resize (getMaxFramesInFlight());
                m_inFlightFences.resize (getMaxFramesInFlight());

                for (size_t i = 0; i < getMaxFramesInFlight(); i++) {
                    VkResult result_0 = vkCreateSemaphore (getLogicalDevice(), 
                                                           &semaphoreInfo, 
                                                           nullptr, 
                                                           &m_imageAvailableSemaphores[i]);
                    if (result_0 != VK_SUCCESS) {
                        LOG_ERROR (m_VKDrawFrameLog) << "Failed to create image availbale semaphore" 
                                                     << " " 
                                                     << result_0 
                                                     << std::endl;
                        throw std::runtime_error ("Failed to create image availbale semaphore");
                    }

                    VkResult result_1 = vkCreateSemaphore (getLogicalDevice(), 
                                                           &semaphoreInfo, 
                                                           nullptr, 
                                                           &m_renderFinishedSemaphores[i]);
                    if (result_1 != VK_SUCCESS) {
                        LOG_ERROR (m_VKDrawFrameLog) << "Failed to create render finished semaphore" 
                                                     << " " 
                                                     << result_1 
                                                     << std::endl;
                        throw std::runtime_error ("Failed to create render finished semaphore");
                    }

                    VkResult result_2 = vkCreateFence (getLogicalDevice(), 
                                                       &fenceInfo, 
                                                       nullptr, 
                                                       &m_inFlightFences[i]);
                    if (result_2 != VK_SUCCESS) {
                        LOG_ERROR (m_VKDrawFrameLog) << "Failed to create in flight fence" 
                                                     << " " 
                                                     << result_2 
                                                     << std::endl;
                        throw std::runtime_error ("Failed to create in flight fence");
                    }
                }
            }

            /* At a high level, rendering a frame in Vulkan consists of a common set of steps:
             * (1) Wait for the previous frame to finish
             * (2) Acquire an image from the swap chain
             * (3) Record a command buffer which draws the scene onto that image
             * (4) Submit the recorded command buffer into the queue
             * (5) Present the swap chain image
            */
            void drawFrame (void) {
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
                vkWaitForFences (getLogicalDevice(), 1, &m_inFlightFences [m_currentFrame], VK_TRUE, UINT64_MAX);

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
                                                         m_imageAvailableSemaphores [m_currentFrame], 
                                                         VK_NULL_HANDLE, 
                                                         &imageIndex);

                /* If the swap chain turns out to be out of date when attempting to acquire an image, then it is no longer 
                 * possible to present to it. Therefore we should immediately recreate the swap chain and try again in 
                 * the next drawFrame call
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    LOG_WARNING (m_VKDrawFrameLog) << "Failed to acquire swap chain image" << " " << result << std::endl; 
                    recreateSwapChain();
                    return;
                }
                /* You could also decide to recreate and return if the swap chain is suboptimal, but we've chosen to 
                 * proceed anyway in that case because we've already acquired an image. Both VK_SUCCESS and 
                 * VK_SUBOPTIMAL_KHR are considered "success" return codes here
                */
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    LOG_ERROR (m_VKDrawFrameLog) << "Failed to acquire swap chain image" << " " << result << std::endl; 
                    throw std::runtime_error ("Failed to acquire swap chain image");
                }

                /* After waiting for fence, we need to manually reset the fence to the unsignaled state immediately after.
                 * But we delay it to upto this point to avoid deadlock on inFlightFence
                 *
                 * When vkAcquireNextImageKHR returns VK_ERROR_OUT_OF_DATE_KHR, we recreate the swapchain and then return 
                 * from drawFrame. But before that happens, the current frame's fence was waited upon and reset. Since we 
                 * return immediately, no work is submitted for execution and the fence will never be signaled, causing 
                 * vkWaitForFences to halt forever.
                 * 
                 * To overcome this, delay resetting the fence until after we know for sure we will be submitting work 
                 * with it. Thus, if we return early, the fence is still signaled and vkWaitForFences wont deadlock the 
                 * next time we use the same fence object
                */
                vkResetFences (getLogicalDevice(), 1, &m_inFlightFences [m_currentFrame]);

                /* (3)
                 * First, we call vkResetCommandBuffer on the command buffer to make sure it is able to be recorded. 
                 * Then, we use the recordCommandBuffer function to record the commands we want
                */
                vkResetCommandBuffer (getCommandBuffers() [m_currentFrame], 0);
                recordCommandBuffer (getCommandBuffers() [m_currentFrame], imageIndex);

                /* (4)
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
                VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores [m_currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                /* The next two parameters specify which command buffers to actually submit for execution
                */
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &getCommandBuffers() [m_currentFrame];
                /* The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the 
                 * command buffer(s) have finished execution
                */
                VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores [m_currentFrame]};
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
                                        m_inFlightFences [m_currentFrame]);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDrawFrameLog) << "Failed to submit draw command buffer" << " " << result << std::endl; 
                    throw std::runtime_error ("Failed to submit draw command buffer");                    
                }

                /* (5)
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
                */
                result = vkQueuePresentKHR (getPresentQueue(), &presentInfo);
                /* Why didn't we check 'framebufferResized' boolean after vkAcquireNextImageKHR?
                 * It is important to note that a signalled semaphore can only be destroyed by vkDeviceWaitIdle if it is 
                 * being waited on by a vkQueueSubmit. Since we are handling the resize explicitly using the boolean, 
                 * returning after vkAcquireNextImageKHR (thus calling vkDeviceWaitIdle) will make the semaphore signalled 
                 * but have nothing waiting on it
                */
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFrameBufferResized()) {
                    LOG_WARNING (m_VKDrawFrameLog) << "Failed to present swap chain image" << " " << result << std::endl; 
                    setFrameBufferResized (false);
                    recreateSwapChain();
                }
                else if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKDrawFrameLog) << "Failed to present swap chain image" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to present swap chain image");
                }

                /* Update frame index to loop around MAX_FRAMES_IN_FLIGHT
                */
                m_currentFrame = (m_currentFrame + 1) % getMaxFramesInFlight();
            }

            void cleanUp (void) {
                /* Destroy synchronization primitives
                */
                for (size_t i = 0; i < getMaxFramesInFlight(); i++) {
                    vkDestroySemaphore (getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
                    vkDestroySemaphore (getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
                    vkDestroyFence (getLogicalDevice(), m_inFlightFences[i], nullptr);
                }                
            }
    };

    Log::Record* VKDrawFrame::m_VKDrawFrameLog;
}   // namespace Renderer
#endif  // VK_DRAW_FRAME_H