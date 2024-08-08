#ifndef VK_HAND_OFF_H
#define VK_HAND_OFF_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKUniform.h"
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Renderer {
    class VKHandOff {
        private:
            struct HandOffInfo {
                struct Meta {
                    uint32_t texId;
                } meta;

                struct Id {
                    uint32_t swapChainImageInfoBase;
                    uint32_t depthImageInfo;
                    uint32_t multiSampleImageInfo;
                    uint32_t uniformBufferInfoBase;
                    uint32_t inFlightFenceInfoBase;
                    uint32_t imageAvailableSemaphoreInfoBase;
                    uint32_t renderDoneSemaphoreInfoBase;
                } id;

                struct Resource {
                    VkSampler textureSampler;
                    VkDescriptorPool descriptorPool;
                    std::vector <VkDescriptorSet> descriptorSets;

                    VkCommandPool commandPool;
                    std::vector <VkCommandBuffer> commandBuffers;
                } resource;
            };
            std::map <uint32_t, HandOffInfo> m_handOffInfoPool;

            static Log::Record* m_VKHandOffLog;
            const uint32_t m_instanceId = g_collectionsId++;

            void deleteHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end()) {
                    m_handOffInfoPool.erase (handOffInfoId);
                    return;
                }

                LOG_ERROR (m_VKHandOffLog) << "Failed to delete hand off info "
                                           << "[" << handOffInfoId << "]"          
                                           << std::endl;
                throw std::runtime_error ("Failed to delete hand off info"); 
            }
            
        public:
            VKHandOff (void) {
                m_VKHandOffLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKHandOff (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyHandOffInfo (uint32_t handOffInfoId,
                                   const std::vector <uint32_t>& infoIds) {
                
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end()) {
                    LOG_ERROR (m_VKHandOffLog) << "Hand off info id already exists "
                                               << "[" << handOffInfoId << "]"
                                               << std::endl;
                    throw std::runtime_error ("Hand off info id already exists");
                }

                HandOffInfo info{};
                info.meta.texId                         = 0;
                info.id.swapChainImageInfoBase          = infoIds[0];
                info.id.depthImageInfo                  = infoIds[1];
                info.id.multiSampleImageInfo            = infoIds[2];
                info.id.uniformBufferInfoBase           = infoIds[3];
                info.id.inFlightFenceInfoBase           = infoIds[4];
                info.id.imageAvailableSemaphoreInfoBase = infoIds[5];
                info.id.renderDoneSemaphoreInfoBase     = infoIds[6];

                m_handOffInfoPool[handOffInfoId] = info;
            }

            HandOffInfo* getHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end())
                    return &m_handOffInfoPool[handOffInfoId];
                
                LOG_ERROR (m_VKHandOffLog) << "Failed to find hand off info "
                                           << "[" << handOffInfoId << "]"
                                           << std::endl;
                throw std::runtime_error ("Failed to find hand off info");
            }

            void dumpHandOffInfoPool (void) {
                LOG_INFO (m_VKHandOffLog) << "Dumping hand off info pool"
                                          << std::endl;

                for (auto const& [key, val]: m_handOffInfoPool) {
                    LOG_INFO (m_VKHandOffLog) << "Hand off info id " 
                                              << "[" << key << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Cycle texture id "
                                              << "[" << val.meta.texId << "]" 
                                              << std::endl;  

                    LOG_INFO (m_VKHandOffLog) << "Swap chain image info id base " 
                                              << "[" << val.id.swapChainImageInfoBase << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Depth image info id " 
                                              << "[" << val.id.depthImageInfo << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Multi sample image info id " 
                                              << "[" << val.id.multiSampleImageInfo << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Uniform buffer info id base "
                                              << "[" << val.id.uniformBufferInfoBase << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "In flight fence info id base "
                                              << "[" << val.id.inFlightFenceInfoBase << "]" 
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Image available semaphore info id base " 
                                              << "[" << val.id.imageAvailableSemaphoreInfoBase << "]" 
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Render done semaphore info id base " 
                                              << "[" << val.id.renderDoneSemaphoreInfoBase << "]" 
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Descriptor sets count " 
                                              << "[" << val.resource.descriptorSets.size() << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Command buffers count " 
                                              << "[" << val.resource.commandBuffers.size() << "]"
                                              << std::endl;                                                                         
                }              
            }

            void cleanUp (uint32_t handOffInfoId) {
                deleteHandOffInfo (handOffInfoId);
            }
    };

    Log::Record* VKHandOff::m_VKHandOffLog;
}   // namespace Renderer
#endif  // VK_HAND_OFF_H