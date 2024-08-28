#ifndef VK_STORAGE_BUFFER_H
#define VK_STORAGE_BUFFER_H

#include "VKBufferMgr.h"

using namespace Collections;

namespace Core {
    class VKStorageBuffer: protected virtual VKBufferMgr {
        private:
            Log::Record* m_VKStorageBufferLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

        public:
            VKStorageBuffer (void) {
                m_VKStorageBufferLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKStorageBuffer (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:  
            void createStorageBuffer (uint32_t bufferInfoId, 
                                      uint32_t deviceInfoId, 
                                      VkDeviceSize size, 
                                      const void* data) {
                // TO DO
                static_cast <void> (bufferInfoId);
                static_cast <void> (deviceInfoId);
                static_cast <void> (size);
                static_cast <void> (data);
            }

            void createStorageBuffer (uint32_t bufferInfoId, 
                                      uint32_t deviceInfoId, 
                                      VkDeviceSize size) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto bufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->meta.graphicsFamilyIndex.value()
                };

                createBuffer (bufferInfoId,
                              deviceInfoId, 
                              STORAGE_BUFFER,
                              size,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                              bufferShareQueueFamilyIndices);

                auto bufferInfo = getBufferInfo (bufferInfoId, STORAGE_BUFFER);
                vkMapMemory (deviceInfo->resource.logDevice, 
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
}   // namespace Core
#endif  // VK_STORAGE_BUFFER_H