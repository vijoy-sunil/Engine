#ifndef VK_BUFFER_MGR_H
#define VK_BUFFER_MGR_H

#include "../Device/VKPhyDevice.h"

using namespace Collections;

namespace Renderer {
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
            std::map <e_bufferType, std::vector <BufferInfo>> m_bufferInfoPool{};

            static Log::Record* m_VKBufferMgrLog;
            const uint32_t m_instanceId = g_collectionsId++;

            void deleteBufferInfo (BufferInfo* bufferInfo, e_bufferType type) {
                if (m_bufferInfoPool.find (type) != m_bufferInfoPool.end()) {
                    auto& infos = m_bufferInfoPool[type];

                    infos.erase (std::remove (infos.begin(), infos.end(), *bufferInfo), infos.end());
                    m_bufferInfoPool[type] = infos;
                    return;
                }

                LOG_ERROR (m_VKBufferMgrLog) << "Failed to delete buffer info "
                                             << "[" << bufferInfo->meta.id << "]"
                                             << " "
                                             << "[" << Utils::string_bufferType (type) << "]"             
                                             << std::endl;
                throw std::runtime_error ("Failed to delete buffer info");              
            }

        public:
            VKBufferMgr (void) {
                m_VKBufferMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKBufferMgr (void) {
                LOG_CLOSE (m_instanceId);
            }
            
        protected:
            void createBuffer (uint32_t bufferInfoId,
                               e_bufferType type,
                               VkDeviceSize size, 
                               VkBufferUsageFlags usage, 
                               VkMemoryPropertyFlags property, 
                               const std::vector <uint32_t>& queueFamilyIndices) {

                auto deviceInfo = getDeviceInfo();
                for (auto const& info: m_bufferInfoPool[type]) {
                    if (info.meta.id == bufferInfoId) {
                        LOG_ERROR (m_VKBufferMgrLog) << "Buffer info id already exists " 
                                                     << "[" << bufferInfoId << "]"
                                                     << " "
                                                     << "[" << Utils::string_bufferType (type) << "]"
                                                     << std::endl;
                        throw std::runtime_error ("Buffer info id already exists");
                    }
                }

                VkBufferCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.size  = size;
                createInfo.usage = usage;
                /* If the queue families differ, then we'll be using the concurrent mode (buffers can be used across 
                 * multiple queue families without explicit ownership transfers.) Concurrent mode requires you to specify 
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
                 * family. This option offers the best performance.)
                */
                else {
                    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;
                    createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE; 
                }
                /* The flags parameter is used to configure sparse buffer memory, we'll leave it at the default value of 0
                */
                createInfo.flags = 0;

                VkBuffer buffer;
                VkResult result = vkCreateBuffer (deviceInfo->shared.logDevice, &createInfo, VK_NULL_HANDLE, &buffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKBufferMgrLog) << "Failed to create buffer " 
                                                 << "[" << bufferInfoId << "]"
                                                 << " "
                                                 << "[" << Utils::string_bufferType (type) << "]"
                                                 << " "
                                                 << "[" << string_VkResult (result) << "]" 
                                                 << std::endl; 
                    throw std::runtime_error ("Failed to create buffer");
                }

                /* The buffer has been created, but it doesn't actually have any memory assigned to it yet. The first 
                 * step of allocating memory for the buffer is to query its memory requirements
                 *
                 * The VkMemoryRequirements struct has three fields:
                 * (1) size: The size of the required amount of memory in bytes, may differ from bufferInfo.size
                 * (2) alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends 
                 * on bufferInfo.usage and bufferInfo.flags
                 * (3) memoryTypeBits: A bitmask which contains one bit set for every supported memory type for the 
                 * resource. Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties 
                 * structure for the physical device is supported for the resource
                */
                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements (deviceInfo->shared.logDevice, buffer, &memRequirements);
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
                allocInfo.memoryTypeIndex = getMemoryTypeIndex (memRequirements.memoryTypeBits, property);

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
                result = vkAllocateMemory (deviceInfo->shared.logDevice, &allocInfo, VK_NULL_HANDLE, &bufferMemory);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKBufferMgrLog) << "Failed to allocate buffer memory " 
                                                 << "[" << bufferInfoId << "]"
                                                 << " "
                                                 << "[" << Utils::string_bufferType (type) << "]"
                                                 << " "
                                                 << "[" << string_VkResult (result) << "]"
                                                 << std::endl; 
                    throw std::runtime_error ("Failed to allocate buffer memory");
                }

                /* If memory allocation was successful, then we can now associate this memory with the buffer. The fourth 
                 * parameter is the offset within the region of memory that is to be bound to the buffer. If the offset 
                 * is non-zero, then it is required to be divisible by memRequirements.alignment
                */
                vkBindBufferMemory (deviceInfo->shared.logDevice, buffer, bufferMemory, 0);

                BufferInfo info{};
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
                                             << "[" << Utils::string_bufferType (type) << "]"           
                                             << std::endl;
                throw std::runtime_error ("Failed to find buffer info");
            }

            void dumpBufferInfoPool (void) {
                LOG_INFO (m_VKBufferMgrLog) << "Dumping buffer info pool"
                                            << std::endl;

                for (auto const& [key, val]: m_bufferInfoPool) {
                    LOG_INFO (m_VKBufferMgrLog) << "Type " 
                                                << "[" << Utils::string_bufferType (key) << "]"
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
                        auto flags = Utils::splitString (string_VkBufferUsageFlags (info.params.usage), "|");
                        for (auto const& flag: flags)
                            LOG_INFO (m_VKBufferMgrLog) << "[" << flag << "]" << std::endl; 

                        LOG_INFO (m_VKBufferMgrLog) << "Property"
                                                    << std::endl;
                        auto properties = Utils::splitString (string_VkMemoryPropertyFlags (info.params.property), "|");
                        for (auto const& property: properties)
                            LOG_INFO (m_VKBufferMgrLog) << "[" << property << "]" << std::endl; 

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

            void cleanUp (uint32_t bufferInfoId, e_bufferType type) {
                auto bufferInfo = getBufferInfo (bufferInfoId, type);
                auto deviceInfo = getDeviceInfo();

                vkDestroyBuffer  (deviceInfo->shared.logDevice, bufferInfo->resource.buffer,       VK_NULL_HANDLE);
                /* Memory that is bound to a buffer object may be freed once the buffer is no longer used, so let's free 
                 * it after the buffer has been destroyed
                */
                vkFreeMemory     (deviceInfo->shared.logDevice, bufferInfo->resource.bufferMemory, VK_NULL_HANDLE);
                deleteBufferInfo (bufferInfo, type);
            } 
    };

    Log::Record* VKBufferMgr::m_VKBufferMgrLog;
}   // namespace Renderer
#endif  // VK_BUFFER_MGR_H