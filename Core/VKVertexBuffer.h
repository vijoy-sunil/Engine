#ifndef VK_VERTEX_BUFFER_H
#define VK_VERTEX_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSyncObjects.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKVertexBuffer: protected VKSyncObjects {
        private:
            /* Handle to the vertex and index buffer. An index buffer is essentially an array of pointers into the 
             * vertex buffer. It allows you to reorder the vertex data, reuse existing data for multiple vertices and
             * this saving memory when loading complex models
            */
            VkBuffer m_vertexBuffer;
            VkBuffer m_indexBuffer;
            /* Handle to vertex and index buffer memory
            */
            VkDeviceMemory m_vertexBufferMemory;
            VkDeviceMemory m_indexBufferMemory;
            /* Handle to staging buffer and memory for vertex and index buffers
            */
            VkBuffer m_vertexStagingBuffer;
            VkBuffer m_indexStagingBuffer;
            VkDeviceMemory m_vertexStagingBufferMemory;
            VkDeviceMemory m_indexStagingBufferMemory;
            /* Handle to the log object
            */
            static Log::Record* m_VKVertexBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 22; 
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

                LOG_ERROR (m_VKVertexBufferLog) << "Failed to find suitable memory type" << std::endl;
                throw std::runtime_error("Failed to find suitable memory type");
            }

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
                    LOG_ERROR (m_VKVertexBufferLog) << "Failed to create vertex buffer" << " " << result << std::endl; 
                    throw std::runtime_error ("Failed to create vertex buffer");
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
                 * the memory requirements of the vertex buffer and the desired property
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
                    LOG_ERROR (m_VKVertexBufferLog) << "Failed to allocate vertex buffer memory" 
                                                    << " " 
                                                    << result 
                                                    << std::endl; 
                    throw std::runtime_error ("Failed to allocate vertex buffer memory");
                }

                /* If memory allocation was successful, then we can now associate this memory with the buffer
                 * The fourth parameter is the offset within the region of memory. Since this memory is allocated 
                 * specifically for this the vertex buffer, the offset is simply 0. If the offset is non-zero, then it 
                 * is required to be divisible by memRequirements.alignment
                */
                vkBindBufferMemory (getLogicalDevice(), buffer, bufferMemory, 0);
            }

        public:
            VKVertexBuffer (void) {
                m_VKVertexBufferLog = LOG_INIT (m_instanceId, 
                                                Log::VERBOSE, 
                                                Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                "./Build/Log/");
                LOG_INFO (m_VKVertexBufferLog) << "Constructor called" << std::endl; 
            }

            ~VKVertexBuffer (void) {
                LOG_INFO (m_VKVertexBufferLog) << "Destructor called" << std::endl;
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* We're going to create two vertex buffers:
             * (1) A staging buffer in CPU accessible memory to upload the data from the vertex array to, and 
             * (2) Another vertex buffer in device local memory (high performance memory)
             * 
             * Why do we need two vertex buffers?
             * With just one vertex buffer everything may work correctly, but, the memory type that allows us to access 
             * it from the CPU may not be the most optimal memory type for the graphics card itself to read from. The 
             * most optimal memory has the VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT flag and is usually not accessible by the 
             * CPU on dedicated graphics cards
             * 
             * After creating the two buffers, we'll then use a buffer copy command to move the data from the staging 
             * buffer to the actual vertex buffer by recording a copy command on the transfer queue
            */
            void createVertexBuffer (void) {
                VkDeviceSize bufferSize = sizeof (getVertices()[0]) * getVertices().size();
                /* The buffer usage bit is set to VK_BUFFER_USAGE_TRANSFER_SRC_BIT, this means the buffer can be used as 
                 * source in a memory transfer operation
                 *
                 * Memory type properties:
                 * (1) VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                 * This property says that we are able to map the allocated memry so we can write to it from the CPU
                 * 
                 * (2) VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                 * After we memcpy the vertex data to the mapped memory (see memcpy below) and unmap it again using 
                 * vkUnmapMemory, the driver may not immediately copy the data into the buffer memory, for example 
                 * because of caching. It is also possible that writes to the buffer are not visible in the mapped 
                 * memory yet. There are two ways to deal with that problem:
                 * 
                 * (1) Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                 * (2) Or, call vkFlushMappedMemoryRanges after writing to the mapped memory, and call 
                 * vkInvalidateMappedMemoryRanges before reading from the mapped memory
                 * 
                 * We went for the first approach, which ensures that the mapped memory always matches the contents of 
                 * the allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit 
                 * flushing
                */
                createGenericBuffer (bufferSize, 
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                     m_vertexStagingBuffer, 
                                     m_vertexStagingBufferMemory);

                /* It is now time to copy the vertex data to the buffer. This is done by mapping the buffer memory into 
                 * CPU accessible memory with vkMapMemory. This function allows us to access a region of the specified 
                 * memory resource defined by an offset and size. The offset and size here are 0 and bufferInfo.size, 
                 * respectively. The second to last parameter can be used to specify flags, but there aren't any 
                 * available yet in the current API. It must be set to the value 0. The last parameter specifies the 
                 * output for the pointer to the mapped memory
                 * 
                 * vkMapMemory function maps the memory object into application address space
                */
                void* data;
                vkMapMemory (getLogicalDevice(), m_vertexStagingBufferMemory, 0, bufferSize, 0, &data);
                /* You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory
                */
                memcpy (data, getVertices().data(), (size_t) bufferSize);
                /* Unmap the memory object once host access to it is no longer needed by the application
                */
                vkUnmapMemory (getLogicalDevice(), m_vertexStagingBufferMemory);
    
                /* The m_vertexBuffer can now be allocated from a memory type that is device local, which generally means 
                 * that we're not able to use vkMapMemory. However, we can copy data from the stagingBuffer to the 
                 * m_vertexBuffer. We have to indicate that we intend to do that by specifying the transfer source flag 
                 * for the stagingBuffer and the transfer destination flag for the vertexBuffer, along with the vertex 
                 * buffer usage flag
                */
                createGenericBuffer (bufferSize, 
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                     m_vertexBuffer, 
                                     m_vertexBufferMemory);
            }

            /* Creating an index buffer is almost identical to creating the vertex buffer. There are only two notable 
             * differences. The bufferSize is now equal to the number of indices times the size of the index type. The 
             * usage of the m_indexBuffer should be VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of 
             * VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            */
            void createIndexBuffer (void) {
                VkDeviceSize bufferSize = sizeof (getIndices()[0]) * getIndices().size();

                createGenericBuffer (bufferSize, 
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                     m_indexStagingBuffer, 
                                     m_indexStagingBufferMemory);

                void* data;
                vkMapMemory (getLogicalDevice(), m_indexStagingBufferMemory, 0, bufferSize, 0, &data);
                memcpy (data, getIndices().data(), (size_t) bufferSize);
                vkUnmapMemory (getLogicalDevice(), m_indexStagingBufferMemory);
    
                createGenericBuffer (bufferSize, 
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                     m_indexBuffer, 
                                     m_indexBufferMemory);
            }

            /* After setting up the buffers, we can now move the vertex and index data to the device local buffers
            */
            void copyBuffers (void) {
                /* (1) Create command pool and command buffers
                */
                createCommandPool();
                createCommandBuffers();
                /* (2) Record command buffer (index 0)
                */
                VkDeviceSize vertexBufferSize = sizeof (getVertices()[0]) * getVertices().size();
                recordCommandBuffer (getCommandBuffers()[0], 
                                     m_vertexStagingBuffer, 
                                     m_vertexBuffer, 
                                     vertexBufferSize);

                /* (3) Record command buffer (index 1)
                */
                VkDeviceSize indexBufferSize = sizeof (getIndices()[0]) * getIndices().size();
                recordCommandBuffer (getCommandBuffers()[1], 
                                     m_indexStagingBuffer, 
                                     m_indexBuffer, 
                                     indexBufferSize);
                
                /* (4) Submit command buffer to transfer queue
                */
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = static_cast <uint32_t> (getCommandBuffers().size());
                submitInfo.pCommandBuffers = getCommandBuffers().data();
                vkQueueSubmit (getTransferQueue(), 1, &submitInfo, getTransferCompleteFence());

                /* (5) Wait for fence 
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
                LOG_INFO (m_VKVertexBufferLog) << "Waiting for getTransferCompleteFence" << std::endl;
                vkWaitForFences (getLogicalDevice(), 1, &getTransferCompleteFence(), VK_TRUE, UINT64_MAX);
                vkResetFences (getLogicalDevice(), 1, &getTransferCompleteFence());
                LOG_INFO (m_VKVertexBufferLog) << "Reset getTransferCompleteFence" << std::endl;

                /* (6)
                 * The vertex and index data are now being loaded from high performance memory, next we should clean up 
                 * the staging buffer handles
                */
                vkDestroyBuffer (getLogicalDevice(), m_vertexStagingBuffer, nullptr);
                vkDestroyBuffer (getLogicalDevice(), m_indexStagingBuffer, nullptr);
                vkFreeMemory (getLogicalDevice(), m_vertexStagingBufferMemory, nullptr);
                vkFreeMemory (getLogicalDevice(), m_indexStagingBufferMemory, nullptr);

                /* (7) Destroy command pool
                */
                VKTransferCmdBuffer::cleanUp();
                /* All that remains is binding the vertex and index buffer to the graphics command buffer, which is done 
                 * in the recordCommandBuffer function for the graphics queue
                */
            }

            VkBuffer getVertexBuffer (void) {
                return m_vertexBuffer;
            }

            VkBuffer getIndexBuffer (void) {
                return m_indexBuffer;
            }

            void cleanUp (void) {
                /* The buffer should be available for use in rendering commands until the end of the program
                */
                vkDestroyBuffer (getLogicalDevice(), m_vertexBuffer, nullptr);
                vkDestroyBuffer (getLogicalDevice(), m_indexBuffer, nullptr);
                /* Memory that is bound to a buffer object may be freed once the buffer is no longer used, so let's 
                 * free it after the buffer has been destroyed
                */
                vkFreeMemory (getLogicalDevice(), m_vertexBufferMemory, nullptr);
                vkFreeMemory (getLogicalDevice(), m_indexBufferMemory, nullptr);
            }
    };

    Log::Record* VKVertexBuffer::m_VKVertexBufferLog;
}   // namespace Renderer
#endif  // VK_VERTEX_BUFFER_H