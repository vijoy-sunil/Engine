#ifndef VK_VERTEX_BUFFER_H
#define VK_VERTEX_BUFFER_H

#include "VKBufferMgr.h"

using namespace Collections;

namespace Core {
    class VKVertexBuffer: protected virtual VKBufferMgr {
        private:
            static Log::Record* m_VKVertexBufferLog;
            const uint32_t m_instanceId = g_collectionsId++; 

        public:
            VKVertexBuffer (void) {
                m_VKVertexBufferLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKVertexBuffer (void) {
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
            void createVertexBuffer (uint32_t bufferInfoId, 
                                     uint32_t resourceId, 
                                     VkDeviceSize size, 
                                     const void* data) {

                auto deviceInfo = getDeviceInfo();
                /* Images/buffers can be owned by a specific queue family or be shared between multiple at the same time. 
                 * The vector holds the queue family indices that will share/own this buffer
                */
                auto stagingBufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.transferFamily.value()
                };
                /* The buffer usage bit is set to VK_BUFFER_USAGE_TRANSFER_SRC_BIT, this means the buffer can be used as 
                 * source in a memory transfer operation
                 *
                 * Memory type properties:
                 * (1) VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                 * This property says that we are able to map the allocated memory so we can write to it from the CPU
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
                createBuffer (bufferInfoId, 
                              STAGING_BUFFER,
                              size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                              stagingBufferShareQueueFamilyIndices);
                /* It is now time to copy the vertex data to the buffer. This is done by mapping the buffer memory into 
                 * CPU accessible memory with vkMapMemory. This function allows us to access a region of the specified 
                 * memory resource defined by an offset and size
                */
                auto bufferInfo = getBufferInfo (bufferInfoId, STAGING_BUFFER);
                vkMapMemory (deviceInfo->shared.logDevice, 
                             bufferInfo->resource.bufferMemory, 
                             0, 
                             size, 
                             0, 
                             &bufferInfo->meta.bufferMapped);
                /* You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory
                */
                memcpy (bufferInfo->meta.bufferMapped, data, static_cast <size_t> (size));
                /* Unmap the memory object once host access to it is no longer needed by the application
                */
                vkUnmapMemory (deviceInfo->shared.logDevice, bufferInfo->resource.bufferMemory);

                auto bufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value(), 
                    deviceInfo->unique[resourceId].indices.transferFamily.value()
                };
                /* The vertex buffer can now be allocated from a memory type that is device local, which generally means 
                 * that we're not able to use vkMapMemory. However, we can copy data from the stagingBuffer to the 
                 * vertex buffer. We have to indicate that we intend to do that by specifying the transfer source flag 
                 * for the staging buffer and the transfer destination flag for the vertexBuffer, along with the vertex 
                 * buffer usage flag
                */
                createBuffer (bufferInfoId, 
                              VERTEX_BUFFER,
                              size, 
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                              bufferShareQueueFamilyIndices);
            }
    };

    Log::Record* VKVertexBuffer::m_VKVertexBufferLog;
}   // namespace Core
#endif  // VK_VERTEX_BUFFER_H