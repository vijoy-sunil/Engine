#ifndef VK_GENERIC_BUFFER_H
#define VK_GENERIC_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKLogDevice.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKGenericBuffer: protected virtual VKLogDevice {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKGenericBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 25;
            /* Graphics cards can offer different types of memory to allocate from. Each type of memory varies in terms 
             * of allowed operations and performance characteristics. We need to combine the requirements of the buffer 
             * (VkMemoryRequirements) and our own application requirements to find the right type of memory to use
            */
            uint32_t findMemoryType (uint32_t typeFilter, VkMemoryPropertyFlags properties) {
                /* First we need to query info about the available types of memory
                 *
                 * The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. Memory 
                 * heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out. 
                 * The different types of memory exist within these heaps. Right now we'll only concern ourselves with 
                 * the type of memory and not the heap it comes from, but you can imagine that this can affect 
                 * performance
                */
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties (getPhysicalDevice(), &memProperties);
                /* If there is a memory type suitable for the buffer that also has all of the properties we need, then 
                 * we return its index, otherwise we throw an exception 
                 *
                 * The memoryTypes array consists of VkMemoryType structs that specify the heap and properties of each 
                 * type of memory. The properties define special features of the memory, like being able to map it so we 
                 * can write to it from the CPU (indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                */
                for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                    if ((typeFilter & (1 << i)) && 
                        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                        return i;
                }

                LOG_ERROR (m_VKGenericBufferLog) << "Failed to find suitable memory type" << std::endl;
                throw std::runtime_error ("Failed to find suitable memory type");
            }

        public:
            VKGenericBuffer (void) {
                m_VKGenericBufferLog = LOG_INIT (m_instanceId, 
                                                 static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                                 Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                 "./Build/Log/");
            }

            ~VKGenericBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createGenericBuffer (VkDeviceSize size, 
                                      VkBufferUsageFlags usage, 
                                      VkMemoryPropertyFlags properties, 
                                      VkBuffer& buffer, 
                                      VkDeviceMemory& bufferMemory) {
                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                /* Specify the size of the buffer in bytes
                */
                bufferInfo.size = size;
                /* The usage field indicates for which purposes the data in the buffer is going to be used. It is 
                 * possible to specify multiple purposes using a bitwise or
                */
                bufferInfo.usage = usage;
                /* Just like the images in the swap chain, buffers can also be owned by a specific queue family or be 
                 * shared between multiple at the same time.
                */
                QueueFamilyIndices indices = checkQueueFamilySupport (getPhysicalDevice());
                uint32_t queueFamilyIndices[] = { 
                    indices.graphicsFamily.value(), 
                    indices.transferFamily.value()
                };

                /* If the queue families differ, then we'll be using the concurrent mode (buffers can be used across 
                 * multiple queue families without explicit ownership transfers.) Concurrent mode requires you to specify 
                 * in advance between which queue families ownership will be shared using the queueFamilyIndexCount and 
                 * pQueueFamilyIndices parameters
                */
                if (indices.graphicsFamily != indices.transferFamily) {
                    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
                    bufferInfo.queueFamilyIndexCount = 2;
                    bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
                }
                /* If the queue families are the same, then we should stick to exclusive mode (A buffer is owned by one 
                 * queue family at a time and ownership must be explicitly transferred before using it in another queue 
                 * family. This option offers the best performance.)
                */
                else {
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    bufferInfo.queueFamilyIndexCount = 0;
                    bufferInfo.pQueueFamilyIndices = nullptr;
                }
                /* The flags parameter is used to configure sparse buffer memory, which is not relevant right now. We'll 
                 * leave it at the default value of 0
                */
                bufferInfo.flags = 0;

                VkResult result = vkCreateBuffer (getLogicalDevice(), &bufferInfo, nullptr, &buffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGenericBufferLog) << "Failed to create buffer" << " " << result << std::endl; 
                    throw std::runtime_error ("Failed to create buffer");
                }

                /* The buffer has been created, but it doesn't actually have any memory assigned to it yet. The first 
                 * step of allocating memory for the buffer is to query its memory requirements
                 *
                 * The VkMemoryRequirements struct has three fields:
                 * (1) size: The size of the required amount of memory in bytes, may differ from bufferInfo.size
                 * (2) alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends 
                 * on bufferInfo.usage and bufferInfo.flags
                 * (3) memoryTypeBits: Bit field of the memory types that are suitable for the buffer
                */
                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements (getLogicalDevice(), buffer, &memRequirements);

                /* Next, we can allocate the memory by filling in the VkMemoryAllocateInfo structure
                */
                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                /* Memory allocation is now as simple as specifying the size and type, both of which are derived from 
                 * the memory requirements of the buffer and the desired property
                */
                allocInfo.allocationSize = memRequirements.size;
                /* Find suitable memory type
                */
                allocInfo.memoryTypeIndex = findMemoryType (memRequirements.memoryTypeBits, properties);

                /* It should be noted that in a real world application, you're not supposed to actually call 
                 * vkAllocateMemory for every individual buffer. The maximum number of simultaneous memory allocations 
                 * is limited by the maxMemoryAllocationCount physical device limit, which may be as low as 4096 even 
                 * on high end hardware like an NVIDIA GTX 1080. The right way to allocate memory for a large number of 
                 * objects at the same time is to create a custom allocator that splits up a single allocation among 
                 * many different objects by using the offset parameters that we've seen in many functions, or, use
                 * VulkanMemoryAllocator library
                 * 
                 * It is also recommended to store multiple buffers, like the vertex and index buffer, into a single 
                 * VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. The advantage is that your data is 
                 * more cache friendly in that case, because it's closer together
                */
                result = vkAllocateMemory (getLogicalDevice(), &allocInfo, nullptr, &bufferMemory);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGenericBufferLog) << "Failed to allocate buffer memory" 
                                                     << " " 
                                                     << result 
                                                     << std::endl; 
                    throw std::runtime_error ("Failed to allocate buffer memory");
                }

                /* If memory allocation was successful, then we can now associate this memory with the buffer. The fourth 
                 * parameter is the offset within the region of memory that is to be bound to the buffer. If the offset 
                 * is non-zero, then it is required to be divisible by memRequirements.alignment
                */
                vkBindBufferMemory (getLogicalDevice(), buffer, bufferMemory, 0);
            }
    };

    Log::Record* VKGenericBuffer::m_VKGenericBufferLog;
}   // namespace Renderer
#endif  // VK_GENERIC_BUFFER_H