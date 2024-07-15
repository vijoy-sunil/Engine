#ifndef VK_LOG_DEVICE_H
#define VK_LOG_DEVICE_H

#include "VKValidation.h"

using namespace Collections;

namespace Renderer {
    class VKLogDevice: protected virtual VKValidation {
        private:
            static Log::Record* m_VKLogDeviceLog;
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKLogDevice (void) {
                m_VKLogDeviceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);              
            }

            ~VKLogDevice (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createLogicalDevice (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
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
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value(),
                    deviceInfo->unique[resourceId].indices.presentFamily.value(),
                    deviceInfo->unique[resourceId].indices.transferFamily.value()
                };
                /* Assign priorities to queues to influence the scheduling of command buffer execution using floating 
                 * point numbers between 0.0 and 1.0. This is required even if there is only a single queue
                */
                float queuePriority = 1.0f;
                /* Populate the structs
                */
                for (auto const& queueFamily: uniqueQueueFamilies) {
                    VkDeviceQueueCreateInfo createInfo{};
                    createInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    createInfo.queueFamilyIndex = queueFamily;
                    createInfo.queueCount       = 1;
                    createInfo.pQueuePriorities = &queuePriority;

                    queueCreateInfos.push_back (createInfo);
                }
                /* The next information to specify is the set of device features that we'll be using. These are the 
                 * features that we can query for with vkGetPhysicalDeviceFeatures
                */
                VkPhysicalDeviceFeatures deviceFeatures{};
                /* Enable anisotropy feature for the texture sampler. Note that, even though it is very unlikely that a 
                 * modern graphics card will not support it, we still check if it is available when picking the physical
                 * device
                */
                deviceFeatures.samplerAnisotropy = VK_TRUE;
                /* Enable sample shading
                */
                deviceFeatures.sampleRateShading = VK_TRUE;
                /* With the previous two structures in place, we can start filling in the main VkDeviceCreateInfo 
                 * structure
                */
                VkDeviceCreateInfo createInfo{};
                createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.queueCreateInfoCount = static_cast <uint32_t> (queueCreateInfos.size());
                createInfo.pQueueCreateInfos    = queueCreateInfos.data();
                createInfo.pEnabledFeatures     = &deviceFeatures;

                /* The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires 
                 * you to specify extensions and validation layers. The difference is that these are device specific this 
                 * time
                 *
                 * Previous implementations of Vulkan made a distinction between instance and device specific validation 
                 * layers, but this is no longer the case. That means that the enabledLayerCount and ppEnabledLayerNames 
                 * fields of VkDeviceCreateInfo are ignored by up-to-date implementations. However, it is still a good 
                 * idea to set them anyway to be compatible with older implementations
                */
                if (isValidationLayersEnabled() && !isValidationLayersSupported()) {
                    LOG_WARNING (m_VKLogDeviceLog) << "Required validation layers not available" << std::endl;
                    createInfo.enabledLayerCount = 0;
                }
                else if (isValidationLayersEnabled()) {
                    createInfo.enabledLayerCount   = static_cast <uint32_t> (getValidationLayers().size());
                    createInfo.ppEnabledLayerNames = getValidationLayers().data();
                }

                /* Setup device extensions
                */
                createInfo.enabledExtensionCount   = static_cast <uint32_t> (deviceInfo->meta.deviceExtensions.size());
                createInfo.ppEnabledExtensionNames = deviceInfo->meta.deviceExtensions.data();

                /* We're now ready to instantiate the logical device
                 * NOTE: Logical devices don't interact directly with instances, which is why it's not included as a 
                 * parameter while creating or destroying it
                */
                VkDevice logDevice;
                VkResult result = vkCreateDevice (deviceInfo->shared.phyDevice, 
                                                  &createInfo, 
                                                  nullptr, 
                                                  &logDevice);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKLogDeviceLog) << "Failed to create logic device " 
                                                 << "[" << resourceId << "]"
                                                 << " "
                                                 << "[" << string_VkResult (result) << "]" 
                                                 << std::endl;
                    throw std::runtime_error ("Failed to create logic device");
                }
                
                VkQueue graphicsQueue, presentQueue, transferQueue;
                /* Retrieve queue handles for each queue family, The parameters are the logical device, queue family, 
                 * queue index and a pointer to the variable to store the queue handle in. Because we're only creating a 
                 * single queue from this family, we'll simply use index 0.
                */
                vkGetDeviceQueue (logDevice, 
                                  deviceInfo->unique[resourceId].indices.graphicsFamily.value(), 0, 
                                  &graphicsQueue);

                vkGetDeviceQueue (logDevice, 
                                  deviceInfo->unique[resourceId].indices.presentFamily.value(), 0, 
                                  &presentQueue);

                vkGetDeviceQueue (logDevice, 
                                  deviceInfo->unique[resourceId].indices.transferFamily.value(), 0, 
                                  &transferQueue);

                deviceInfo->shared.logDevice                 = logDevice;
                deviceInfo->unique[resourceId].graphicsQueue = graphicsQueue;
                deviceInfo->unique[resourceId].presentQueue  = presentQueue;
                deviceInfo->unique[resourceId].transferQueue = transferQueue;

            }

            void cleanUp (void) {
                auto deviceInfo = getDeviceInfo();
                vkDestroyDevice (deviceInfo->shared.logDevice, nullptr);
            }
    };

    Log::Record* VKLogDevice::m_VKLogDeviceLog;
}   // namespace Renderer
#endif  // VK_LOG_DEVICE_H