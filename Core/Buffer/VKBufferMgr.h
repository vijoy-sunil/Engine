#ifndef VK_BUFFER_MGR_H
#define VK_BUFFER_MGR_H

#include "../Device/VKPhyDevice.h"

namespace Core {
    class VKBufferMgr: protected virtual VKPhyDevice {
        private:
            struct BufferInfo {
                struct Meta {
                    uint32_t id;
                    VkDeviceSize size;
                    void* bufferMapped;
                } meta;

                struct Resource {
                    VkBuffer buffer;
                    VkDeviceMemory bufferMemory;
                } resource;

                struct Parameters {
                    VkBufferUsageFlags usage;
                    VkMemoryPropertyFlags property;
                    VkSharingMode sharingMode;
                } params;

                struct Allocation {
                    VkDeviceSize size;
                    uint32_t memoryTypeBits;
                    uint32_t memoryTypeIndex;
                } allocation;

                bool operator == (const BufferInfo& other) const {
                    return meta.id == other.meta.id;
                }
            };
            std::unordered_map <e_bufferType, std::vector <BufferInfo>> m_bufferInfoPool;

            Log::Record* m_VKBufferMgrLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deleteBufferInfo (BufferInfo* bufferInfo, e_bufferType type) {
                if (m_bufferInfoPool.find (type) != m_bufferInfoPool.end()) {
                    auto& infos = m_bufferInfoPool[type];

                    infos.erase (std::remove (infos.begin(), infos.end(), *bufferInfo), infos.end());
                    return;
                }

                LOG_ERROR (m_VKBufferMgrLog) << "Failed to delete buffer info "
                                             << "[" << bufferInfo->meta.id << "]"
                                             << " "
                                             << "[" << getBufferTypeString (type) << "]"
                                             << std::endl;
                throw std::runtime_error ("Failed to delete buffer info");
            }

        public:
            VKBufferMgr (void) {
                m_VKBufferMgrLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKBufferMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createBuffer (uint32_t deviceInfoId,
                               uint32_t bufferInfoId,
                               e_bufferType type,
                               VkDeviceSize size,
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags property,
                               const std::vector <uint32_t>& queueFamilyIndices) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                for (auto const& info: m_bufferInfoPool[type]) {
                    if (info.meta.id == bufferInfoId) {
                        LOG_ERROR (m_VKBufferMgrLog) << "Buffer info id already exists "
                                                     << "[" << bufferInfoId << "]"
                                                     << " "
                                                     << "[" << getBufferTypeString (type) << "]"
                                                     << std::endl;
                        throw std::runtime_error ("Buffer info id already exists");
                    }
                }

                VkBufferCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                createInfo.size  = size;
                createInfo.usage = usage;
                /* If the queue families differ, then we'll be using the concurrent mode (buffers can be used across
                 * multiple queue families without explicit ownership transfers). Concurrent mode requires you to specify
                 * in advance between which queue families ownership will be shared using the queueFamilyIndexCount and
                 * pQueueFamilyIndices parameters
                */
                if (isQueueFamiliesUnique (queueFamilyIndices)) {
                    createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = static_cast <uint32_t> (queueFamilyIndices.size());
                    createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
                }
                /* If the queue families are the same, then we should stick to exclusive mode (A buffer is owned by one
                 * queue family at a time and ownership must be explicitly transferred before using it in another queue
                 * family. This option offers the best performance)
                */
                else {
                    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;
                    createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE;
                }

                VkBuffer buffer;
                VkResult result = vkCreateBuffer (deviceInfo->resource.logDevice, &createInfo, VK_NULL_HANDLE, &buffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKBufferMgrLog) << "Failed to create buffer "
                                                 << "[" << bufferInfoId << "]"
                                                 << " "
                                                 << "[" << getBufferTypeString (type) << "]"
                                                 << " "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create buffer");
                }

                /* The buffer has been created, but it doesn't actually have any memory assigned to it yet. The first
                 * step of allocating memory for the buffer is to query its memory requirements
                 *
                 * The VkMemoryRequirements struct has three fields:
                 * (1) size: The size of the required amount of memory in bytes, may differ from size specified in create
                 * info struct
                 * (2) alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends
                 * on usage and flags in create info struct
                 * (3) memoryTypeBits: A bitmask which contains one bit set for every supported memory type for the
                 * resource. Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties
                 * structure for the physical device is supported for the resource
                */
                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements (deviceInfo->resource.logDevice, buffer, &memRequirements);
                /* Next, we can allocate the memory by filling in the VkMemoryAllocateInfo structure
                */
                VkMemoryAllocateInfo allocInfo;
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.pNext = VK_NULL_HANDLE;
                /* Memory allocation is now as simple as specifying the size and type, both of which are derived from
                 * the memory requirements of the buffer and the desired property
                */
                allocInfo.allocationSize = memRequirements.size;
                /* Find suitable memory type
                */
                allocInfo.memoryTypeIndex = getMemoryTypeIndex (deviceInfoId, memRequirements.memoryTypeBits, property);

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
                deviceInfo->meta.memoryAllocationCount++;

                VkDeviceMemory bufferMemory;
                result = vkAllocateMemory (deviceInfo->resource.logDevice, &allocInfo, VK_NULL_HANDLE, &bufferMemory);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKBufferMgrLog) << "Failed to allocate buffer memory "
                                                 << "[" << bufferInfoId << "]"
                                                 << " "
                                                 << "[" << getBufferTypeString (type) << "]"
                                                 << " "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Failed to allocate buffer memory");
                }

                /* If memory allocation was successful, then we can now associate this memory with the buffer. The fourth
                 * parameter is the offset within the region of memory that is to be bound to the buffer. If the offset
                 * is non-zero, then it is required to be divisible by memRequirements.alignment
                */
                vkBindBufferMemory (deviceInfo->resource.logDevice, buffer, bufferMemory, 0);

                BufferInfo info;
                info.meta.id                    = bufferInfoId;
                info.meta.size                  = size;
                info.meta.bufferMapped          = VK_NULL_HANDLE;
                info.resource.buffer            = buffer;
                info.resource.bufferMemory      = bufferMemory;
                info.params.usage               = usage;
                info.params.property            = property;
                info.params.sharingMode         = createInfo.sharingMode;
                info.allocation.size            = allocInfo.allocationSize;
                info.allocation.memoryTypeBits  = memRequirements.memoryTypeBits;
                info.allocation.memoryTypeIndex = allocInfo.memoryTypeIndex;

                m_bufferInfoPool[type].push_back (info);
            }

            uint32_t getNextInfoIdFromBufferType (e_bufferType type) {
                uint32_t nextInfoId = 0;
                if (m_bufferInfoPool.find (type) != m_bufferInfoPool.end()) {
                    auto& infos = m_bufferInfoPool[type];
                    for (auto const& info: infos) {
                        if (info.meta.id >= nextInfoId) nextInfoId = info.meta.id + 1;
                    }
                }
                return nextInfoId;
            }

            BufferInfo* getBufferInfo (uint32_t bufferInfoId, e_bufferType type) {
                if (m_bufferInfoPool.find (type) != m_bufferInfoPool.end()) {
                    auto& infos = m_bufferInfoPool[type];
                    for (auto& info: infos) {
                        if (info.meta.id == bufferInfoId) return &info;
                    }
                }

                LOG_ERROR (m_VKBufferMgrLog) << "Failed to find buffer info "
                                             << "[" << bufferInfoId << "]"
                                             << " "
                                             << "[" << getBufferTypeString (type) << "]"
                                             << std::endl;
                throw std::runtime_error ("Failed to find buffer info");
            }

            void dumpBufferInfoPool (void) {
                LOG_INFO (m_VKBufferMgrLog) << "Dumping buffer info pool"
                                            << std::endl;

                for (auto const& [key, val]: m_bufferInfoPool) {
                    LOG_INFO (m_VKBufferMgrLog) << "Type "
                                                << "[" << getBufferTypeString (key) << "]"
                                                << std::endl;

                    for (auto const& info: val) {
                        LOG_INFO (m_VKBufferMgrLog) << "Id "
                                                    << "[" << info.meta.id << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Size "
                                                    << "[" << info.meta.size << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Usage"
                                                    << std::endl;
                        auto flags = getSplitString (string_VkBufferUsageFlags (info.params.usage), "|");
                        for (auto const& flag: flags)
                        LOG_INFO (m_VKBufferMgrLog) << "[" << flag << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Property"
                                                    << std::endl;
                        auto properties = getSplitString (string_VkMemoryPropertyFlags (info.params.property), "|");
                        for (auto const& property: properties)
                        LOG_INFO (m_VKBufferMgrLog) << "[" << property << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Sharing mode "
                                                    << "[" << string_VkSharingMode (info.params.sharingMode) << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Allocation size "
                                                    << "[" << info.allocation.size << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Mempry type bits "
                                                    << "[" << info.allocation.memoryTypeBits << "]"
                                                    << std::endl;

                        LOG_INFO (m_VKBufferMgrLog) << "Mempry type index "
                                                    << "[" << info.allocation.memoryTypeIndex << "]"
                                                    << std::endl;
                    }
                }
            }

            void cleanUp (uint32_t deviceInfoId, uint32_t bufferInfoId, e_bufferType type) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto bufferInfo = getBufferInfo (bufferInfoId, type);

                vkDestroyBuffer  (deviceInfo->resource.logDevice, bufferInfo->resource.buffer,       VK_NULL_HANDLE);
                /* Memory that is bound to a buffer object may be freed once the buffer is no longer used, so let's free
                 * it after the buffer has been destroyed
                */
                vkFreeMemory     (deviceInfo->resource.logDevice, bufferInfo->resource.bufferMemory, VK_NULL_HANDLE);
                deleteBufferInfo (bufferInfo, type);
            }
    };
}   // namespace Core
#endif  // VK_BUFFER_MGR_H