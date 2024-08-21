#ifndef VK_STORAGE_BUFFER_H
#define VK_STORAGE_BUFFER_H

#include "VKBufferMgr.h"

using namespace Collections;

namespace Core {
    class VKStorageBuffer: protected virtual VKBufferMgr {
        private:
            static Log::Record* m_VKStorageBufferLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKStorageBuffer (void) {
                m_VKStorageBufferLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKStorageBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:  
            void createStorageBuffer (uint32_t bufferInfoId, 
                                      uint32_t resourceId, 
                                      VkDeviceSize size, 
                                      const void* data) {
                // TO DO
                static_cast <void> (bufferInfoId);
                static_cast <void> (resourceId);
                static_cast <void> (size);
                static_cast <void> (data);
            }

            void createStorageBuffer (uint32_t bufferInfoId, 
                                      uint32_t resourceId, 
                                      VkDeviceSize size) {

                auto deviceInfo = getDeviceInfo();
                auto bufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value()
                };

                createBuffer (bufferInfoId, 
                              STORAGE_BUFFER,
                              size,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                              bufferShareQueueFamilyIndices);

                auto bufferInfo = getBufferInfo (bufferInfoId, STORAGE_BUFFER);
                vkMapMemory (deviceInfo->shared.logDevice, 
                             bufferInfo->resource.bufferMemory, 
                             0, 
                             size, 
                             0, 
                             &bufferInfo->meta.bufferMapped);
            }

            void updateStorageBuffer (uint32_t bufferInfoId, 
                                      VkDeviceSize size, 
                                      const void* data) {
                                        
                auto bufferInfo = getBufferInfo (bufferInfoId, STORAGE_BUFFER);
                memcpy (bufferInfo->meta.bufferMapped, data, static_cast <size_t> (size));
            }
    };

    Log::Record* VKStorageBuffer::m_VKStorageBufferLog;
}   // namespace Core
#endif  // VK_STORAGE_BUFFER_H