#ifndef VK_SURFACE_H
#define VK_SURFACE_H

#include "VKDeviceMgr.h"

using namespace Collections;

namespace Renderer {
    class VKSurface: protected virtual VKDeviceMgr {
        private:
            static Log::Record* m_VKSurfaceLog;
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKSurface (void) {
                m_VKSurfaceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir); 
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKSurface (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createSurface (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();

                VkSurfaceKHR surface;
                VkResult result = glfwCreateWindowSurface (deviceInfo->shared.instance, 
                                                           deviceInfo->unique[resourceId].window, 
                                                           nullptr, 
                                                           &surface);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSurfaceLog) << "Failed to create surface "
                                               << "[" << resourceId << "]"
                                               << " " 
                                               << "[" << string_VkResult (result) << "]" 
                                               << std::endl;
                    throw std::runtime_error ("Failed to create surface");
                }
                
                deviceInfo->unique[resourceId].surface = surface;
            }

            void cleanUp (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                vkDestroySurfaceKHR (deviceInfo->shared.instance, 
                                     deviceInfo->unique[resourceId].surface, 
                                     nullptr);               
            }
    };
    
    Log::Record* VKSurface::m_VKSurfaceLog;
}   // namespace Renderer
#endif  // VK_SURFACE_H