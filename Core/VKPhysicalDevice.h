#ifndef VK_PHYSICAL_DEVICE_H
#define VK_PHYSICAL_DEVICE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../Collections/Log/include/Log.h"
#include <set>

using namespace Collections;

namespace Renderer {
    class VKPhysicalDevice {
        private:
            /* The graphics card that we'll end up selecting will be stored in a VkPhysicalDevice handle. This object 
             * will be implicitly destroyed when the VkInstance is destroyed, so we won't need to do anything new in the 
             * cleanup function
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
            static Log::Record* m_VKPhysicalDeviceLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 7; 

        public:
            VKPhysicalDevice (void) {
                m_physicalDevice = VK_NULL_HANDLE;

                m_VKPhysicalDeviceLog = LOG_INIT (m_instanceId, 
                                                  Log::VERBOSE, 
                                                  Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                  "./Build/Log/");
                LOG_INFO (m_VKPhysicalDeviceLog) << "Constructor called" << std::endl; 
            }

            ~VKPhysicalDevice (void) {
                LOG_INFO (m_VKPhysicalDeviceLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            bool checkDeviceExtensionSupport (void) {
                /* Query all available extensions
                */
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties (m_physicalDevice, 
                                                      nullptr, 
                                                      &extensionCount, 
                                                      nullptr);
                std::vector <VkExtensionProperties> availableExtensions (extensionCount);
                vkEnumerateDeviceExtensionProperties (m_physicalDevice, 
                                                      nullptr, 
                                                      &extensionCount, 
                                                      availableExtensions.data());

                LOG_INFO (m_VKPhysicalDeviceLog) << "Available device extensions" << std::endl;
                for (const auto& extension: availableExtensions)
                    LOG_INFO (m_VKPhysicalDeviceLog) << extension.extensionName 
                                                     << "," 
                                                     << extension.specVersion 
                                                     << std::endl;
                
                LOG_INFO (m_VKPhysicalDeviceLog) << "Required device extensions" << std::endl;
                for (const auto& extension: m_deviceExtensions)
                    LOG_INFO (m_VKPhysicalDeviceLog) << extension << std::endl;

                /* Use a set of strings here to represent the unconfirmed required extensions. That way we can easily 
                 * tick them off while enumerating the sequence of available extensions
                */
                std::set <std::string> requiredExtensions (m_deviceExtensions.begin(), m_deviceExtensions.end());
                for (const auto& extension : availableExtensions)
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
                bool extensionsSupported = checkDeviceExtensionSupport();
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

            void pickPhysicalDevice (void) {
                /* Query all available graphic cards with vulkan support
                */
                uint32_t deviceCount = 0;
                vkEnumeratePhysicalDevices (m_instance, &deviceCount, nullptr);
                if (deviceCount == 0) {
                    LOG_ERROR (m_VKPhysicalDeviceLog) << "Failed to find GPUs with Vulkan support" << std::endl;
                    throw std::runtime_error ("Failed to find GPUs with Vulkan support");
                }
                std::vector <VkPhysicalDevice> devices (deviceCount);
                vkEnumeratePhysicalDevices (m_instance, &deviceCount, devices.data());

                for (const auto& device: devices) {
                    if (checkPhysicalDeviceSupport (device)) {
                        m_physicalDevice = device;
                        break;
                    }
                }
                if (m_physicalDevice == VK_NULL_HANDLE) {
                    LOG_ERROR (m_VKPhysicalDeviceLog) << "GPU doesn't meet required expectations" << std::endl;
                    throw std::runtime_error ("GPU doesn't meet required expectations");
                }
            }
    };
}   // namespace Renderer
#endif  // VK_PHYSICAL_DEVICE_H