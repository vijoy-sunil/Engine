#ifndef VK_LOG_DEVICE_H
#define VK_LOG_DEVICE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKQueue.h"
#include "VKPhyDevice.h"
#include "../Collections/Log/include/Log.h"
#include <vector>
#include <set>

using namespace Collections;

namespace Renderer {
    class VKLogDevice: protected VKQueue,
                       protected VKPhyDevice {
        private:
            /* Handle to the logical device
            */
            VkDevice m_logicalDevice;
            /* Handle to the log object
            */
            static Log::Record* m_VKLogDeviceLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 8;

        public:
            VKLogDevice (void) {
                m_VKLogDeviceLog = LOG_INIT (m_instanceId, 
                                             static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE),
                                             Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                             "./Build/Log/");
            }

            ~VKLogDevice (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkDevice getLogicalDevice (void) {
                return m_logicalDevice;
            }

            void createLogicalDevice (void) {
                QueueFamilyIndices indices = checkQueueFamilySupport (getPhysicalDevice());
                /* The creation of a logical device involves specifying a bunch of details in structs again, of which 
                 * the first one will be VkDeviceQueueCreateInfo. This structure describes the number of queues we want 
                 * for a single queue family. We need to have multiple VkDeviceQueueCreateInfo structs to create a queue 
                 * from different families
                */
                std::vector <VkDeviceQueueCreateInfo> queueCreateInfos;
                /* It's very likely that these end up being the same queue family after all, but we will treat them as 
                 * if they were separate queues for a uniform approach
                */
                std::set <uint32_t> uniqueQueueFamilies = {
                    indices.graphicsFamily.value(), 
                    indices.presentFamily.value(),
                    indices.transferFamily.value()
                };
                /* Assign priorities to queues to influence the scheduling of command buffer execution using floating 
                 * point numbers between 0.0 and 1.0. This is required even if there is only a single queue
                */
                float queuePriority = 1.0f;
                /* Populate the structs
                */
                for (uint32_t queueFamily : uniqueQueueFamilies) {
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = queueFamily;
                    queueCreateInfo.queueCount = 1;
                    queueCreateInfo.pQueuePriorities = &queuePriority;

                    queueCreateInfos.push_back(queueCreateInfo);
                }
                /* The next information to specify is the set of device features that we'll be using. These are the 
                 * features that we can query for with vkGetPhysicalDeviceFeatures
                */
                VkPhysicalDeviceFeatures deviceFeatures{};
                /* .
                 * .
                 * . Right now we don't need anything special, so we can simply define it and leave everything to VK_FALSE
                 * .
                 * .
                */

                /* With the previous two structures in place, we can start filling in the main VkDeviceCreateInfo 
                 * structure
                */
                VkDeviceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.pQueueCreateInfos = queueCreateInfos.data();
                createInfo.queueCreateInfoCount = static_cast <uint32_t> (queueCreateInfos.size());
                createInfo.pEnabledFeatures = &deviceFeatures;

                /* The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires 
                 * you to specify extensions and validation layers. The difference is that these are device specific this 
                 * time.
                 *
                 * Previous implementations of Vulkan made a distinction between instance and device specific validation 
                 * layers, but this is no longer the case. That means that the enabledLayerCount and ppEnabledLayerNames 
                 * fields of VkDeviceCreateInfo are ignored by up-to-date implementations. However, it is still a good 
                 * idea to set them anyway to be compatible with older implementations
                */
                if (isValidationLayersEnabled() && !checkValidationLayerSupport()) {
                    LOG_WARNING (m_VKLogDeviceLog) << "Required validation layers not available" << std::endl;
                    createInfo.enabledLayerCount = 0;
                }
                else if (isValidationLayersEnabled()) {
                    createInfo.enabledLayerCount = static_cast <uint32_t> (getValidationLayers().size());
                    createInfo.ppEnabledLayerNames = getValidationLayers().data();
                }

                /* Setup device extensions
                */
                createInfo.enabledExtensionCount = static_cast <uint32_t> (getDeviceExtensions().size());
                createInfo.ppEnabledExtensionNames = getDeviceExtensions().data();

                /* We're now ready to instantiate the logical device
                 * NOTE: Logical devices don't interact directly with instances, which is why it's not included as a 
                 * parameter while creating or destroying it
                */
                VkResult result = vkCreateDevice (getPhysicalDevice(), &createInfo, nullptr, &m_logicalDevice);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKLogDeviceLog) << "Failed to create logic device" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create logic device");
                }

                /* Retrieve queue handles for each queue family, The parameters are the logical device, queue family, 
                 * queue index and a pointer to the variable to store the queue handle in. Because we're only creating a 
                 * single queue from this family, we'll simply use index 0.
                */
                VkQueue graphicsQueue, presentQueue, transferQueue;
                vkGetDeviceQueue (m_logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
                vkGetDeviceQueue (m_logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
                vkGetDeviceQueue (m_logicalDevice, indices.transferFamily.value(), 0, &transferQueue);

                setGraphicsQueue (graphicsQueue);
                setPresentQueue (presentQueue);
                setTransferQueue (transferQueue);
            }

            void cleanUp (void) {
                /* Destroy logical device handle
                */
                vkDestroyDevice (m_logicalDevice, nullptr);
            }
    };

    Log::Record* VKLogDevice::m_VKLogDeviceLog;
}   // namespace Renderer
#endif  // VK_LOG_DEVICE_H