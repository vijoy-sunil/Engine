#ifndef VK_QUEUE_H
#define VK_QUEUE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSurface.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKQueue: protected VKSurface {
        private:
            /* Handle to graphics queue, device queues are implicitly cleaned up when the device is destroyed, so we 
             * don't need to do anything in the cleanup function
            */
            VkQueue m_graphicsQueue;
            /* Handle to present queue
            */
            VkQueue m_presentQueue;
            /* Handle to the log object
            */
            static Log::Record* m_VKQueueLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 5;
            
        public:
            VKQueue (void) {
                m_VKQueueLog = LOG_INIT (m_instanceId, 
                                         Log::VERBOSE, 
                                         Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                         "./Build/Log/");
                LOG_INFO (m_VKQueueLog) << "Constructor called" << std::endl; 
            }

            ~VKQueue (void) {
                LOG_INFO (m_VKQueueLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Holds index of queues in queue families
            */
            struct QueueFamilyIndices {
                /* It's not really possible to use a magic value to indicate the nonexistence of a queue family, since 
                 * any value of uint32_t could in theory be a valid queue family index including 0. std::optional is a 
                 * wrapper that contains no value until you assign something to it.
                */
                std::optional <uint32_t> graphicsFamily;
                /* The presentation is a queue-specific feature, we need to find a queue family that supports presenting 
                 * to the surface we created. It's actually possible that the queue families supporting drawing (graphic) 
                 * commands and the ones supporting presentation do not overlap
                */
                std::optional <uint32_t> presentFamily;
                /* Easy method to quickly check if a family index has been found
                */
                bool isComplete (void) {
                    return graphicsFamily.has_value() && presentFamily.has_value();
                }
            };

            VkQueue getGraphicsQueue (void) {
                return m_graphicsQueue;
            }

            VkQueue getPresentQueue (void) {
                return m_presentQueue;
            }

            void setGraphicsQueue (VkQueue graphicsQueue) {
                m_graphicsQueue = graphicsQueue;
            }

            void setPresentQueue (VkQueue presentQueue) {
                m_presentQueue = presentQueue;
            }            

            /* Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be 
             * submitted to a queue. There are different types of queues that originate from different queue families 
             * and each family of queues allows only a subset of commands
            */
            QueueFamilyIndices checkQueueFamilySupport (VkPhysicalDevice physicalDevice) {
                QueueFamilyIndices indices;
                /* Query list of available queue families
                */
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, nullptr);
                std::vector <VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, queueFamilies.data());

                /* keep track of queue family index
                */
                int i = 0;
                for (const auto& queueFamily : queueFamilies) {
                    /* find a queue family that supports graphics commnands
                    */
                    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        indices.graphicsFamily = i;
                    /* find a queue family that has the capability of presenting to our window surface
                    */
                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, getSurface(), &presentSupport);
                    if (presentSupport)
                        indices.presentFamily = i;
                    /* loop break
                    */
                    if (indices.isComplete())
                        break;

                    i++;
                }
                return indices;
            }
    };

    Log::Record* VKQueue::m_VKQueueLog;
}   // namespace Renderer
#endif  // VK_QUEUE_H