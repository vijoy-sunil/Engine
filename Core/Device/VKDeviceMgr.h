#ifndef VK_DEVICE_MGR_H
#define VK_DEVICE_MGR_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Renderer {
    class VKDeviceMgr {
        private:
            /* It's not really possible to use a magic value to indicate the nonexistence of a queue family, since any 
             * value of uint32_t could in theory be a valid queue family index including 0. std::optional is a wrapper 
             * that contains no value until you assign something to it
            */
            struct QueueFamilyIndices {
                std::optional <uint32_t> graphicsFamily;
                /* The presentation is a queue-specific feature, we need to find a queue family that supports presenting 
                 * to the surface we created. It's actually possible that the queue families supporting drawing (graphic) 
                 * commands and the ones supporting presentation do not overlap
                */
                std::optional <uint32_t> presentFamily;
                /* Note that any queue family with VK_QUEUE_GRAPHICS_BIT (graphics queue) or VK_QUEUE_COMPUTE_BIT 
                 * capabilities already implicitly support VK_QUEUE_TRANSFER_BIT (transfer queue) operations. However, if
                 * the application needs a transfer queue that is different from the graphics queue for some reason, it 
                 * should queury a queue family with VK_QUEUE_TRANSFER_BIT and without VK_QUEUE_GRAPHICS_BIT
                */
                std::optional <uint32_t> transferFamily;
            };

            struct DeviceInfo {
                struct Meta {
                    uint32_t memoryAllocationCount;
                    uint32_t deviceResourcesCount;
                    /* List of required device extensions
                    */
                    const std::vector <const char*> deviceExtensions = {
#if __APPLE__
                        "VK_KHR_portability_subset",
#endif  // __APPLE__
                        /* Extensions for enabling swap chain, since image presentation is heavily tied into the window 
                         * system and the surfaces associated with windows, it is not actually part of the Vulkan core
                        */
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        /* Extensions to enable descriptor indexing and bindless (run time) descriptor arrays. With 
                         * bindless, the shader author does not need to know the upper limit of the array, and from the 
                         * application side the implementer only needs to be sure they do not cause the shader to index 
                         * outside of a valid range of bound descriptors
                         * 
                         * Features supported by this extension include
                         * (1) Update after bind            [application side]
                         * (2) Partially bound              [application side]
                         * (3) Dynamic non uniform indexing [shader side]
                         * (4) Run time descriptor array    [shader side]
                        */
                        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                        VK_KHR_MAINTENANCE_3_EXTENSION_NAME
                    }; 
                } meta;

                struct ShadredResources {
                    VkInstance instance;
                    /* The graphics card that we'll end up selecting will be stored in a VkPhysicalDevice handle. This 
                     * object will be implicitly destroyed when the VkInstance is destroyed, so we won't need to do 
                     * anything new in the cleanup function
                    */
                    VkPhysicalDevice phyDevice;
                    VkDevice logDevice;
                } shared;

                struct UniqueResources {
                    GLFWwindow* window;
                    /* VK_KHR_surface (instance level extension) exposes a VkSurfaceKHR object that represents an abstract
                     * type of surface to present rendered images to
                    */
                    VkSurfaceKHR surface;
                    VkQueue graphicsQueue;
                    VkQueue presentQueue;
                    VkQueue transferQueue;
                    QueueFamilyIndices indices;

                    struct SwapChain {
                        VkFormat format;
                        VkPresentModeKHR presentMode;
                        VkExtent2D extent;
                        VkSwapchainKHR swapChain;
                        uint32_t size;
                    } swapChain;
                } unique[g_maxDeviceResourcesCount];

                struct Parameters {
                    /* Sample points for MSAA (multi sample anit aliasing)
                    */
                    VkSampleCountFlagBits maxSampleCount;
                    uint32_t maxPushConstantsSize;
                    uint32_t maxMemoryAllocationCount;
                    /* maxAnisotropy is the anisotropy value clamp used by the sampler, it limits the amount of texel 
                     * samples that can be used to calculate the final color
                    */
                    float maxSamplerAnisotropy;
                } params;
            } m_deviceInfo;

            static Log::Record* m_VKDeviceMgrLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKDeviceMgr (void) {
                m_VKDeviceMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKDeviceMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyDeviceInfo (uint32_t deviceResourceCount) {
                if (deviceResourceCount > g_maxDeviceResourcesCount) {
                    LOG_ERROR (m_VKDeviceMgrLog) << "Invalid device resources count "
                                                 << "[" << deviceResourceCount << "]"
                                                 << "->"
                                                 << "[" << g_maxDeviceResourcesCount << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Invalid device resources count");                    
                }
                m_deviceInfo.meta.deviceResourcesCount = deviceResourceCount;
            }
            
            DeviceInfo* getDeviceInfo (void) {
                if (m_deviceInfo.meta.deviceResourcesCount == 0) {
                    LOG_ERROR (m_VKDeviceMgrLog) << "Invalid device resources count "
                                                 << "[" << m_deviceInfo.meta.deviceResourcesCount << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Invalid device resources count");
                }
                return &m_deviceInfo;
            }    

            void dumpDeviceInfoPool (void) {
                auto deviceInfo = getDeviceInfo();

                LOG_INFO (m_VKDeviceMgrLog) << "Dumping device info pool"
                                            << std::endl;

                LOG_INFO (m_VKDeviceMgrLog) << "Memory allocation count "
                                            << "[" << deviceInfo->meta.memoryAllocationCount << "]"
                                            << std::endl; 

                LOG_INFO (m_VKDeviceMgrLog) << "Device resources count "
                                            << "[" << deviceInfo->meta.deviceResourcesCount << "]"
                                            << std::endl;  

                LOG_INFO (m_VKDeviceMgrLog) << "Required device extensions"
                                            << std::endl;  
                for (auto const& extension: deviceInfo->meta.deviceExtensions)
                    LOG_INFO (m_VKDeviceMgrLog) << "[" << extension << "]"
                                                << std::endl;

                for (uint32_t i = 0; i < deviceInfo->meta.deviceResourcesCount; i++) {
                    LOG_INFO (m_VKDeviceMgrLog) << "Resource id "
                                                << "[" << i << "]"
                                                << std::endl;

                    LOG_INFO (m_VKDeviceMgrLog) << "Graphics queue family index "
                                                << "[" << deviceInfo->unique[i].indices.graphicsFamily.value() << "]"
                                                << std::endl;

                    LOG_INFO (m_VKDeviceMgrLog) << "Present queue family index "
                                                << "[" << deviceInfo->unique[i].indices.presentFamily.value() << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Transfer queue family index "
                                                << "[" << deviceInfo->unique[i].indices.transferFamily.value() << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain format "
                                                << "[" << string_VkFormat
                                                          (deviceInfo->unique[i].swapChain.format) << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain present mode "
                                                << "[" << string_VkPresentModeKHR
                                                          (deviceInfo->unique[i].swapChain.presentMode) << "]"
                                                << std::endl;                                                  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain extent "
                                                << "[" << deviceInfo->unique[i].swapChain.extent.width  << ", "
                                                       << deviceInfo->unique[i].swapChain.extent.height << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain size "
                                                << "[" << deviceInfo->unique[i].swapChain.size << "]"
                                                << std::endl;                      
                }

                LOG_INFO (m_VKDeviceMgrLog) << "Max sample count "
                                            << "[" << string_VkSampleCountFlagBits 
                                                      (deviceInfo->params.maxSampleCount) << "]"
                                            << std::endl; 

                LOG_INFO (m_VKDeviceMgrLog) << "Max push constants size "
                                            << "[" << deviceInfo->params.maxPushConstantsSize << "]"
                                            << std::endl;

                LOG_INFO (m_VKDeviceMgrLog) << "Max memory allocation count "
                                            << "[" << deviceInfo->params.maxMemoryAllocationCount << "]"
                                            << std::endl;   

                LOG_INFO (m_VKDeviceMgrLog) << "Max sampler anisotropy "
                                            << "[" << deviceInfo->params.maxSamplerAnisotropy << "]"
                                            << std::endl;
            }

            void cleanUp (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                vkDestroySwapchainKHR (deviceInfo->shared.logDevice, 
                                       deviceInfo->unique[resourceId].swapChain.swapChain, 
                                       VK_NULL_HANDLE);
            }
    };

    Log::Record* VKDeviceMgr::m_VKDeviceMgrLog;
}   // namespace Renderer
#endif  // VK_DEVICE_MGR_H