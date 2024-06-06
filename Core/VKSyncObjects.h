#ifndef VK_SYNC_OBJECTS_H
#define VK_SYNC_OBJECTS_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKLogDevice.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKSyncObjects: protected virtual VKLogDevice {
        private:
            /* We'll need one semaphore to signal that an image has been acquired from the swapchain and is ready for 
             * rendering, another one to signal that rendering has finished and presentation can happen, and a fence to 
             * make sure only one frame is rendering at a time, but since we can handle multiple frames in flight, each 
             * frame should have its own set of semaphores and fence
            */
            std::vector <VkSemaphore> m_imageAvailableSemaphores;
            std::vector <VkSemaphore> m_renderFinishedSemaphores;
            std::vector <VkFence> m_inFlightFences;
            /* We will also need one fence to wait on until the transfers are complete for the vertex and index buffers
            */
            VkFence m_transferCompleteFence;
            /* Handle to the log object
            */
            static Log::Record* m_VKSyncObjectsLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
            
            /* A core design philosophy in Vulkan is that synchronization of execution on the GPU is explicit. The order 
             * of operations is up to us to define using various synchronization primitives which tell the driver the 
             * order we want things to run in. This means that many Vulkan API calls which start executing work on the 
             * GPU are asynchronous, the functions will return before the operation has finished and there are a number 
             * of events that we need to order explicitly
            */
            void createGraphicsSyncObjects (void) {
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
                /* On the first frame we call graphicsOps(), which immediately waits on inFlightFence to be signaled. 
                 * inFlightFence is only signaled after a frame has finished rendering, yet since this is the first frame, 
                 * there are no previous frames in which to signal the fence! Thus vkWaitForFences() blocks indefinitely, 
                 * waiting on something which will never happen. To combat this, create the fence in the signaled state, 
                 * so that the first call to vkWaitForFences() returns immediately since the fence is already signaled
                */
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                /* Create required semaphores and fences
                */
                m_imageAvailableSemaphores.resize (MAX_FRAMES_IN_FLIGHT);
                m_renderFinishedSemaphores.resize (MAX_FRAMES_IN_FLIGHT);
                m_inFlightFences.resize (MAX_FRAMES_IN_FLIGHT);

                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    VkResult result_0 = vkCreateSemaphore (getLogicalDevice(), 
                                                           &semaphoreInfo, 
                                                           nullptr, 
                                                           &m_imageAvailableSemaphores[i]);
                    if (result_0 != VK_SUCCESS) {
                        LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create image availbale semaphore " 
                                                       << "[" << string_VkResult (result_0) << "]"
                                                       << std::endl;
                        throw std::runtime_error ("Failed to create image availbale semaphore");
                    }

                    VkResult result_1 = vkCreateSemaphore (getLogicalDevice(), 
                                                           &semaphoreInfo, 
                                                           nullptr, 
                                                           &m_renderFinishedSemaphores[i]);
                    if (result_1 != VK_SUCCESS) {
                        LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create render finished semaphore " 
                                                       << "[" << string_VkResult (result_1) << "]"
                                                       << std::endl;
                        throw std::runtime_error ("Failed to create render finished semaphore");
                    }

                    VkResult result_2 = vkCreateFence (getLogicalDevice(), 
                                                       &fenceInfo, 
                                                       nullptr, 
                                                       &m_inFlightFences[i]);
                    if (result_2 != VK_SUCCESS) {
                        LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create in flight fence " 
                                                       << "[" << string_VkResult (result_2) << "]" 
                                                       << std::endl;
                        throw std::runtime_error ("Failed to create in flight fence");
                    }
                }
            }

            void createTransferSyncObjects (void) {
                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                VkResult result = vkCreateFence (getLogicalDevice(), 
                                                 &fenceInfo,
                                                 nullptr,
                                                 &m_transferCompleteFence);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create transfer complete fence " 
                                                   << "[" << string_VkResult (result) << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Failed to create transfer complete fence");
                }
            }

        public:
            VKSyncObjects (void) {
                m_VKSyncObjectsLog = LOG_INIT (m_instanceId, 
                                               static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                               Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                               "./Build/Log/");
            }

            ~VKSyncObjects (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createSyncObjects (void) {
                createGraphicsSyncObjects();
                createTransferSyncObjects();
            }

            std::vector <VkSemaphore>& getImageAvailableSemaphores (void) {
                return m_imageAvailableSemaphores;
            }

            std::vector <VkSemaphore>& getRenderFinishedSemaphores (void) {
                return m_renderFinishedSemaphores;
            }

            std::vector <VkFence>& getInFlightFences (void) {
                return m_inFlightFences;
            }

            VkFence& getTransferCompleteFence (void) {
                return m_transferCompleteFence;
            }

            void cleanUp (void) {
                /* Destroy synchronization primitives
                */
                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    vkDestroySemaphore (getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
                    vkDestroySemaphore (getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
                    vkDestroyFence (getLogicalDevice(), m_inFlightFences[i], nullptr);
                }
                vkDestroyFence (getLogicalDevice(), m_transferCompleteFence, nullptr);              
            }
    };

    Log::Record* VKSyncObjects::m_VKSyncObjectsLog;
}   // namespace Renderer
#endif  // VK_SYNC_OBJECTS_H