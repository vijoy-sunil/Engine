#ifndef VK_UNIFORM_BUFFER_H
#define VK_UNIFORM_BUFFER_H

#include "VKBufferMgr.h"

using namespace Collections;

namespace Renderer {
    class VKUniformBuffer: protected virtual VKBufferMgr {
        private:
            static Log::Record* m_VKUniformBufferLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKUniformBuffer (void) {
                m_VKUniformBufferLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKUniformBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:  
            /* Source: https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
            */ 
            VkDeviceSize getDynamicUBOOffsetAlignment (VkDeviceSize minOffsetAlignment, VkDeviceSize size) {
                VkDeviceSize dynamicUBOOffsetAlignment = size;
                if (minOffsetAlignment > 0)
                    dynamicUBOOffsetAlignment = (dynamicUBOOffsetAlignment + minOffsetAlignment - 1) & 
                                                                           ~(minOffsetAlignment - 1);
                return dynamicUBOOffsetAlignment;
            }

            void createUniformBuffer (uint32_t bufferInfoId, 
                                      uint32_t resourceId, 
                                      VkDeviceSize size, 
                                      const void* data) {
                // TO DO
                static_cast <void> (bufferInfoId);
                static_cast <void> (resourceId);
                static_cast <void> (size);
                static_cast <void> (data);
            }

            void createUniformBuffer (uint32_t bufferInfoId, 
                                      uint32_t resourceId, 
                                      VkDeviceSize size) {
                /* Note that, this method doesn't accept a data pointer. This is because we're going to copy new data to 
                 * the uniform buffer every time we call the update function. In addition, it doesn't really make any 
                 * sense to have a staging buffer since it would just add extra overhead in this case and likely degrade 
                 * performance instead of improving it
                */
                auto deviceInfo = getDeviceInfo();
                auto bufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value()
                };

                createBuffer (bufferInfoId, 
                              UNIFORM_BUFFER,
                              size,
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                              bufferShareQueueFamilyIndices);

                auto bufferInfo = getBufferInfo (bufferInfoId, UNIFORM_BUFFER);
                /* We map the buffer right after creation using vkMapMemory to get a pointer to which we can write the 
                 * data later on. The buffer stays mapped to this pointer for the application's whole lifetime. This 
                 * technique is called "persistent mapping" and works on all Vulkan implementations. Not having to map 
                 * the buffer every time we need to update it increases performances, as mapping is not free
                */
                vkMapMemory (deviceInfo->shared.logDevice, 
                             bufferInfo->resource.bufferMemory, 
                             0, 
                             size, 
                             0, 
                             &bufferInfo->meta.bufferMapped);
            }

            void updateUniformBuffer (uint32_t bufferInfoId, 
                                      VkDeviceSize size, 
                                      const void* data) {
                                        
                auto bufferInfo = getBufferInfo (bufferInfoId, UNIFORM_BUFFER);
                memcpy (bufferInfo->meta.bufferMapped, data, static_cast <size_t> (size));
            }
    };

    Log::Record* VKUniformBuffer::m_VKUniformBufferLog;
}   // namespace Renderer
#endif  // VK_UNIFORM_BUFFER_H