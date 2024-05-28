#ifndef VK_PHY_DEVICE_H
#define VK_PHY_DEVICE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKConstants.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKPhyDevice {
        private:
            /* The graphics card that we'll end up selecting will be stored in a VkPhysicalDevice handle. This object 
             * will be implicitly destroyed when the VkInstance is destroyed, so we won't need to do anything new in 
             * the cleanup function
            */
            VkPhysicalDevice m_physicalDevice;
            /* List of device extensions
            */
            const std::vector <const char*> m_deviceExtensions = {
#if __APPLE__
                "VK_KHR_portability_subset",
#endif
                /* Extensions for enabling swap chain, since image presentation is heavily tied into the window system 
                 * and the surfaces associated with windows, it is not actually part of the Vulkan core
                */
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };
            /* Handle to the log object
            */
            static Log::Record* m_VKPhyDeviceLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 18;

        public:
            VKPhyDevice (void) {
                m_VKPhyDeviceLog = LOG_INIT (m_instanceId, 
                                             static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE),
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
            }

            ~VKPhyDevice (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkPhysicalDevice getPhysicalDevice (void) {
                return m_physicalDevice;
            }

            void setPhysicalDevice (VkPhysicalDevice physicalDevice) {
                m_physicalDevice = physicalDevice;
            }

            const std::vector <const char*>& getDeviceExtensions (void) {
                return m_deviceExtensions;
            }         
    };

    Log::Record* VKPhyDevice::m_VKPhyDeviceLog;
}   // namespace Renderer
#endif  // VK_PHY_DEVICE_H