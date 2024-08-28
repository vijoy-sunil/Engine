#ifndef VK_QUEUE_H
#define VK_QUEUE_H

#include <set>
#include "VKDeviceMgr.h"
#include "../../Utils/LogHelper.h"

using namespace Collections;

namespace Core {
    class VKQueue: protected virtual VKDeviceMgr {
        private:
            Log::Record* m_VKQueueLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

            bool isQueueFamilyIndicesComplete (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                return deviceInfo->meta.graphicsFamilyIndex.has_value() && 
                       deviceInfo->meta.presentFamilyIndex. has_value() &&
                       deviceInfo->meta.transferFamilyIndex.has_value();
            }

        public:
            VKQueue (void) {
                m_VKQueueLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~VKQueue (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be 
             * submitted to a queue. There are different types of queues that originate from different queue families 
             * and each family of queues allows only a subset of commands
            */
            bool pickQueueFamilyIndices (uint32_t deviceInfoId, VkPhysicalDevice phyDevice) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                /* Query list of available queue families
                */
                uint32_t queueFamiliesCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties (phyDevice, 
                                                          &queueFamiliesCount, 
                                                          VK_NULL_HANDLE);

                std::vector <VkQueueFamilyProperties> queueFamilies (queueFamiliesCount);
                vkGetPhysicalDeviceQueueFamilyProperties (phyDevice, 
                                                          &queueFamiliesCount, 
                                                          queueFamilies.data());

                LOG_INFO (m_VKQueueLog) << "Queue families count "
                                        << "[" << deviceInfoId << "]"
                                        << " "
                                        << "[" << queueFamiliesCount << "]" 
                                        << std::endl;

                uint32_t queueFamilyIndex = 0;
                for (auto const& queueFamily: queueFamilies) {
                    LOG_INFO (m_VKQueueLog) << "Queue family index "
                                            << "[" << queueFamilyIndex << "]" 
                                            << std::endl; 
                                            
                    LOG_INFO (m_VKQueueLog) << "Queue family supported flags" 
                                            << std::endl;  
                    auto flags = Utils::getSplitString (string_VkQueueFlags (queueFamily.queueFlags), "|");
                    for (auto const& flag: flags)
                    LOG_INFO (m_VKQueueLog) << "[" << flag << "]" 
                                            << std::endl;

#if ENABLE_AUTO_PICK_QUEUE_FAMILY_INDICES
                    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                       !deviceInfo->meta.graphicsFamilyIndex.has_value())
                        deviceInfo->meta.graphicsFamilyIndex = queueFamilyIndex;

                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR (phyDevice, 
                                                          queueFamilyIndex, 
                                                          deviceInfo->resource.surface, 
                                                          &presentSupport);
                    if (presentSupport &&
                        !deviceInfo->meta.presentFamilyIndex.has_value())
                         deviceInfo->meta.presentFamilyIndex = queueFamilyIndex;

                    if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && 
                       !deviceInfo->meta.transferFamilyIndex.has_value())
                        deviceInfo->meta.transferFamilyIndex = queueFamilyIndex;
#else
                    deviceInfo->meta.graphicsFamilyIndex = g_queueSettings.graphicsFamilyIndex;
                    deviceInfo->meta.presentFamilyIndex  = g_queueSettings.presentFamilyIndex;
                    deviceInfo->meta.transferFamilyIndex = g_queueSettings.transferFamilyIndex;
#endif  // ENABLE_AUTO_PICK_QUEUE_FAMILY_INDICES
                    queueFamilyIndex++;
                }
                return isQueueFamilyIndicesComplete (deviceInfoId);
            }

            bool isQueueFamiliesUnique (const std::vector <uint32_t>& queueFamilyIndices) {
                /* Convert the vector into set
                */
                std::set <uint32_t> setContainer;
                for (size_t i = 0; i < queueFamilyIndices.size(); i++) 
                    setContainer.insert (queueFamilyIndices[i]);

                std::vector <uint32_t> vectorContainer;
                vectorContainer.assign (setContainer.begin(), setContainer.end());
                /* If all the input indices were unique, then the resulting vector size will not be 1
                */
                return vectorContainer.size() != 1;
            }
    };
}   // namespace Core
#endif  // VK_QUEUE_H