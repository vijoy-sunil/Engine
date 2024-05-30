#ifndef VK_PHY_DEVICE_HELPER_H
#define VK_PHY_DEVICE_HELPER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSwapChain.h"
#include "../Collections/Log/include/Log.h"
#include <vector>
#include <set>

using namespace Collections;

namespace Renderer {
    class VKPhyDeviceHelper: protected virtual VKSwapChain {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKPhyDeviceHelperLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 7;
            
            bool checkDeviceExtensionSupport (VkPhysicalDevice physicalDevice) {
                /* Query all available extensions
                */
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties (physicalDevice, 
                                                      nullptr, 
                                                      &extensionCount, 
                                                      nullptr);
                std::vector <VkExtensionProperties> availableExtensions (extensionCount);
                vkEnumerateDeviceExtensionProperties (physicalDevice, 
                                                      nullptr, 
                                                      &extensionCount, 
                                                      availableExtensions.data());

                LOG_INFO (m_VKPhyDeviceHelperLog) << "Available device extensions" << std::endl;
                for (const auto& extension: availableExtensions)
                    LOG_INFO (m_VKPhyDeviceHelperLog) << extension.extensionName 
                                                      << "," 
                                                      << extension.specVersion 
                                                      << std::endl;
                
                LOG_INFO (m_VKPhyDeviceHelperLog) << "Required device extensions" << std::endl;

                const std::vector <const char*> deviceExtensions = getDeviceExtensions();
                for (const auto& extension: deviceExtensions)
                    LOG_INFO (m_VKPhyDeviceHelperLog) << extension << std::endl;

                /* Use a set of strings here to represent the unconfirmed required extensions. That way we can easily 
                 * tick them off while enumerating the sequence of available extensions
                */
                std::set <std::string> requiredExtensions (deviceExtensions.begin(), deviceExtensions.end());
                for (const auto& extension: availableExtensions)
                    requiredExtensions.erase (extension.extensionName);

                return requiredExtensions.empty();
            }

            bool checkPhysicalDeviceSupport (VkPhysicalDevice physicalDevice) {
                /* list of gpu devices have already been queried and is passed into this function one by one, which is 
                 * then checked for support
                */
                QueueFamilyIndices indices = checkQueueFamilySupport (physicalDevice);
                /* check device extension support
                */
                bool extensionsSupported = checkDeviceExtensionSupport (physicalDevice);
                /* It should be noted that the availability of a presentation queue, implies that the swap chain 
                 * extension must be supported. However, it's still good to be explicit about things, and the extension 
                 * does have to be explicitly enabled
                */
                bool swapChainAdequate = false;
                if (extensionsSupported) {
                    SwapChainSupportDetails swapChainSupport = checkSwapChainSupport (physicalDevice);
                    /* Swap chain support is sufficient for now if there is at least one supported image format and one 
                     * supported presentation mode given the window surface we have
                    */
                    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
                }
                return indices.isComplete() && extensionsSupported && swapChainAdequate;
            }

        public:
            VKPhyDeviceHelper (void) {
                m_VKPhyDeviceHelperLog = LOG_INIT (m_instanceId, 
                                                   static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & 
                                                                              (Log::WARNING | Log::ERROR)),
                                                   Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                   "./Build/Log/"); 
            }

            ~VKPhyDeviceHelper (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void pickPhysicalDevice (void) {
                /* Query all available graphic cards with vulkan support
                */
                uint32_t deviceCount = 0;
                vkEnumeratePhysicalDevices (getInstance(), &deviceCount, nullptr);
                if (deviceCount == 0) {
                    LOG_ERROR (m_VKPhyDeviceHelperLog) << "Failed to find GPUs with Vulkan support" << std::endl;
                    throw std::runtime_error ("Failed to find GPUs with Vulkan support");
                }
                std::vector <VkPhysicalDevice> devices (deviceCount);
                vkEnumeratePhysicalDevices (getInstance(), &deviceCount, devices.data());

                for (const auto& device: devices) {
                    if (checkPhysicalDeviceSupport (device)) {
                        setPhysicalDevice (device);
                        break;
                    }
                }
                if (getPhysicalDevice() == VK_NULL_HANDLE) {
                    LOG_ERROR (m_VKPhyDeviceHelperLog) << "GPU doesn't meet required expectations" << std::endl;
                    throw std::runtime_error ("GPU doesn't meet required expectations");
                }
            }
    };

    Log::Record* VKPhyDeviceHelper::m_VKPhyDeviceHelperLog;
}   // namespace Renderer
#endif  // VK_PHY_DEVICE_HELPER_H