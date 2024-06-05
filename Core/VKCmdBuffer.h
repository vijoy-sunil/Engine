#ifndef VK_CMD_BUFFER_H
#define VK_CMD_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKLogDevice.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKCmdBuffer: protected virtual VKLogDevice {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKCmdBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKCmdBuffer (void) {
                m_VKCmdBufferLog = LOG_INIT (m_instanceId, 
                                             static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
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
            void createCommandPool (VkCommandPoolCreateFlags flags, 
                                    uint32_t queueFamilyIndex,
                                    VkCommandPool& commandPool) {
                VkCommandPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                /* Command pool possible flags:
                 * (1) VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will 
                 * be short-lived, meaning that they will be reset or freed in a relatively short timeframe
                 * 
                 * (2) VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool 
                 * to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the 
                 * implicit reset when calling vkBeginCommandBuffer
                */
                poolInfo.flags = flags;
                /* Command buffers are executed by submitting them on one of the device queues, like the graphics and 
                 * presentation queues we retrieved. Each command pool can only allocate command buffers that are 
                 * submitted on a single type of queue
                */
                poolInfo.queueFamilyIndex = queueFamilyIndex;

                VkResult result = vkCreateCommandPool (getLogicalDevice(), 
                                                       &poolInfo, 
                                                       nullptr, 
                                                       &commandPool);
                
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to create command pool" 
                                                 << " " 
                                                 << result 
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create command pool");
                }
            }

            void createCommandBuffers (VkCommandPool commandPool, 
                                       uint32_t bufferCount,
                                       std::vector <VkCommandBuffer>& commandBuffers) {
                commandBuffers.resize (bufferCount);

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                /* Specify the command pool and number of buffers to allocate
                */
                allocInfo.commandPool = commandPool;
                allocInfo.commandBufferCount = static_cast <uint32_t> (commandBuffers.size());
                /* The level parameter specifies if the allocated command buffers are primary or secondary command buffers
                 * VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from 
                 * other command buffers
                 * VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command 
                 * buffers
                 * We won't make use of the secondary command buffer functionality here, but you can imagine that it's 
                 * helpful to reuse common operations from primary command buffers
                */
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                
                VkResult result = vkAllocateCommandBuffers (getLogicalDevice(), 
                                                            &allocInfo, 
                                                            commandBuffers.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKCmdBufferLog) << "Failed to create command buffers" 
                                                 << " " 
                                                 << result 
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create command buffers");
                } 
            }
    };

    Log::Record* VKCmdBuffer::m_VKCmdBufferLog;
}   // namespace Renderer
#endif  // VK_CMD_BUFFER_H