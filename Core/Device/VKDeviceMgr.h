#ifndef VK_DEVICE_MGR_H
#define VK_DEVICE_MGR_H

#include <vulkan/vk_enum_string_helper.h>
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Core {
    class VKDeviceMgr {
        private:
            struct DeviceInfo {
                struct Meta {
                    uint32_t memoryAllocationCount;
                    uint32_t swapChainSize;
                    /* It's not really possible to use a magic value to indicate the nonexistence of a queue family, 
                     * since any value of uint32_t could in theory be a valid queue family index including 0. We will be
                     * using std::optional which is a wrapper that contains no value until you assign something to it
                    */
                    std::optional <uint32_t> graphicsFamilyIndex;
                    /* The presentation is a queue-specific feature, we need to find a queue family that supports 
                     * presenting to the surface we created. It's actually possible that the queue families supporting 
                     * drawing (graphic) commands and the ones supporting presentation do not overlap
                    */
                    std::optional <uint32_t> presentFamilyIndex;
                    /* Note that any queue family with VK_QUEUE_GRAPHICS_BIT (graphics queue) or VK_QUEUE_COMPUTE_BIT 
                     * capabilities already implicitly support VK_QUEUE_TRANSFER_BIT (transfer queue) operations. 
                     * However, if the application needs a transfer queue that is different from the graphics queue for 
                     * some reason, it should queury a queue family with VK_QUEUE_TRANSFER_BIT and without 
                     * VK_QUEUE_GRAPHICS_BIT
                    */
                    std::optional <uint32_t> transferFamilyIndex;
                } meta;

                struct Resource {
                    VkInstance instance;
                    /* The graphics card that we'll end up selecting will be stored in a VkPhysicalDevice handle. This 
                     * object will be implicitly destroyed when the VkInstance is destroyed, so we won't need to do 
                     * anything new in the cleanup function
                    */
                    VkPhysicalDevice phyDevice;
                    VkDevice logDevice;
                    GLFWwindow* window;
                    /* VK_KHR_surface (instance level extension) exposes a VkSurfaceKHR object that represents an abstract
                     * type of surface to present rendered images to
                    */
                    VkSurfaceKHR surface;
                    VkQueue graphicsQueue;
                    VkQueue presentQueue;
                    VkQueue transferQueue;
                    VkSwapchainKHR swapChain;
                } resource;

                struct Parameters {
                    VkFormat swapChainFormat;
                    VkPresentModeKHR swapChainPresentMode;
                    VkExtent2D swapChainExtent;
                    /* Sample points for MSAA (multi sample anit aliasing)
                    */
                    VkSampleCountFlagBits maxSampleCount;
                    /* The maximum value that can be specified in the range member of a VkDescriptorBufferInfo structure 
                     * for storage buffer/dynamic descriptors
                    */
                    uint32_t maxStorageBufferRange;
                    uint32_t maxPushConstantsSize;
                    uint32_t maxMemoryAllocationCount;
                    /* maxAnisotropy is the anisotropy value clamp used by the sampler, it limits the amount of texel 
                     * samples that can be used to calculate the final color
                    */
                    float maxSamplerAnisotropy;
                } params;
            };
            std::map <uint32_t, DeviceInfo> m_deviceInfoPool;

            /* List of required device extensions
            */
            const std::vector <const char*> m_deviceExtensions = {
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
            
            Log::Record* m_VKDeviceMgrLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

            void deleteDeviceInfo (uint32_t deviceInfoId) {
                if (m_deviceInfoPool.find (deviceInfoId) != m_deviceInfoPool.end()) {
                    m_deviceInfoPool.erase (deviceInfoId);
                    return;
                }

                LOG_ERROR (m_VKDeviceMgrLog) << "Failed to delete device info "
                                             << "[" << deviceInfoId << "]"          
                                             << std::endl;
                throw std::runtime_error ("Failed to delete device info");
            }

        public:
            VKDeviceMgr (void) {
                m_VKDeviceMgrLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKDeviceMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyDeviceInfo (uint32_t deviceInfoId) {
                if (m_deviceInfoPool.find (deviceInfoId) != m_deviceInfoPool.end()) {
                    LOG_ERROR (m_VKDeviceMgrLog) << "Device info id already exists "
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl;
                    throw std::runtime_error ("Device info id already exists");
                }

                DeviceInfo info{};
                m_deviceInfoPool[deviceInfoId] = info;
            }

            const std::vector <const char*>& getDeviceExtensions (void) {
                return m_deviceExtensions;
            }

            DeviceInfo* getDeviceInfo (uint32_t deviceInfoId) {
                if (m_deviceInfoPool.find (deviceInfoId) != m_deviceInfoPool.end())
                    return &m_deviceInfoPool[deviceInfoId];
                
                LOG_ERROR (m_VKDeviceMgrLog) << "Failed to find device info "
                                             << "[" << deviceInfoId << "]"
                                             << std::endl;
                throw std::runtime_error ("Failed to find device info"); 
            }    

            void dumpDeviceInfoPool (void) {
                LOG_INFO (m_VKDeviceMgrLog) << "Dumping device info pool"
                                            << std::endl;

                for (auto const& [key, val]: m_deviceInfoPool) {
                    LOG_INFO (m_VKDeviceMgrLog) << "Device info id " 
                                                << "[" << key << "]"
                                                << std::endl; 
             
                    LOG_INFO (m_VKDeviceMgrLog) << "Memory allocation count "
                                                << "[" << val.meta.memoryAllocationCount << "]"
                                                << std::endl; 

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain size "
                                                << "[" << val.meta.swapChainSize << "]"
                                                << std::endl; 

                    LOG_INFO (m_VKDeviceMgrLog) << "Graphics queue family index "
                                                << "[" << val.meta.graphicsFamilyIndex.value() << "]"
                                                << std::endl;

                    LOG_INFO (m_VKDeviceMgrLog) << "Present queue family index "
                                                << "[" << val.meta.presentFamilyIndex.value() << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Transfer queue family index "
                                                << "[" << val.meta.transferFamilyIndex.value() << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain format "
                                                << "[" << string_VkFormat (val.params.swapChainFormat) << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain present mode "
                                                << "[" << string_VkPresentModeKHR (val.params.swapChainPresentMode) << "]"
                                                << std::endl;                                                  

                    LOG_INFO (m_VKDeviceMgrLog) << "Swap chain extent "
                                                << "[" << val.params.swapChainExtent.width  << ", "
                                                       << val.params.swapChainExtent.height << "]"
                                                << std::endl;  

                    LOG_INFO (m_VKDeviceMgrLog) << "Max sample count "
                                                << "[" << string_VkSampleCountFlagBits (val.params.maxSampleCount) << "]"
                                                << std::endl; 

                    LOG_INFO (m_VKDeviceMgrLog) << "Max storage buffer range "
                                                << "[" << val.params.maxStorageBufferRange << "]"
                                                << std::endl;

                    LOG_INFO (m_VKDeviceMgrLog) << "Max push constants size "
                                                << "[" << val.params.maxPushConstantsSize << "]"
                                                << std::endl;

                    LOG_INFO (m_VKDeviceMgrLog) << "Max memory allocation count "
                                                << "[" << val.params.maxMemoryAllocationCount << "]"
                                                << std::endl;   

                    LOG_INFO (m_VKDeviceMgrLog) << "Max sampler anisotropy "
                                                << "[" << val.params.maxSamplerAnisotropy << "]"
                                                << std::endl;
                }
            }

            void cleanUpSwapChain (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                vkDestroySwapchainKHR (deviceInfo->resource.logDevice, 
                                       deviceInfo->resource.swapChain, 
                                       VK_NULL_HANDLE);
            }

            void cleanUp (uint32_t deviceInfoId) {
                deleteDeviceInfo (deviceInfoId);
            }
    };
}   // namespace Core
#endif  // VK_DEVICE_MGR_H