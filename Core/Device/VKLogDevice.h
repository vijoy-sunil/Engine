#ifndef VK_LOG_DEVICE_H
#define VK_LOG_DEVICE_H

#include "VKValidation.h"
#include "VKPhyDevice.h"

using namespace Collections;

namespace Core {
    class VKLogDevice: protected virtual VKValidation,
                       protected virtual VKPhyDevice {
        private:
            Log::Record* m_VKLogDeviceLog;
            const uint32_t m_instanceId = g_collectionsId++;

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
            void createLogDevice (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
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
                    deviceInfo->meta.graphicsFamilyIndex.value(),
                    deviceInfo->meta.presentFamilyIndex. value(),
                    deviceInfo->meta.transferFamilyIndex.value()
                };
                /* Assign priorities to queues to influence the scheduling of command buffer execution using floating 
                 * point numbers between 0.0 and 1.0. This is required even if there is only a single queue
                */
                float queuePriority = 1.0f;
                /* Populate the structs
                */
                for (auto const& queueFamily: uniqueQueueFamilies) {
                    VkDeviceQueueCreateInfo createInfo;
                    createInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    createInfo.pNext            = VK_NULL_HANDLE;
                    createInfo.flags            = 0;
                    createInfo.queueFamilyIndex = queueFamily;
                    createInfo.queueCount       = 1;
                    createInfo.pQueuePriorities = &queuePriority;

                    queueCreateInfos.push_back (createInfo);
                }
 
                /* With the previous two structures in place, we can start filling in the main VkDeviceCreateInfo 
                 * structure
                */
                VkDeviceCreateInfo createInfo;
                createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.queueCreateInfoCount = static_cast <uint32_t> (queueCreateInfos.size());
                createInfo.pQueueCreateInfos    = queueCreateInfos.data();
                createInfo.flags                = 0;
                /* Note that, if we are using pNext, then pEnabledFeatures will have to be null as required by the spec
                */
                createInfo.pEnabledFeatures     = VK_NULL_HANDLE;
                /* The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires 
                 * you to specify extensions and validation layers. The difference is that these are device specific this 
                 * time
                 *
                 * Previous implementations of Vulkan made a distinction between instance and device specific validation 
                 * layers, but this is no longer the case. That means that the enabledLayerCount and ppEnabledLayerNames 
                 * fields of VkDeviceCreateInfo are ignored by up-to-date implementations. However, it is still a good 
                 * idea to set them anyway to be compatible with older implementations
                */
                if (isValidationLayersEnabled() && !isValidationLayersSupportedAlias()) {
                    LOG_WARNING (m_VKLogDeviceLog) << "Required validation layers not available" 
                                                   << std::endl;
                    createInfo.enabledLayerCount = 0;
                }
                else if (isValidationLayersEnabled()) {
                    createInfo.enabledLayerCount   = static_cast <uint32_t> (getValidationLayers().size());
                    createInfo.ppEnabledLayerNames = getValidationLayers().data();
                }

                /* Setup device extensions
                */
                createInfo.enabledExtensionCount   = static_cast <uint32_t> (getDeviceExtensions().size());
                createInfo.ppEnabledExtensionNames = getDeviceExtensions().data();

                /* The next information to specify is the set of device features that we'll be using
                 * (1) Core 1.0 features
                 * These are the set of features that were available from the initial 1.0 release of Vulkan. The list of 
                 * features can be found in VkPhysicalDeviceFeatures, we can selectively set the features that we require
                 * by setting the boolean in an empty VkPhysicalDeviceFeatures struct, or use vkGetPhysicalDeviceFeatures
                 * function to enable all supported features and pass it to VkDeviceCreateInfo
                 * 
                 * (2) Future core version features
                 * With Vulkan 1.1+ some new features were added to the core version of Vulkan. To keep the size of 
                 * VkPhysicalDeviceFeatures backward compatible, new structs were created to hold the grouping of 
                 * features, for example VkPhysicalDeviceVulkan11Features, VkPhysicalDeviceVulkan12Features
                 * 
                 * (3) Extension features
                 * Sometimes extensions contain features in order to enable certain aspects of the extension. These are 
                 * easily found as they are all labeled as VkPhysicalDevice[ExtensionName]Features
                 * 
                 * Note that, for the core 1.0 features, this is as simple as setting VkDeviceCreateInfo::pEnabledFeatures
                 * with the features desired to be turned on (only if we are not using pNext), and for all features, 
                 * including the Core 1.0 Features, use VkPhysicalDeviceFeatures2 to pass into VkDeviceCreateInfo.pNext
                */
                VkPhysicalDeviceFeatures requiredFeatures{};
                /* Enable only the following device features
                 * (1) samplerAnisotropy
                 * (2) sampleRateShading
                 * 
                 * Note that, even though it is very unlikely that a modern graphics card will not support it, we still 
                 * check if it is available when picking the physical device
                */
                requiredFeatures.samplerAnisotropy = VK_TRUE;
                requiredFeatures.sampleRateShading = VK_TRUE;

                VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
                descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
                descriptorIndexingFeatures.pNext = VK_NULL_HANDLE;
                /* Enable only the following descriptor indexing features, note that we have queried for their support 
                 * already while selecting the phy device
                 * (1) runtimeDescriptorArray
                */
                descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

                auto requiredFeatures2 = getPhyDeviceFeatures2 (deviceInfo->resource.phyDevice,
                                                                &requiredFeatures,
                                                                &descriptorIndexingFeatures,
                                                                false);
                createInfo.pNext = &requiredFeatures2;
                /* We're now ready to instantiate the logical device
                 * NOTE: Logical devices don't interact directly with instances, which is why it's not included as a 
                 * parameter while creating or destroying it
                */
                VkDevice logDevice;
                VkResult result = vkCreateDevice (deviceInfo->resource.phyDevice, 
                                                  &createInfo, 
                                                  VK_NULL_HANDLE, 
                                                  &logDevice);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKLogDeviceLog) << "Failed to create logic device " 
                                                 << "[" << deviceInfoId << "]"
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
                vkGetDeviceQueue (logDevice, deviceInfo->meta.graphicsFamilyIndex.value(), 0, &graphicsQueue);
                vkGetDeviceQueue (logDevice, deviceInfo->meta.presentFamilyIndex. value(), 0, &presentQueue);
                vkGetDeviceQueue (logDevice, deviceInfo->meta.transferFamilyIndex.value(), 0, &transferQueue);

                deviceInfo->resource.logDevice     = logDevice;
                deviceInfo->resource.graphicsQueue = graphicsQueue;
                deviceInfo->resource.presentQueue  = presentQueue;
                deviceInfo->resource.transferQueue = transferQueue;
            }

            void cleanUp (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                vkDestroyDevice (deviceInfo->resource.logDevice, VK_NULL_HANDLE);
            }
    };
}   // namespace Core
#endif  // VK_LOG_DEVICE_H