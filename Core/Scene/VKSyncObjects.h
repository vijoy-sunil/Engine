#ifndef VK_SYNC_OBJECTS_H
#define VK_SYNC_OBJECTS_H

#include "../Device/VKDeviceMgr.h"
#include "../../Utils/LogHelper.h"

using namespace Collections;

namespace Renderer {
    /* A core design philosophy in Vulkan is that synchronization of execution on the GPU is explicit. The order of 
     * operations is up to us to define using various synchronization primitives which tell the driver the order we want 
     * things to run in. This means that many Vulkan API calls which start executing work on the GPU are asynchronous, 
     * the functions will return before the operation has finished and there are a number of events that we need to order
     * explicitly
    */
    class VKSyncObjects: protected virtual VKDeviceMgr {
        private:
            struct SemaphoreInfo {
                struct Meta {
                    uint32_t id;
                } meta;
                
                struct Resource {
                    VkSemaphore semaphore;
                } resource;

                bool operator == (const SemaphoreInfo& other) const {
                    return meta.id == other.meta.id;
                }
            };
            std::map <e_syncType, std::vector <SemaphoreInfo>> m_semaphoreInfoPool{};

            struct FenceInfo {
                struct Meta {
                    uint32_t id;
                } meta;

                struct Resource {
                    VkFence fence;
                } resource;

                bool operator == (const FenceInfo& other) const {
                    return meta.id == other.meta.id;
                }
            };
            std::map <e_syncType, std::vector <FenceInfo>> m_fenceInfoPool{};

            static Log::Record* m_VKSyncObjectsLog;
            const size_t m_instanceId = g_collectionsId++;

            void deleteSemaphoreInfo (SemaphoreInfo* semaphoreInfo, e_syncType type) {
                if (m_semaphoreInfoPool.find (type) != m_semaphoreInfoPool.end()) {
                    auto& infos = m_semaphoreInfoPool[type];

                    infos.erase (std::remove (infos.begin(), infos.end(), *semaphoreInfo), infos.end());
                    m_semaphoreInfoPool[type] = infos;
                    return;
                }

                LOG_ERROR (m_VKSyncObjectsLog) << "Failed to delete semaphore info "
                                               << "[" << semaphoreInfo->meta.id << "]"
                                               << " "
                                               << "[" << Utils::string_syncType (type) << "]"            
                                               << std::endl;
                throw std::runtime_error ("Failed to delete semaphore info");              
            }

            void deleteFenceInfo (FenceInfo* fenceInfo, e_syncType type) {
                if (m_fenceInfoPool.find (type) != m_fenceInfoPool.end()) {
                    auto& infos = m_fenceInfoPool[type];

                    infos.erase (std::remove (infos.begin(), infos.end(), *fenceInfo), infos.end());
                    m_fenceInfoPool[type] = infos;
                    return;
                }

                LOG_ERROR (m_VKSyncObjectsLog) << "Failed to delete fence info "
                                               << "[" << fenceInfo->meta.id << "]"
                                               << " "
                                               << "[" << Utils::string_syncType (type) << "]"            
                                               << std::endl;
                throw std::runtime_error ("Failed to delete fence info");              
            }

        public:
            VKSyncObjects (void) {
                m_VKSyncObjectsLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKSyncObjects (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createSemaphore (uint32_t semaphoreInfoId, e_syncType type) {
                auto deviceInfo = getDeviceInfo();
                for (auto const& info: m_semaphoreInfoPool[type]) {
                    if (info.meta.id == semaphoreInfoId) {
                        LOG_ERROR (m_VKSyncObjectsLog) << "Semaphore info id already exists " 
                                                       << "[" << semaphoreInfoId << "]"
                                                       << " "
                                                       << "[" << Utils::string_syncType (type) << "]"
                                                       << std::endl;
                        throw std::runtime_error ("Semaphore info id already exists");
                    }
                }
                /* A semaphore is used to add order between queue operations. Queue operations refer to the work we 
                 * submit to a queue, either in a command buffer or from within a function. Semaphores are used both to 
                 * order work inside the same queue and between different queues
                 * 
                 * The way we use a semaphore to order queue operations is by providing the same semaphore as a 'signal' 
                 * semaphore in one queue operation and as a 'wait' semaphore in another queue operation. For example, 
                 * lets say we have semaphore S and queue operations A and B that we want to execute in order. What we 
                 * tell Vulkan is that operation A will 'signal' semaphore S when it finishes executing, and operation B 
                 * will 'wait' on semaphore S before it begins executing. When operation A finishes, semaphore S will be 
                 * signaled, while operation B wont start until S is signaled. After operation B begins executing, 
                 * semaphore S is automatically reset back to being unsignaled, allowing it to be used again
                 * 
                 * Note that, the waiting only happens on the GPU. The CPU continues running without blocking
                */
                VkSemaphoreCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                VkSemaphore semaphore;
                VkResult result = vkCreateSemaphore (deviceInfo->shared.logDevice, 
                                                     &createInfo, 
                                                     nullptr, 
                                                     &semaphore);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create semaphore " 
                                                   << "[" << semaphoreInfoId << "]"
                                                   << " "
                                                   << "[" << Utils::string_syncType (type) << "]"
                                                   << " "
                                                   << "[" << string_VkResult (result) << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Failed to create semaphore");
                }

                SemaphoreInfo info{};
                info.meta.id            = semaphoreInfoId;
                info.resource.semaphore = semaphore;
                m_semaphoreInfoPool[type].push_back (info);
            }

            void createFence (uint32_t fenceInfoId, 
                              e_syncType type,
                              VkFenceCreateFlags flags) {

                auto deviceInfo = getDeviceInfo();
                for (auto const& info: m_fenceInfoPool[type]) {
                    if (info.meta.id == fenceInfoId) {
                        LOG_ERROR (m_VKSyncObjectsLog) << "Fence info id already exists " 
                                                       << "[" << fenceInfoId << "]"
                                                       << " "
                                                       << "[" << Utils::string_syncType (type) << "]"
                                                       << std::endl;
                        throw std::runtime_error ("Fence info id already exists");
                    }
                }
                /* A fence has a similar purpose, in that it is used to synchronize execution, but it is for ordering the 
                 * execution on the CPU, otherwise known as the host. Simply put, if the host needs to know when the GPU 
                 * has finished something, we use a fence
                 * 
                 * Whenever we submit work to execute, we can attach a fence to that work. When the work is finished, the 
                 * fence will be signaled. Then we can make the host wait for the fence to be signaled, guaranteeing that 
                 * the work has finished before the host continues
                 * 
                 * Fences must be reset manually to put them back into the unsignaled state. This is because fences are 
                 * used to control the execution of the host, and so the host gets to decide when to reset the fence. 
                 * Contrast this to semaphores which are used to order work on the GPU without the host being involved
                */
                VkFenceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                createInfo.flags = flags;

                VkFence fence;
                VkResult result = vkCreateFence (deviceInfo->shared.logDevice, 
                                                 &createInfo, 
                                                 nullptr, 
                                                 &fence);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSyncObjectsLog) << "Failed to create fence " 
                                                   << "[" << fenceInfoId << "]"
                                                   << " "
                                                   << "[" << Utils::string_syncType (type) << "]"
                                                   << " "
                                                   << "[" << string_VkResult (result) << "]" 
                                                   << std::endl;
                    throw std::runtime_error ("Failed to create fence");
                }

                FenceInfo info{};
                info.meta.id        = fenceInfoId;
                info.resource.fence = fence;
                m_fenceInfoPool[type].push_back (info);
            }

            SemaphoreInfo* getSemaphoreInfo (uint32_t semaphoreInfoId, e_syncType type) {
                if (m_semaphoreInfoPool.find (type) != m_semaphoreInfoPool.end()) {
                    auto& infos = m_semaphoreInfoPool[type];
                    for (auto& info: infos) {
                        if (info.meta.id == semaphoreInfoId) return &info;
                    }
                }

                LOG_ERROR (m_VKSyncObjectsLog) << "Failed to find semaphore info "
                                               << "[" << semaphoreInfoId << "]"
                                               << " "
                                               << "[" << Utils::string_syncType (type) << "]"           
                                               << std::endl;
                throw std::runtime_error ("Failed to find semaphore info");                
            }

            FenceInfo* getFenceInfo (uint32_t fenceInfoId, e_syncType type) {
                if (m_fenceInfoPool.find (type) != m_fenceInfoPool.end()) {
                    auto& infos = m_fenceInfoPool[type];
                    for (auto& info: infos) {
                        if (info.meta.id == fenceInfoId) return &info;
                    }
                }

                LOG_ERROR (m_VKSyncObjectsLog) << "Failed to find fence info "
                                               << "[" << fenceInfoId << "]"
                                               << " "
                                               << "[" << Utils::string_syncType (type) << "]"             
                                               << std::endl;
                throw std::runtime_error ("Failed to find fence info"); 
            }

            void dumpSemaphoreInfoPool (void) {
                LOG_INFO (m_VKSyncObjectsLog) << "Dumping semaphore info pool"
                                              << std::endl;

                for (auto const& [key, val]: m_semaphoreInfoPool) {
                    LOG_INFO (m_VKSyncObjectsLog) << "Type "
                                                  << "[" << Utils::string_syncType (key) << "]"
                                                  << std::endl;
                    
                    for (auto const& info: val) {
                        LOG_INFO (m_VKSyncObjectsLog) << "Id "
                                                      << "[" << info.meta.id << "]"
                                                      << std::endl; 
                    }                                                                                                                                                                                                                                                                                    
                }    
            }

            void dumpFenceInfoPool (void) {
                LOG_INFO (m_VKSyncObjectsLog) << "Dumping fence info pool"
                                              << std::endl;

                for (auto const& [key, val]: m_fenceInfoPool) {
                    LOG_INFO (m_VKSyncObjectsLog) << "Type "
                                                  << "[" << Utils::string_syncType (key) << "]"
                                                  << std::endl;
                    
                    for (auto const& info: val) {
                        LOG_INFO (m_VKSyncObjectsLog) << "Id "
                                                      << "[" << info.meta.id << "]"
                                                      << std::endl; 
                    }                                                                                                                                                                                                                                                                                    
                }  
            }
            
            void cleanUpSemaphore (uint32_t semaphoreInfoId, e_syncType type) {
                auto semaphoreInfo = getSemaphoreInfo (semaphoreInfoId, type);
                auto deviceInfo    = getDeviceInfo();

                vkDestroySemaphore  (deviceInfo->shared.logDevice, 
                                     semaphoreInfo->resource.semaphore, 
                                     nullptr);
                deleteSemaphoreInfo (semaphoreInfo, type);
            }

            void cleanUpFence (uint32_t fenceInfoId, e_syncType type) {
                auto fenceInfo  = getFenceInfo (fenceInfoId, type);
                auto deviceInfo = getDeviceInfo();

                vkDestroyFence  (deviceInfo->shared.logDevice, 
                                 fenceInfo->resource.fence, 
                                 nullptr);
                deleteFenceInfo (fenceInfo, type); 
            }
    };

    Log::Record* VKSyncObjects::m_VKSyncObjectsLog;
}   // namespace Renderer
#endif  // VK_SYNC_OBJECTS_H