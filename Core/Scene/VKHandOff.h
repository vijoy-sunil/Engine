#ifndef VK_HAND_OFF_H
#define VK_HAND_OFF_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKUniform.h"
#include "VKTransform.h"
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Renderer {
    class VKHandOff {
        private:
            /* Data to be handed off between sequences are packed into this struct and saved to the pool
            */
            struct HandOffInfo {
                struct Meta {
                    TransformInfo transformInfo;
                    FragShaderVarsPC fragShaderVars;
                } meta;

                struct Id {
                    std::vector <uint32_t> inFlightFenceInfos;
                    std::vector <uint32_t> imageAvailableSemaphoreInfos;
                    std::vector <uint32_t> renderDoneSemaphoreInfos;
                } id;

                struct Resource {
                    VkCommandPool commandPool;
                    std::vector <VkCommandBuffer> commandBuffers;
                } resource;
            };
            std::map <uint32_t, HandOffInfo> m_handOffInfoPool{};

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
            void readyHandOffInfo (uint32_t handOffInfoId) {
                if (m_handOffInfoPool.find (handOffInfoId) != m_handOffInfoPool.end()) {
                    LOG_ERROR (m_VKHandOffLog) << "Hand off info id already exists "
                                               << "[" << handOffInfoId << "]"
                                               << std::endl;
                    throw std::runtime_error ("Hand off info id already exists");
                }

                HandOffInfo info{};
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

                    LOG_INFO (m_VKHandOffLog) << "Model transform info" 
                                              << std::endl;  
                    LOG_INFO (m_VKHandOffLog) << "Translate "
                                              << "[" << val.meta.transformInfo.model.translate.x << ", "
                                                     << val.meta.transformInfo.model.translate.y << ", "
                                                     << val.meta.transformInfo.model.translate.z
                                              << "]"  
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Rotate axis "
                                              << "[" << val.meta.transformInfo.model.rotateAxis.x << ", "
                                                     << val.meta.transformInfo.model.rotateAxis.y << ", "
                                                     << val.meta.transformInfo.model.rotateAxis.z
                                              << "]"  
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Scale "
                                              << "[" << val.meta.transformInfo.model.scale.x << ", "
                                                     << val.meta.transformInfo.model.scale.y << ", "
                                                     << val.meta.transformInfo.model.scale.z
                                              << "]"  
                                              << std::endl;                                                   

                    LOG_INFO (m_VKHandOffLog) << "Rotate angle degrees "
                                              << "[" << val.meta.transformInfo.model.rotateAngleDeg << "]" 
                                              << std::endl; 

                    LOG_INFO (m_VKHandOffLog) << "Camera transform info" 
                                              << std::endl;   
                    LOG_INFO (m_VKHandOffLog) << "Position "
                                              << "[" << val.meta.transformInfo.camera.position.x << ", "
                                                     << val.meta.transformInfo.camera.position.y << ", "
                                                     << val.meta.transformInfo.camera.position.z
                                              << "]"  
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Center "
                                              << "[" << val.meta.transformInfo.camera.center.x << ", "
                                                     << val.meta.transformInfo.camera.center.y << ", "
                                                     << val.meta.transformInfo.camera.center.z
                                              << "]"  
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Up vector "
                                              << "[" << val.meta.transformInfo.camera.upVector.x << ", "
                                                     << val.meta.transformInfo.camera.upVector.y << ", "
                                                     << val.meta.transformInfo.camera.upVector.z
                                              << "]"  
                                              << std::endl;  

                    LOG_INFO (m_VKHandOffLog) << "FOV degrees "
                                              << "[" << val.meta.transformInfo.camera.fovDeg << "]" 
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Near plane "
                                              << "[" << val.meta.transformInfo.camera.nearPlane << "]" 
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Far plane "
                                              << "[" << val.meta.transformInfo.camera.farPlane << "]" 
                                              << std::endl;  

                    LOG_INFO (m_VKHandOffLog) << "Fragment shader push constant texture id "
                                              << "[" << val.meta.fragShaderVars.texId << "]" 
                                              << std::endl;  

                    LOG_INFO (m_VKHandOffLog) << "In flight fence info ids" 
                                              << std::endl;
                    for (auto const& infoId: val.id.inFlightFenceInfos) 
                    LOG_INFO (m_VKHandOffLog) << "[" << infoId << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Image available semaphore info ids" 
                                              << std::endl;
                    for (auto const& infoId: val.id.imageAvailableSemaphoreInfos) 
                    LOG_INFO (m_VKHandOffLog) << "[" << infoId << "]"
                                              << std::endl;

                    LOG_INFO (m_VKHandOffLog) << "Render done semaphore info ids" 
                                              << std::endl;
                    for (auto const& infoId: val.id.renderDoneSemaphoreInfos) 
                    LOG_INFO (m_VKHandOffLog) << "[" << infoId << "]"
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