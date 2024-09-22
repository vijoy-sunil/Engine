#ifndef VK_RENDER_PASS_MGR_H
#define VK_RENDER_PASS_MGR_H

#include "../Device/VKDeviceMgr.h"

namespace Core {
    class VKRenderPassMgr: protected virtual VKDeviceMgr {
        private:
            struct RenderPassInfo {
                struct Resource {
                    std::vector <VkAttachmentDescription> attachments;
                    std::vector <VkSubpassDescription>    subPasses;
                    std::vector <VkSubpassDependency>     dependencies;
                    std::vector <VkFramebuffer>           frameBuffers;
                    VkRenderPass renderPass;
                } resource;
            };
            std::unordered_map <uint32_t, RenderPassInfo> m_renderPassInfoPool;

            Log::Record* m_VKRenderPassMgrLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;
            
            void deleteRenderPassInfo (uint32_t renderPassInfoId) {
                if (m_renderPassInfoPool.find (renderPassInfoId) != m_renderPassInfoPool.end()) {
                    m_renderPassInfoPool.erase (renderPassInfoId);
                    return;
                }

                LOG_ERROR (m_VKRenderPassMgrLog) << "Failed to delete render pass info "
                                                 << "[" << renderPassInfoId << "]"          
                                                 << std::endl;
                throw std::runtime_error ("Failed to delete render pass info");              
            }

        public:
            VKRenderPassMgr (void) {
                m_VKRenderPassMgrLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKRenderPassMgr (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyRenderPassInfo (uint32_t renderPassInfoId) {
                if (m_renderPassInfoPool.find (renderPassInfoId) != m_renderPassInfoPool.end()) {
                    LOG_ERROR (m_VKRenderPassMgrLog) << "Render pass info id already exists "
                                                     << "[" << renderPassInfoId << "]"
                                                     << std::endl;
                    throw std::runtime_error ("Render pass info id already exists");
                }

                RenderPassInfo info{};
                m_renderPassInfoPool[renderPassInfoId] = info;
            }

            void createRenderPass (uint32_t deviceInfoId, uint32_t renderPassInfoId) {
                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkRenderPassCreateInfo createInfo;
                createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.pNext           = VK_NULL_HANDLE;
                createInfo.flags           = 0;
                createInfo.attachmentCount = static_cast <uint32_t> (renderPassInfo->resource.attachments.size());
                createInfo.pAttachments    = renderPassInfo->resource.attachments.data();
                createInfo.subpassCount    = static_cast <uint32_t> (renderPassInfo->resource.subPasses.size());
                createInfo.pSubpasses      = renderPassInfo->resource.subPasses.data();
                createInfo.dependencyCount = static_cast <uint32_t> (renderPassInfo->resource.dependencies.size());
                createInfo.pDependencies   = renderPassInfo->resource.dependencies.data();

                VkRenderPass renderPass;
                VkResult result = vkCreateRenderPass (deviceInfo->resource.logDevice, 
                                                      &createInfo, 
                                                      VK_NULL_HANDLE, 
                                                      &renderPass);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRenderPassMgrLog) << "Failed to create render pass "
                                                     << "[" << renderPassInfoId << "]"
                                                     << " "
                                                     << "[" << string_VkResult (result) << "]"
                                                     << std::endl;
                    throw std::runtime_error ("Failed to create render pass");
                }

                renderPassInfo->resource.renderPass = renderPass;
            }

            RenderPassInfo* getRenderPassInfo (uint32_t renderPassInfoId) {
                if (m_renderPassInfoPool.find (renderPassInfoId) != m_renderPassInfoPool.end())
                    return &m_renderPassInfoPool[renderPassInfoId];
                
                LOG_ERROR (m_VKRenderPassMgrLog) << "Failed to find render pass info "
                                                 << "[" << renderPassInfoId << "]"
                                                 << std::endl;
                throw std::runtime_error ("Failed to find render pass info");
            }

            void dumpRenderPassInfoPool (void) {
                LOG_INFO (m_VKRenderPassMgrLog) << "Dumping render pass info pool"
                                                << std::endl;

                for (auto const& [key, val]: m_renderPassInfoPool) {
                    LOG_INFO (m_VKRenderPassMgrLog) << "Render pass info id " 
                                                    << "[" << key << "]"
                                                    << std::endl;

                    LOG_INFO (m_VKRenderPassMgrLog) << "Attachments count " 
                                                    << "[" << val.resource.attachments.size() << "]"
                                                    << std::endl;

                    LOG_INFO (m_VKRenderPassMgrLog) << "Sub passes count " 
                                                    << "[" << val.resource.subPasses.size() << "]"
                                                    << std::endl; 

                    LOG_INFO (m_VKRenderPassMgrLog) << "Dependencies count " 
                                                    << "[" << val.resource.dependencies.size() << "]"
                                                    << std::endl;

                    LOG_INFO (m_VKRenderPassMgrLog) << "Frame buffers count " 
                                                    << "[" << val.resource.frameBuffers.size() << "]"
                                                    << std::endl;
                }
            }

            void cleanUp (uint32_t deviceInfoId, uint32_t renderPassInfoId) {
                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                vkDestroyRenderPass  (deviceInfo->resource.logDevice, 
                                      renderPassInfo->resource.renderPass, 
                                      VK_NULL_HANDLE);
                deleteRenderPassInfo (renderPassInfoId);
            }
    };
}   // namespace Core
#endif  // VK_RENDER_PASS_MGR_H