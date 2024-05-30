#ifndef VK_VERTEX_BUFFER_H
#define VK_VERTEX_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKGenericBuffer.h"
#include "VKVertexData.h"
#include "VKTransferCmdBuffer.h"
#include "VKSyncObjects.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKVertexBuffer: protected virtual VKGenericBuffer,
                          protected virtual VKVertexData,
                          protected VKTransferCmdBuffer,
                          protected VKSyncObjects {
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

        public:
            VKVertexBuffer (void) {
                m_VKVertexBufferLog = LOG_INIT (m_instanceId, 
                                                static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                                Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                "./Build/Log/");
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
                /* The buffers should be available for use in rendering commands until the end of the program
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