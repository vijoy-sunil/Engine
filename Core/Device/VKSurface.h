#ifndef VK_SURFACE_H
#define VK_SURFACE_H

#include "VKDeviceMgr.h"

namespace Core {
    class VKSurface: protected virtual VKDeviceMgr {
        private:
            Log::Record* m_VKSurfaceLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKSurface (void) {
                m_VKSurfaceLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKSurface (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createSurface (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);

                VkSurfaceKHR surface;
                VkResult result = glfwCreateWindowSurface (deviceInfo->resource.instance,
                                                           deviceInfo->resource.window,
                                                           VK_NULL_HANDLE,
                                                           &surface);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSurfaceLog) << "Failed to create surface "
                                               << "[" << deviceInfoId << "]"
                                               << " "
                                               << "[" << string_VkResult (result) << "]"
                                               << std::endl;
                    throw std::runtime_error ("Failed to create surface");
                }

                deviceInfo->resource.surface = surface;
            }

            void cleanUp (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                vkDestroySurfaceKHR (deviceInfo->resource.instance,
                                     deviceInfo->resource.surface,
                                     VK_NULL_HANDLE);
            }
    };
}   // namespace Core
#endif  // VK_SURFACE_H