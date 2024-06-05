#ifndef VK_TRANSFER_H
#define VK_TRANSFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSyncObjects.h"
#include "VKCmdBuffer.h"
#include "VKRecord.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKTransfer: protected virtual VKSyncObjects,
                      protected virtual VKCmdBuffer,
                      protected virtual VKRecord {
        private:
            /* Handle to command pool
            */
            VkCommandPool m_commandPool;
            /* Handle to command buffers
            */
            std::vector <VkCommandBuffer> m_commandBuffers;
            /* Handle to the log object
            */
            static Log::Record* m_VKTransferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++; 

        public:
            VKTransfer (void) {
                m_VKTransferLog = LOG_INIT (m_instanceId, 
                                            static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                            "./Build/Log/");
            }

            ~VKTransfer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Create command pool and command buffers
            */
            void readyCommandBuffers (void) {
                /* Note that the command buffers that we will be submitting to the transfer queue will be short lived, so 
                 * we will choose the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag. And, this buffer copy command requires a 
                 * queue family that supports transfer operations, which is indicated using VK_QUEUE_TRANSFER_BIT
                */
                createCommandPool (VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 
                                   getTransferFamilyIndex(),
                                   m_commandPool);

                createCommandBuffers (m_commandPool, MAX_TRANSFERS_IN_QUEUE, m_commandBuffers);
            }

            /* Vertex and index buffers have already been setup, we can now move the vertex and index data to the device 
             * local buffers
            */
            void transferOps (void) {                
                /* Record command buffer (index 0)
                 * We're only going to use the command buffer once and wait (vkQueueWaitIdle/vkWaitForFences) with 
                 * returning from the function until the copy operation has finished executing. It's good practice to 
                 * tell the driver about our intent using VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
                */
                VkDeviceSize vertexBufferSize = sizeof (getVertices()[0]) * getVertices().size();
                recordCommandBuffer (m_commandBuffers[0],
                                     VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                     VKVertexBuffer::getStagingBuffer(), 
                                     getVertexBuffer(), 
                                     vertexBufferSize);

                /* Record command buffer (index 1)
                */
                VkDeviceSize indexBufferSize = sizeof (getIndices()[0]) * getIndices().size();
                recordCommandBuffer (m_commandBuffers[1],
                                     VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
                                     VKIndexBuffer::getStagingBuffer(), 
                                     getIndexBuffer(), 
                                     indexBufferSize);
                
                /* Submit command buffer to transfer queue
                */
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = static_cast <uint32_t> (m_commandBuffers.size());
                submitInfo.pCommandBuffers = m_commandBuffers.data();
                vkQueueSubmit (getTransferQueue(), 1, &submitInfo, getTransferCompleteFence());

                /* Wait for fence 
                 * Unlike the draw commands, there are no events we need to wait on this time. We just want to execute 
                 * the transfer on the buffers immediately. There are again two possible ways to wait on this transfer 
                 * to complete. 
                 * 
                 * (1) We could use a fence and wait with vkWaitForFences, or 
                 * (2) Simply wait for the transfer queue to become idle via vkQueueWaitIdle (getTransferQueue());
                 * 
                 * A fence would allow you to schedule multiple transfers simultaneously and wait for all of them 
                 * complete, instead of executing one at a time. That may give the driver more opportunities to optimize.
                */
                LOG_INFO (m_VKTransferLog) << "Waiting for getTransferCompleteFence" << std::endl;
                vkWaitForFences (getLogicalDevice(), 1, &getTransferCompleteFence(), VK_TRUE, UINT64_MAX);
                vkResetFences (getLogicalDevice(), 1, &getTransferCompleteFence());
                LOG_INFO (m_VKTransferLog) << "Reset getTransferCompleteFence" << std::endl;

                /* The vertex and index data are now being loaded from high performance memory, next we should clean up 
                 * the staging buffer handles
                */
                VKVertexBuffer::cleanUpStaging();
                VKIndexBuffer::cleanUpStaging();

                /* All that remains is binding the vertex and index buffer to the graphics command buffer, which is done 
                 * in the recordCommandBuffer function for the graphics queue
                */
            }

            void cleanUp (void) {
                /* Destroy command pool, note that command buffers will be automatically freed when their command pool 
                 * is destroyed, so we don't need explicit cleanup
                */
                vkDestroyCommandPool (getLogicalDevice(), m_commandPool, nullptr);
            }
    };

    Log::Record* VKTransfer::m_VKTransferLog;
}   // namespace Renderer
#endif  // VK_TRANSFER_H