#ifndef VK_VERTEX_BUFFER_H
#define VK_VERTEX_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKPipeline.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKVertexBuffer: protected VKPipeline {
        private:
            /* Handle to the vertex buffer
            */
            VkBuffer m_vertexBuffer;
            /* Handle to vertex buffer memory
            */
            VkDeviceMemory m_vertexBufferMemory;
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
                /* First we need to query info about the available types of memory.
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
            /* We need to create a CPU visible buffer and use memcpy to copy the vertex data into it directly, and after 
             * that we'll use a staging buffer to copy the vertex data to high performance memory (so that the GPU can
             * access it)
            */
            void createVertexBuffer (void) {
                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                /* Specify the size of the buffer in bytes
                */
                bufferInfo.size = sizeof (getVertices()[0]) * getVertices().size();
                /* The usage field indicates for which purposes the data in the buffer is going to be used. It is 
                 * possible to specify multiple purposes using a bitwise or. Our use case will be a vertex buffer
                */
                bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                /* Just like the images in the swap chain, buffers can also be owned by a specific queue family or be 
                 * shared between multiple at the same time. The buffer will only be used from the graphics queue, so we 
                 * can stick to exclusive access
                */
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                /* The flags parameter is used to configure sparse buffer memory, which is not relevant right now. We'll 
                 * leave it at the default value of 0
                */
                bufferInfo.flags = 0;

                VkResult result = vkCreateBuffer (getLogicalDevice(), &bufferInfo, nullptr, &m_vertexBuffer);
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
                vkGetBufferMemoryRequirements (getLogicalDevice(), m_vertexBuffer, &memRequirements);

                /* Next, we can allocate the memory by filling in the VkMemoryAllocateInfo structure
                */
                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                /* Memory allocation is now as simple as specifying the size and type, both of which are derived from 
                 * the memory requirements of the vertex buffer and the desired property
                */
                allocInfo.allocationSize = memRequirements.size;
                /* VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                 * This property says that we are able to map the allocated memry so we can write to it from the CPU
                 * 
                 * VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
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
                allocInfo.memoryTypeIndex = findMemoryType (memRequirements.memoryTypeBits, 
                                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                result = vkAllocateMemory (getLogicalDevice(), &allocInfo, nullptr, &m_vertexBufferMemory);
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
                vkBindBufferMemory (getLogicalDevice(), m_vertexBuffer, m_vertexBufferMemory, 0);

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
                vkMapMemory (getLogicalDevice(), m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
                /* You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory
                */
                memcpy (data, getVertices().data(), (size_t) bufferInfo.size);
                /* Unmap the memory object once host access to it is no longer needed by the application
                */
                vkUnmapMemory (getLogicalDevice(), m_vertexBufferMemory);

                /* Note that, flushing memory ranges or using a coherent memory heap means that the driver will be aware 
                 * of our writes to the buffer, but it doesn't mean that they are actually visible on the GPU yet. The 
                 * transfer of data to the GPU is an operation that happens in the background and the specification 
                 * simply tells us that it is guaranteed to be complete as of the next call to vkQueueSubmit
                */

                /* All that remains is binding the vertex buffer to a command buffer, which is done in the 
                 * recordCommandBuffer function
                */
            }

            VkBuffer getVertexBuffer (void) {
                return m_vertexBuffer;
            }

            void cleanUp (void) {
                /* The buffer should be available for use in rendering commands until the end of the program
                */
                vkDestroyBuffer (getLogicalDevice(), m_vertexBuffer, nullptr);
                /* Memory that is bound to a buffer object may be freed once the buffer is no longer used, so let's 
                 * free it after the buffer has been destroyed
                */
                vkFreeMemory (getLogicalDevice(), m_vertexBufferMemory, nullptr);
            }
    };

    Log::Record* VKVertexBuffer::m_VKVertexBufferLog;
}   // namespace Renderer
#endif  // VK_VERTEX_BUFFER_H