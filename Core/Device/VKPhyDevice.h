#ifndef VK_PHY_DEVICE_H
#define VK_PHY_DEVICE_H

#include "VKQueue.h"

using namespace Collections;

namespace Renderer {
    class VKPhyDevice: protected VKQueue {
        private:
            static Log::Record* m_VKPhyDeviceLog;
            const uint32_t m_instanceId = g_collectionsId++;

            bool isDeviceExtensionsSupported (VkPhysicalDevice phyDevice) {
                auto deviceInfo = getDeviceInfo();
                /* Query all available extensions
                */
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties (phyDevice, 
                                                      VK_NULL_HANDLE, 
                                                      &extensionCount, 
                                                      VK_NULL_HANDLE);
                std::vector <VkExtensionProperties> availableExtensions (extensionCount);
                vkEnumerateDeviceExtensionProperties (phyDevice, 
                                                      VK_NULL_HANDLE, 
                                                      &extensionCount, 
                                                      availableExtensions.data());

                LOG_INFO (m_VKPhyDeviceLog) << "Available device extensions" 
                                            << std::endl;
                for (auto const& extension: availableExtensions)
                    LOG_INFO (m_VKPhyDeviceLog) << "[" << extension.extensionName << "]" 
                                                << " "
                                                << "[" << extension.specVersion   << "]"
                                                << std::endl;

                /* Use a set of strings here to represent the unconfirmed required extensions. That way we can easily 
                 * tick them off while enumerating the sequence of available extensions
                */
                std::set <std::string> requiredExtensions (deviceInfo->meta.deviceExtensions.begin(), 
                                                           deviceInfo->meta.deviceExtensions.end());
                for (auto const& extension: availableExtensions)
                    requiredExtensions.erase (extension.extensionName);

                return requiredExtensions.empty();
            }

            bool isPhyDeviceSupported (uint32_t resourceId, VkPhysicalDevice phyDevice) {
                /* List of gpu devices have already been queried and is passed into this function one by one, which is 
                 * then checked for support
                */
                bool queueFamilyIndicesComplete = pickQueueFamilyIndices (resourceId, phyDevice);
                /* Check device extension support
                */
                bool extensionsSupported = isDeviceExtensionsSupported (phyDevice);
                /* It should be noted that the availability of a presentation queue, implies that the swap chain 
                 * extension must be supported. However, it's still good to be explicit about things, and the extension 
                 * does have to be explicitly enabled
                */
                bool swapChainAdequate = false;
                if (extensionsSupported) {
                    auto swapChainSupport = getSwapChainSupportDetails (resourceId, phyDevice);
                    /* Swap chain support is sufficient for now if there is at least one supported image format and one 
                     * supported presentation mode given the window surface we have
                    */
                    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
                }

                VkPhysicalDeviceFeatures supportedFeatures;
                vkGetPhysicalDeviceFeatures (phyDevice, &supportedFeatures);

                VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
                descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
                descriptorIndexingFeatures.pNext = VK_NULL_HANDLE;
                getPhyDeviceFeatures2 (phyDevice, VK_NULL_HANDLE, &descriptorIndexingFeatures);

                return queueFamilyIndicesComplete && 
                       extensionsSupported && 
                       swapChainAdequate   &&
                       supportedFeatures.samplerAnisotropy &&
                       /* This indicates whether the implementation supports the SPIR-V run time descriptor array 
                        * capability. If this feature is not enabled, descriptors must not be declared in runtime arrays
                       */
                       descriptorIndexingFeatures.runtimeDescriptorArray;
            }

            /* The exact maximum number of sample points for MSAA can be extracted from VkPhysicalDeviceProperties 
             * associated with our selected physical device. We're using a depth buffer, so we have to take into account 
             * the sample count for both color and depth. The highest sample count that is supported by both (&) will be 
             * the maximum we can support
            */
            VkSampleCountFlagBits getMaxUsableSampleCount (void) {
                auto deviceInfo = getDeviceInfo();
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties (deviceInfo->shared.phyDevice, &properties);

                VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & 
                                            properties.limits.framebufferDepthSampleCounts;

                if (counts & VK_SAMPLE_COUNT_64_BIT)    return VK_SAMPLE_COUNT_64_BIT;
                if (counts & VK_SAMPLE_COUNT_32_BIT)    return VK_SAMPLE_COUNT_32_BIT;
                if (counts & VK_SAMPLE_COUNT_16_BIT)    return VK_SAMPLE_COUNT_16_BIT;
                if (counts & VK_SAMPLE_COUNT_8_BIT)     return VK_SAMPLE_COUNT_8_BIT;
                if (counts & VK_SAMPLE_COUNT_4_BIT)     return VK_SAMPLE_COUNT_4_BIT;
                if (counts & VK_SAMPLE_COUNT_2_BIT)     return VK_SAMPLE_COUNT_2_BIT;

                return VK_SAMPLE_COUNT_1_BIT;
            }

        public:
            VKPhyDevice (void) {
                m_VKPhyDeviceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir); 
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);               
            }

            ~VKPhyDevice (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Just checking if a swap chain is available is not sufficient, because it may not actually be compatible 
             * with our window surface. Creating a swap chain also involves a lot more settings than instance and device 
             * creation, so we need to query for some more details before we're able to proceed. There are basically 
             * three kinds of properties we need to check: 
             * 
             * (1) Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
             * (2) Surface formats (pixel format, color space)
             * (3) Available presentation modes
            */
            struct SwapChainSupportDetails {
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector <VkSurfaceFormatKHR> formats;
                std::vector <VkPresentModeKHR> presentModes;
            };

            SwapChainSupportDetails getSwapChainSupportDetails (uint32_t resourceId, VkPhysicalDevice phyDevice) {
                auto deviceInfo = getDeviceInfo();
                SwapChainSupportDetails details;
                /* (1)
                */
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR (phyDevice, 
                                                           deviceInfo->unique[resourceId].surface, 
                                                           &details.capabilities);
                /* (2)
                */
                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR (phyDevice, 
                                                      deviceInfo->unique[resourceId].surface, 
                                                      &formatCount, 
                                                      VK_NULL_HANDLE);
                if (formatCount != 0) {
                    details.formats.resize (formatCount);
                    vkGetPhysicalDeviceSurfaceFormatsKHR (phyDevice, 
                                                          deviceInfo->unique[resourceId].surface, 
                                                          &formatCount, 
                                                          details.formats.data());
                }
                /* (3)
                */
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR (phyDevice, 
                                                           deviceInfo->unique[resourceId].surface, 
                                                           &presentModeCount, 
                                                           VK_NULL_HANDLE);
                if (presentModeCount != 0) {
                    details.presentModes.resize (presentModeCount);
                    vkGetPhysicalDeviceSurfacePresentModesKHR (phyDevice, 
                                                               deviceInfo->unique[resourceId].surface, 
                                                               &presentModeCount, 
                                                               details.presentModes.data());
                }
                return details;
            }
            
            /* Graphics cards can offer different types of memory to allocate from. Each type of memory varies in terms 
             * of allowed operations and performance characteristics. We need to combine the requirements of the resource
             * (image, buffer etc.) and our own application requirements to find the right type of memory to use
            */
            uint32_t getMemoryTypeIndex (uint32_t typeFilter, VkMemoryPropertyFlags properties) {
                auto deviceInfo = getDeviceInfo();
                /* First we need to query info about the available types of memory
                 *
                 * The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps
                 * (1) memoryTypes is an array of VkMemoryType structures describing the memory types that can be used to 
                 * access memory allocated from the heaps specified by memoryHeaps
                 * (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT etc.)
                 * 
                 * (2) memoryHeaps is an array of VkMemoryHeap structures describing the memory heaps from which memory 
                 * can be allocated. Memory heaps are distinct memory resources like dedicated VRAM and swap space in 
                 * RAM for when VRAM runs out. 
                 * (VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, VK_MEMORY_HEAP_MULTI_INSTANCE_BIT etc.)
                 * 
                 * Note that, we are only concerning ourselves with the type of memory and not the heap it comes from, but
                 * you can imagine that this can affect performance
                */
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties (deviceInfo->shared.phyDevice, &memProperties);

                LOG_INFO (m_VKPhyDeviceLog) << "Physical device memory types" 
                                            << std::endl;               
                for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                    LOG_INFO (m_VKPhyDeviceLog) << "[" << i << "]" 
                                                << std::endl;

                    LOG_INFO (m_VKPhyDeviceLog) << "Heap index "
                                                << "[" << memProperties.memoryTypes[i].heapIndex << "]"
                                                << std::endl;

                    auto flags = Utils::getSplitString (string_VkMemoryPropertyFlags 
                                                       (memProperties.memoryTypes[i].propertyFlags), "|");
                    for (auto const& flag: flags)
                    LOG_INFO (m_VKPhyDeviceLog) << "[" << flag << "]" 
                                                << std::endl; 
                }

                LOG_INFO (m_VKPhyDeviceLog) << "Physical device memory heaps" 
                                            << std::endl;  
                for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
                    LOG_INFO (m_VKPhyDeviceLog) << "[" << i << "]" 
                                                << std::endl;

                    LOG_INFO (m_VKPhyDeviceLog) << "Heap size (bytes) "
                                                << "[" << memProperties.memoryHeaps[i].size << "]"
                                                << std::endl;

                    auto flags = Utils::getSplitString (string_VkMemoryHeapFlags 
                                                       (memProperties.memoryHeaps[i].flags), "|");
                    for (auto const& flag: flags)
                    LOG_INFO (m_VKPhyDeviceLog) << "[" << flag << "]" 
                                                << std::endl;                                                     
                }                                                                                              
                /* We may have more than one desirable property, so we should check if the result of the bitwise AND is 
                 * not just non-zero, but equal to the desired properties bit field. If there is a memory type suitable 
                 * for the resource that also has all of the properties we need, then we return its index, otherwise we 
                 * throw an exception 
                */
                LOG_INFO (m_VKPhyDeviceLog) << "Desired memory properties" 
                                            << std::endl;  
                auto flags = Utils::getSplitString (string_VkMemoryPropertyFlags (properties), "|");
                for (auto const& flag: flags)
                LOG_INFO (m_VKPhyDeviceLog) << "[" << flag << "]" 
                                            << std::endl;                                                   

                for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                    if ((typeFilter & (1 << i)) && 
                        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        LOG_INFO (m_VKPhyDeviceLog) << "Memory type index " 
                                                    << "[" << i << "]" 
                                                    << std::endl; 
                        return i;
                        }
                }

                LOG_ERROR (m_VKPhyDeviceLog) << "Failed to find suitable memory type" 
                                             << std::endl;
                throw std::runtime_error ("Failed to find suitable memory type");
            }

            VkPhysicalDeviceFeatures2 getPhyDeviceFeatures2 (VkPhysicalDevice phyDevice, 
                                                             const VkPhysicalDeviceFeatures* features,
                                                             void* pNext,
                                                             bool querySupport = true) {

                VkPhysicalDeviceFeatures2 supportedFeatures2;
                supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                if (features != VK_NULL_HANDLE)
                    supportedFeatures2.features = *features;
                    
                /* If the VkPhysicalDevice[ExtensionName]Features structure is included in the pNext chain of the 
                 * VkPhysicalDeviceFeatures2 structure passed to vkGetPhysicalDeviceFeatures2, it is filled in to 
                 * indicate whether each corresponding feature is supported
                */
                supportedFeatures2.pNext = pNext;
                if (querySupport == true)
                    vkGetPhysicalDeviceFeatures2 (phyDevice, &supportedFeatures2);

                return supportedFeatures2;
            }

            void pickPhyDevice (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                /* Query all available graphic cards with vulkan support
                */
                uint32_t phyDevicesCount = 0;
                vkEnumeratePhysicalDevices (deviceInfo->shared.instance, &phyDevicesCount, VK_NULL_HANDLE);
                if (phyDevicesCount == 0) {
                    LOG_ERROR (m_VKPhyDeviceLog) << "Failed to find GPUs with Vulkan support" 
                                                 << std::endl;
                    throw std::runtime_error ("Failed to find GPUs with Vulkan support");
                }
                std::vector <VkPhysicalDevice> phyDevices (phyDevicesCount);
                vkEnumeratePhysicalDevices (deviceInfo->shared.instance, &phyDevicesCount, phyDevices.data());

                for (auto const& phyDevice: phyDevices) {
                    if (isPhyDeviceSupported (resourceId, phyDevice)) {
                        VkPhysicalDeviceProperties properties;
                        vkGetPhysicalDeviceProperties (phyDevice, &properties);

                        deviceInfo->shared.phyDevice                = phyDevice;
                        deviceInfo->params.maxSampleCount           = getMaxUsableSampleCount();
                        deviceInfo->params.maxPushConstantsSize     = properties.limits.maxPushConstantsSize;
                        deviceInfo->params.maxMemoryAllocationCount = properties.limits.maxMemoryAllocationCount;
                        deviceInfo->params.maxSamplerAnisotropy     = properties.limits.maxSamplerAnisotropy;
                        break;
                    }
                }

                if (deviceInfo->shared.phyDevice == VK_NULL_HANDLE) {
                    LOG_ERROR (m_VKPhyDeviceLog) << "GPU doesn't meet required expectations" 
                                                 << std::endl;
                    throw std::runtime_error ("GPU doesn't meet required expectations");
                }
            }
    };

    Log::Record* VKPhyDevice::m_VKPhyDeviceLog;
}   // namespace Renderer
#endif  // VK_PHY_DEVICE_H