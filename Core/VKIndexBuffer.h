#ifndef VK_INDEX_BUFFER_H
#define VK_INDEX_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKGenericBuffer.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKIndexBuffer: protected virtual VKGenericBuffer {
        private:
            /* Handle to the index buffer. An index buffer is essentially an array of pointers into the vertex buffer. 
             * It allows you to reorder the vertex data, reuse existing data for multiple vertices and thus saving memory 
             * when loading complex models
            */
            VkBuffer m_indexBuffer;
            /* Handle to index buffer memory
            */
            VkDeviceMemory m_indexBufferMemory;
            /* Handle to staging buffer and staging memory
            */
            VkBuffer m_indexStagingBuffer;
            VkDeviceMemory m_indexStagingBufferMemory;
            /* Contents of index buffer. Note that it is possible to use either uint16_t or uint32_t for your index 
             * buffer depending on the number of entries in vertices, you also have to specify the correct type when 
             * binding the index buffer
            */
            const std::vector <uint32_t> m_indices = {
                0, 1, 2, 2, 3, 0
            };
            /* Handle to the log object
            */
            static Log::Record* m_VKIndexBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++; 

        public:
            VKIndexBuffer (void) {
                m_VKIndexBufferLog = LOG_INIT (m_instanceId, 
                                               static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                               Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                               "./Build/Log/");
            }

            ~VKIndexBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }
        
        protected:
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

            VkBuffer getIndexBuffer (void) {
                return m_indexBuffer;
            }

            VkBuffer getStagingBuffer (void) {
                return m_indexStagingBuffer;
            }

            std::vector <uint32_t> getIndices (void) {
                return m_indices;
            }

            void cleanUpStaging (void) {
                vkDestroyBuffer (getLogicalDevice(), m_indexStagingBuffer, nullptr);
                vkFreeMemory (getLogicalDevice(), m_indexStagingBufferMemory, nullptr);
            }

            void cleanUp (void) {
                /* The buffers should be available for use in rendering commands until the end of the program
                */
                vkDestroyBuffer (getLogicalDevice(), m_indexBuffer, nullptr);
                /* Memory that is bound to a buffer object may be freed once the buffer is no longer used, so let's 
                 * free it after the buffer has been destroyed
                */
                vkFreeMemory (getLogicalDevice(), m_indexBufferMemory, nullptr);
            }
    };

    Log::Record* VKIndexBuffer::m_VKIndexBufferLog;
}   // namespace Renderer
#endif  // VK_INDEX_BUFFER_H