#ifndef VK_SCENE_MGR_H
#define VK_SCENE_MGR_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKUniform.h"
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Renderer {
    class VKSceneMgr {
        private:
            struct SceneInfo {
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
            std::map <uint32_t, SceneInfo> m_sceneInfoPool;

            static Log::Record* m_VKSceneMgrLog;
            const uint32_t m_instanceId = g_collectionsId++;

            void deleteSceneInfo (uint32_t sceneInfoId) {
                if (m_sceneInfoPool.find (sceneInfoId) != m_sceneInfoPool.end()) {
                    m_sceneInfoPool.erase (sceneInfoId);
                    return;
                }

                LOG_ERROR (m_VKSceneMgrLog) << "Failed to delete scene info "
                                            << "[" << sceneInfoId << "]"          
                                            << std::endl;
                throw std::runtime_error ("Failed to delete scene info"); 
            }
            
        public:
            VKSceneMgr (void) {
                m_VKSceneMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKSceneMgr (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readySceneInfo (uint32_t sceneInfoId, const std::vector <uint32_t>& infoIds) {
                if (m_sceneInfoPool.find (sceneInfoId) != m_sceneInfoPool.end()) {
                    LOG_ERROR (m_VKSceneMgrLog) << "Scene info id already exists "
                                                << "[" << sceneInfoId << "]"
                                                << std::endl;
                    throw std::runtime_error ("Scene info id already exists");
                }

                SceneInfo info{};
                info.meta.texId                         = 0;
                info.id.swapChainImageInfoBase          = infoIds[0];
                info.id.depthImageInfo                  = infoIds[1];
                info.id.multiSampleImageInfo            = infoIds[2];
                info.id.uniformBufferInfoBase           = infoIds[3];
                info.id.inFlightFenceInfoBase           = infoIds[4];
                info.id.imageAvailableSemaphoreInfoBase = infoIds[5];
                info.id.renderDoneSemaphoreInfoBase     = infoIds[6];

                m_sceneInfoPool[sceneInfoId] = info;
            }

            SceneInfo* getSceneInfo (uint32_t sceneInfoId) {
                if (m_sceneInfoPool.find (sceneInfoId) != m_sceneInfoPool.end())
                    return &m_sceneInfoPool[sceneInfoId];
                
                LOG_ERROR (m_VKSceneMgrLog) << "Failed to find scene info "
                                            << "[" << sceneInfoId << "]"
                                            << std::endl;
                throw std::runtime_error ("Failed to find scene info");
            }

            void dumpSceneInfoPool (void) {
                LOG_INFO (m_VKSceneMgrLog) << "Dumping scene info pool"
                                           << std::endl;

                for (auto const& [key, val]: m_sceneInfoPool) {
                    LOG_INFO (m_VKSceneMgrLog) << "Scene info id " 
                                               << "[" << key << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Cycle texture id "
                                               << "[" << val.meta.texId << "]" 
                                               << std::endl;  

                    LOG_INFO (m_VKSceneMgrLog) << "Swap chain image info id base " 
                                               << "[" << val.id.swapChainImageInfoBase << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Depth image info id " 
                                               << "[" << val.id.depthImageInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Multi sample image info id " 
                                               << "[" << val.id.multiSampleImageInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Uniform buffer info id base "
                                               << "[" << val.id.uniformBufferInfoBase << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "In flight fence info id base "
                                               << "[" << val.id.inFlightFenceInfoBase << "]" 
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Image available semaphore info id base " 
                                               << "[" << val.id.imageAvailableSemaphoreInfoBase << "]" 
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Render done semaphore info id base " 
                                               << "[" << val.id.renderDoneSemaphoreInfoBase << "]" 
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Descriptor sets count " 
                                               << "[" << val.resource.descriptorSets.size() << "]"
                                               << std::endl;

                    LOG_INFO (m_VKSceneMgrLog) << "Command buffers count " 
                                               << "[" << val.resource.commandBuffers.size() << "]"
                                               << std::endl;                                                                         
                }              
            }

            void cleanUp (uint32_t sceneInfoId) {
                deleteSceneInfo (sceneInfoId);
            }
    };

    Log::Record* VKSceneMgr::m_VKSceneMgrLog;
}   // namespace Renderer
#endif  // VK_SCENE_MGR_H