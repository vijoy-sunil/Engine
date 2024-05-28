#ifndef VK_TRANSFER_CMD_BUFFER_H
#define VK_TRANSFER_CMD_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKPipeline.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKTransferCmdBuffer: protected VKPipeline {
        private:
            /* Handle to command pool
            */
            VkCommandPool m_commandPool;
            /* Handle to command buffers
            */
            std::vector <VkCommandBuffer> m_commandBuffers;
            /* Handle to the log object
            */
            static Log::Record* m_VKTransferCmdBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 23;
            
        public:
            VKTransferCmdBuffer (void) {
                m_VKTransferCmdBufferLog = LOG_INIT (m_instanceId, 
                                                     static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                                     Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                     "./Build/Log/");
            }

            ~VKTransferCmdBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Memory transfer operations are executed using command buffers, just like drawing commands. Therefore we 
             * must first create a separate command pool for command buffers to be submitted to the transfer queue 
            */
            void createCommandPool (void) {
                VkCommandPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                /* Command pool possible flags:
                 * (1) VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will 
                 * be short-lived, meaning that they will be reset or freed in a relatively short timeframe
                 * 
                 * (2) VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool 
                 * to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the 
                 * implicit reset when calling vkBeginCommandBuffer
                 * 
                 * The command buffers that we will be submitting to the transfer queue will be short lived, so we will 
                 * choose the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
                */
                poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                /* The buffer copy command requires a queue family that supports transfer operations, which is indicated 
                 * using VK_QUEUE_TRANSFER_BIT. The good news is that any queue family with VK_QUEUE_GRAPHICS_BIT or 
                 * VK_QUEUE_COMPUTE_BIT capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations
                 * 
                 * Let us check if we have a queue family with the VK_QUEUE_TRANSFER_BIT bit
                */
                QueueFamilyIndices queueFamilyIndices = checkQueueFamilySupport (getPhysicalDevice());
                poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

                VkResult result = vkCreateCommandPool (getLogicalDevice(), 
                                                       &poolInfo, 
                                                       nullptr, 
                                                       &m_commandPool);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKTransferCmdBufferLog) << "Failed to create command pool" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to create command pool");
                }
            }

            void createCommandBuffers (void) {
                m_commandBuffers.resize (MAX_TRANSFERS_IN_QUEUE);

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = m_commandPool;
                allocInfo.commandBufferCount = static_cast <uint32_t> (m_commandBuffers.size());
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                
                VkResult result = vkAllocateCommandBuffers (getLogicalDevice(), 
                                                            &allocInfo, 
                                                            m_commandBuffers.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKTransferCmdBufferLog) << "Failed to create command buffers" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to create command buffers");
                }           
            }

            void recordCommandBuffer (VkCommandBuffer commandBuffer, 
                                      VkBuffer srcBuffer, 
                                      VkBuffer dstBuffer, 
                                      VkDeviceSize size) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                /* The flags parameter specifies how we're going to use the command buffer
                 * (1) VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that each recording of the command buffer 
                 * will only be submitted once, and the command buffer will be reset and recorded again between each 
                 * submission
                 * 
                 * (2) VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a secondary command buffer is 
                 * considered to be entirely inside a render pass. If this is a primary command buffer, then this bit is 
                 * ignored
                 * 
                 * (3) VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that a command buffer can be resubmitted 
                 * to any queue of the same queue family while it is in the pending state, and recorded into multiple 
                 * primary command buffers
                 * 
                 * We're only going to use the command buffer once and wait (vkQueueWaitIdle/vkWaitForFences) with 
                 * returning from the function until the copy operation has finished executing. It's good practice to 
                 * tell the driver about our intent using VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
                */
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                beginInfo.pInheritanceInfo = nullptr;   

                VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKTransferCmdBufferLog) << "Failed to begin recording command buffer" 
                                                         << " "
                                                         << result
                                                         << std::endl;
                    throw std::runtime_error ("Failed to begin recording command buffer");
                }             

                /* (1) Copy cmd
                 *
                 * Contents of buffers are transferred using the vkCmdCopyBuffer command. It takes the source and 
                 * destination buffers as arguments, and an array of regions to copy. The regions are defined in 
                 * VkBufferCopy structs and consist of a source buffer offset, destination buffer offset and size
                */
                VkBufferCopy copyRegion{};
                copyRegion.srcOffset = 0; 
                copyRegion.dstOffset = 0;
                copyRegion.size = size;
                vkCmdCopyBuffer (commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

                /* Finish recording command
                */
                result = vkEndCommandBuffer (commandBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKTransferCmdBufferLog) << "Failed to record command buffer" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to record command buffer");
                }            
            }

            std::vector <VkCommandBuffer>& getCommandBuffers (void) {
                return m_commandBuffers;
            }

            void cleanUp (void) {
                vkDestroyCommandPool (getLogicalDevice(), m_commandPool, nullptr);
            }
    };

    Log::Record* VKTransferCmdBuffer::m_VKTransferCmdBufferLog;
}   // namespace Renderer
#endif  // VK_TRANSFER_CMD_BUFFER_H