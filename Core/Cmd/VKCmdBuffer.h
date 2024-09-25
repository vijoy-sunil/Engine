#ifndef VK_CMD_BUFFER_H
#define VK_CMD_BUFFER_H

#include "../Device/VKDeviceMgr.h"

namespace Core {
    class VKCmdBuffer: protected virtual VKDeviceMgr {
        private:
            Log::Record* m_VKCmdBufferLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKCmdBuffer (void) {
                m_VKCmdBufferLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKCmdBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function
             * calls. You have to record all of the operations you want to perform in command buffer objects. The
             * advantage of this is that when we are ready to tell the Vulkan what we want to do, all of the commands are
             * submitted together and Vulkan can more efficiently process the commands since all of them are available
             * together
             *
             * We have to create a command pool before we can create command buffers. Command pools manage the memory
             * that is used to store the buffers and command buffers are allocated from them
            */
            VkCommandPool getCommandPool (uint32_t deviceInfoId,
                                          VkCommandPoolCreateFlags poolCreateFlags,
                                          uint32_t queueFamilyIndex) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);

                VkCommandPoolCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                /* Command pool possible flags:
                 * (1) VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will
                 * be short-lived, meaning that they will be reset or freed in a relatively short timeframe
                 *
                 * (2) VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool
                 * to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the
                 * implicit reset when calling vkBeginCommandBuffer
                */
                createInfo.flags = poolCreateFlags;
                /* Command buffers are executed by submitting them on one of the device queues. Each command pool can
                 * only allocate command buffers that are submitted on a single type of queue
                */
                createInfo.queueFamilyIndex = queueFamilyIndex;

                VkCommandPool commandPool;
                VkResult result = vkCreateCommandPool (deviceInfo->resource.logDevice,
                                                       &createInfo,
                                                       VK_NULL_HANDLE,
                                                       &commandPool);

                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to create command pool "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create command pool");
                }
                return commandPool;
            }

            std::vector <VkCommandBuffer> getCommandBuffers (uint32_t deviceInfoId,
                                                             VkCommandPool commandPool,
                                                             uint32_t bufferCount,
                                                             VkCommandBufferLevel bufferLevel) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);

                VkCommandBufferAllocateInfo allocInfo;
                allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.pNext              = VK_NULL_HANDLE;
                allocInfo.commandPool        = commandPool;
                allocInfo.commandBufferCount = bufferCount;
                /* The level parameter specifies if the allocated command buffers are primary or secondary command buffers
                 * VK_COMMAND_BUFFER_LEVEL_PRIMARY
                 * Can be submitted to a queue for execution, but cannot be called from other command buffers
                 *
                 * VK_COMMAND_BUFFER_LEVEL_SECONDARY
                 * Cannot be submitted directly, but can be called from primary command buffers
                */
                allocInfo.level = bufferLevel;

                std::vector <VkCommandBuffer> commandBuffers (bufferCount);
                VkResult result = vkAllocateCommandBuffers (deviceInfo->resource.logDevice,
                                                            &allocInfo,
                                                            commandBuffers.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to create command buffers "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create command buffers");
                }
                return commandBuffers;
            }

            void beginRecording (VkCommandBuffer commandBuffer,
                                 VkCommandBufferUsageFlags bufferUsageFlags,
                                 const VkCommandBufferInheritanceInfo* inheritanceInfo) {
                /* We always begin recording a command buffer by calling vkBeginCommandBuffer with a small
                 * VkCommandBufferBeginInfo structure as argument that specifies some details about the usage of this
                 * specific command buffer
                */
                VkCommandBufferBeginInfo beginInfo;
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.pNext = VK_NULL_HANDLE;
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
                */
                beginInfo.flags = bufferUsageFlags;
                /* The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which
                 * state to inherit from the calling primary command buffers
                */
                beginInfo.pInheritanceInfo = inheritanceInfo;

                /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly
                 * reset it. It's not possible to append commands to a buffer at a later time
                */
                VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to begin recording command buffer "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to begin recording command buffer");
                }
            }

            void endRecording (VkCommandBuffer commandBuffer) {
                VkResult result = vkEndCommandBuffer (commandBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to end recording command buffer "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to end recording command buffer");
                }
            }

            void cleanUp (uint32_t deviceInfoId, VkCommandPool commandPool) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                /* Destroy command pool, note that command buffers will be automatically freed when their command pool
                 * is destroyed, so we don't need explicit cleanup
                */
                vkDestroyCommandPool (deviceInfo->resource.logDevice, commandPool, VK_NULL_HANDLE);
            }
    };
}   // namespace Core
#endif  // VK_CMD_BUFFER_H