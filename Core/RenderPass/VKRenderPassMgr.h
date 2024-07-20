#ifndef VK_RENDER_PASS_MGR_H
#define VK_RENDER_PASS_MGR_H

#include "../Device/VKDeviceMgr.h"

using namespace Collections;

namespace Renderer {
    class VKRenderPassMgr: protected virtual VKDeviceMgr {
        private:
            struct RenderPassInfo {
                struct Meta {
                    std::vector <VkAttachmentDescription> attachments;
                    std::vector <VkSubpassDescription>    subPasses;
                    std::vector <VkSubpassDependency>     dependencies;
                } meta;

                struct Resource {
                    VkRenderPass renderPass;
                    std::vector <VkFramebuffer> framebuffers;
                } resource;
            };
            std::map <uint32_t, RenderPassInfo> m_renderPassInfoPool{};

            static Log::Record* m_VKRenderPassMgrLog;
            const size_t m_instanceId = g_collectionsId++;
            
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
                m_VKRenderPassMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
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

            void createRenderPass (uint32_t renderPassInfoId) {
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto deviceInfo     = getDeviceInfo();

                VkRenderPassCreateInfo createInfo{};
                createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.attachmentCount = static_cast <uint32_t> (renderPassInfo->meta.attachments.size());
                createInfo.pAttachments    = renderPassInfo->meta.attachments.data();
                createInfo.subpassCount    = static_cast <uint32_t> (renderPassInfo->meta.subPasses.size());
                createInfo.pSubpasses      = renderPassInfo->meta.subPasses.data();
                createInfo.dependencyCount = static_cast <uint32_t> (renderPassInfo->meta.dependencies.size());
                createInfo.pDependencies   = renderPassInfo->meta.dependencies.data();

                VkRenderPass renderPass;
                VkResult result = vkCreateRenderPass (deviceInfo->shared.logDevice, 
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

                    LOG_INFO (m_VKRenderPassMgrLog) << "Attachment count " 
                                                    << "[" << val.meta.attachments.size() << "]"
                                                    << std::endl;

                    LOG_INFO (m_VKRenderPassMgrLog) << "Subpass count " 
                                                    << "[" << val.meta.subPasses.size() << "]"
                                                    << std::endl; 

                    LOG_INFO (m_VKRenderPassMgrLog) << "Dependency count " 
                                                    << "[" << val.meta.dependencies.size() << "]"
                                                    << std::endl;

                    LOG_INFO (m_VKRenderPassMgrLog) << "Framebuffer count " 
                                                    << "[" << val.resource.framebuffers.size() << "]"
                                                    << std::endl;
                }
            }

            void cleanUp (uint32_t renderPassInfoId) {
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto deviceInfo     = getDeviceInfo();

                vkDestroyRenderPass  (deviceInfo->shared.logDevice, renderPassInfo->resource.renderPass, VK_NULL_HANDLE);
                deleteRenderPassInfo (renderPassInfoId);
            }
    };

    Log::Record* VKRenderPassMgr::m_VKRenderPassMgrLog;
}   // namespace Renderer
#endif  // VK_RENDER_PASS_MGR_H