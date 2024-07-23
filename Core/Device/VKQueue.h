#ifndef VK_QUEUE_H
#define VK_QUEUE_H

#include <set>
#include "VKDeviceMgr.h"
#include "../../Utils/LogHelper.h"

using namespace Collections;

namespace Renderer {
    class VKQueue: protected virtual VKDeviceMgr {
        private:
            static Log::Record* m_VKQueueLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKQueue (void) {
                m_VKQueueLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~VKQueue (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            bool isQueueFamilyIndicesComplete (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                return deviceInfo->unique[resourceId].indices.graphicsFamily.has_value() && 
                       deviceInfo->unique[resourceId].indices.presentFamily.has_value()  &&
                       deviceInfo->unique[resourceId].indices.transferFamily.has_value();
            }

            /* Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be 
             * submitted to a queue. There are different types of queues that originate from different queue families 
             * and each family of queues allows only a subset of commands
            */
            void populateQueueFamilyIndices (uint32_t resourceId, VkPhysicalDevice phyDevice) {
                auto deviceInfo = getDeviceInfo();
                /* Query list of available queue families
                */
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties (phyDevice, 
                                                          &queueFamilyCount, 
                                                          VK_NULL_HANDLE);
                LOG_INFO (m_VKQueueLog) << "Queue family count "
                                        << "[" << queueFamilyCount << "]" 
                                        << std::endl;

                std::vector <VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties (phyDevice, 
                                                          &queueFamilyCount, 
                                                          queueFamilies.data());

                int queueFamilyIndex = 0;
                for (auto const& queueFamily: queueFamilies) {
                    /* find a queue family that supports graphics commnands
                    */
                    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        deviceInfo->unique[resourceId].indices.graphicsFamily = queueFamilyIndex;
                    /* find a queue family that has the capability of presenting to our window surface
                    */
                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR (phyDevice, 
                                                          queueFamilyIndex, 
                                                          deviceInfo->unique[resourceId].surface, 
                                                          &presentSupport);
                    if (presentSupport)
                        deviceInfo->unique[resourceId].indices.presentFamily = queueFamilyIndex;
                    /* find a queue family that supports transfer commands
                    */
                    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                        deviceInfo->unique[resourceId].indices.transferFamily = queueFamilyIndex;

                    LOG_INFO (m_VKQueueLog) << "Queue family index "
                                            << "[" << queueFamilyIndex << "]" << std::endl; 
                                            
                    LOG_INFO (m_VKQueueLog) << "Queue family supported flags" 
                                            << std::endl;  
                    auto flags = Utils::splitString (string_VkQueueFlags (queueFamily.queueFlags), "|");
                    for (auto const& flag: flags)
                        LOG_INFO (m_VKQueueLog) << "[" << flag << "]" << std::endl;
                              
                    if (isQueueFamilyIndicesComplete (resourceId))                     
                        break;
                        
                    queueFamilyIndex++;
                }
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

    Log::Record* VKQueue::m_VKQueueLog;
}   // namespace Renderer
#endif  // VK_QUEUE_H