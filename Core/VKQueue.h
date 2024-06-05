#ifndef VK_QUEUE_H
#define VK_QUEUE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKSurface.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKQueue: protected VKSurface {
        private:
            /* Holds index of queue families
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
                /* Note that any queue family with VK_QUEUE_GRAPHICS_BIT (graphics queue) or VK_QUEUE_COMPUTE_BIT 
                 * capabilities already implicitly support VK_QUEUE_TRANSFER_BIT (transfer queue) operations. However,
                 * if the application needs a transfer queue that is different from the graphics queue for some reason, 
                 * it can queury a queue family with VK_QUEUE_TRANSFER_BIT and without VK_QUEUE_GRAPHICS_BIT
                */
                std::optional <uint32_t> transferFamily;
            } m_indices;
            /* Handle to graphics queue, device queues are implicitly cleaned up when the device is destroyed, so we 
             * don't need to do anything in the cleanup function
            */
            VkQueue m_graphicsQueue;
            /* Handle to present queue
            */
            VkQueue m_presentQueue;
            /* Handle to transfer queue
            */
            VkQueue m_transferQueue;
            /* Handle to the log object
            */
            static Log::Record* m_VKQueueLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
            
            /* reset std::optional
            */
            void resetIndices (void) {
                m_indices.graphicsFamily.reset();
                m_indices.presentFamily.reset();
                m_indices.transferFamily.reset();
            }
            
        public:
            VKQueue (void) {
                m_VKQueueLog = LOG_INIT (m_instanceId, 
                                         static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                         Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                         "./Build/Log/");
            }

            ~VKQueue (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkQueue getGraphicsQueue (void) {
                return m_graphicsQueue;
            }

            VkQueue getPresentQueue (void) {
                return m_presentQueue;
            }

            VkQueue getTransferQueue (void) {
                return m_transferQueue;
            }

            void setGraphicsQueue (VkQueue graphicsQueue) {
                m_graphicsQueue = graphicsQueue;
            }

            void setPresentQueue (VkQueue presentQueue) {
                m_presentQueue = presentQueue;
            }   

            void setTransferQueue (VkQueue transferQueue) {
                m_transferQueue = transferQueue;
            }          

            /* Check if all required queue family indices have been found
            */
            bool isQueueFamilyIndicesComplete (void) {
                return m_indices.graphicsFamily.has_value() && 
                       m_indices.presentFamily.has_value()  &&
                       m_indices.transferFamily.has_value();
            }

            uint32_t getGraphicsFamilyIndex (void) {
                return m_indices.graphicsFamily.value();
            }

            uint32_t getPresentFamilyIndex (void) {
                return m_indices.presentFamily.value();
            }

            uint32_t getTransferFamilyIndex (void) {
                return m_indices.transferFamily.value();
            }

            /* Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be 
             * submitted to a queue. There are different types of queues that originate from different queue families 
             * and each family of queues allows only a subset of commands
            */
            void populateQueueFamilyIndices (VkPhysicalDevice physicalDevice) {
                /* Reset indices first before populating them
                */
                resetIndices();
                /* Query list of available queue families
                */
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, nullptr);
                std::vector <VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, queueFamilies.data());

                /* keep track of queue family index
                */
                int queueFamilyIndex = 0;
                for (const auto& queueFamily: queueFamilies) {
                    /* find a queue family that supports graphics commnands
                    */
                    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        m_indices.graphicsFamily = queueFamilyIndex;
                    /* find a queue family that has the capability of presenting to our window surface
                    */
                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, 
                                                          queueFamilyIndex, 
                                                          getSurface(), 
                                                          &presentSupport);
                    if (presentSupport)
                        m_indices.presentFamily = queueFamilyIndex;
                    /* find a queue family that supports transfer commands
                    */
                    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                        m_indices.transferFamily = queueFamilyIndex;               
                    /* loop break
                    */
                    if (isQueueFamilyIndicesComplete())
                        break;

                    queueFamilyIndex++;
                }
            }
    };

    Log::Record* VKQueue::m_VKQueueLog;
}   // namespace Renderer
#endif  // VK_QUEUE_H