#ifndef VK_VALIDATION_H
#define VK_VALIDATION_H

#include <set>
#include "VKDeviceMgr.h"

using namespace Collections;

namespace Core {
    class VKValidation: protected virtual VKDeviceMgr {
        private:
            /* You can simply enable validation layers for debug builds and completely disable them for release builds
             * if needed
            */
            bool m_enableValidationLayers;
            /* This boolean indicates that the required list of validation layers are supported
            */
            bool m_validationLayersSupported;
            /* Handle to the debug callback
            */
            VkDebugUtilsMessengerEXT m_debugMessenger;
            /* Vulkan does not come with any validation layers built-in, but the LunarG Vulkan SDK provides a nice set of 
             * layers that check for common errors. Just like extensions, validation layers need to be enabled by 
             * specifying their name. All of the useful standard validation is bundled into a layer included in the SDK 
             * that is known as VK_LAYER_KHRONOS_validation
            */
            const std::vector <const char*> m_validationLayers = {
                "VK_LAYER_KHRONOS_validation"
            };

            static Log::Record* m_VKValidationLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            /* Logging to a circular buffer requires us to specify the buffer capacity. A multiple of 3 will allow us to
             * save the validation message as a whole (msg, severity and type)
            */
            const size_t m_logBufferCapacity = 3;

            /* Check if required validation layers are supported
            */
            bool isValidationLayersSupported (void) {
                uint32_t layerCount = 0;
                vkEnumerateInstanceLayerProperties (&layerCount, VK_NULL_HANDLE);
                std::vector <VkLayerProperties> availableLayers (layerCount);
                vkEnumerateInstanceLayerProperties (&layerCount, availableLayers.data());

                LOG_INFO (m_VKValidationLog) << "Available validation layers" 
                                             << std::endl;
                for (auto const& layer: availableLayers)
                    LOG_INFO (m_VKValidationLog) << "[" << layer.layerName   << "]"
                                                 << " "
                                                 << "[" << layer.specVersion << "]"
                                                 << std::endl;
                
                LOG_INFO (m_VKValidationLog) << "Required validation layers" 
                                             << std::endl;
                for (auto const& layer: m_validationLayers)
                    LOG_INFO (m_VKValidationLog) << "[" << layer << "]"
                                                 << std::endl;

                std::set <std::string> requiredLayers (m_validationLayers.begin(), m_validationLayers.end());
                for (auto const& layer: availableLayers)
                    requiredLayers.erase (layer.layerName);

                return requiredLayers.empty();  
            }
            
            /* Setup the debug callback function (for validation layer functionality), the VKAPI_ATTR and VKAPI_CALL 
             * ensure that the function has the right signature for Vulkan to call it. The pCallbackData parameter refers 
             * to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself
            */
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                 void* pUserData) {
                /* Suppress unused parameter warning
                */
                static_cast <void> (pUserData);
                LOG_WARNING (m_VKValidationLog) << "Message " 
                                                << "[" 
                                                << pCallbackData->pMessage 
                                                << "]" 
                                                << std::endl;
                LOG_WARNING (m_VKValidationLog) << "Message severity " 
                                                << "[" 
                                                << string_VkDebugUtilsMessageSeverityFlagBitsEXT (messageSeverity) 
                                                << "]" 
                                                << std::endl;
                LOG_WARNING (m_VKValidationLog) << "Message type " 
                                                << "[" 
                                                << string_VkDebugUtilsMessageTypeFlagsEXT (messageType) 
                                                << "]" 
                                                << std::endl;

                /* The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer 
                 * message should be aborted. If the callback returns true, then the call is aborted with the 
                 * VK_ERROR_VALIDATION_FAILED_EXT error.
                */
                return VK_FALSE;
            }

            /* Create the handle to the debug messenger using instance and details about the messenger (createInfo 
             * struct). We have to look up its address ourselves using vkGetInstanceProcAddr since this is an extension 
             * function
            */
            VkResult CreateDebugUtilsMessengerEXT (VkInstance instance, 
                                                   const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                                   const VkAllocationCallbacks* pAllocator, 
                                                   VkDebugUtilsMessengerEXT* pDebugMessenger) {
                
                auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, 
                                                                 "vkCreateDebugUtilsMessengerEXT");
                if (func != VK_NULL_HANDLE)
                    return func (instance, pCreateInfo, pAllocator, pDebugMessenger);
                else
                    return VK_ERROR_EXTENSION_NOT_PRESENT;
            }

            /* Destroy the handle to the debug messenger, similarly to vkCreateDebugUtilsMessengerEXT the function needs 
             * to be explicitly loaded
            */
            void DestroyDebugUtilsMessengerEXT (VkInstance instance, 
                                                const VkDebugUtilsMessengerEXT* debugMessenger, 
                                                const VkAllocationCallbacks* pAllocator) {
                
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, 
                                                                  "vkDestroyDebugUtilsMessengerEXT");
                if (func != VK_NULL_HANDLE)
                    func (instance, *debugMessenger, pAllocator);
            }

        public:
            VKValidation (void) {
                m_enableValidationLayers    = false;
                m_validationLayersSupported = false;
                m_VKValidationLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath, m_logBufferCapacity);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE | 
                                                                                     Log::TO_FILE_BUFFER_CIRCULAR);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKValidation (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void enableValidationLayers (void) {
                m_enableValidationLayers = true;

                if (isValidationLayersSupported())
                    m_validationLayersSupported = true;
            }

            void disableValidationLayers (void) {
                m_enableValidationLayers = false;
            }

            bool isValidationLayersEnabled (void) {
                return m_enableValidationLayers;
            }

            bool isValidationLayersSupportedAlias (void) {
                return m_validationLayersSupported;
            }

            const std::vector <const char*>& getValidationLayers (void) {
                return m_validationLayers;
            }
            
            /* Fill up the struct that will be used to provide details about the debug messenger and its callback:
             * (1) messageSeverity 
             * This field allows you to specify all the types of severities you would like your callback to be called for
             * 
             * (2) messageType 
             * This field lets you filter which types of messages your callback is notified about
             * 
             * (3) pfnUserCallback 
             * This field specifies the pointer to the callback function
             * 
             * (4) pUserData
             * You can optionally pass a pointer to the pUserData field which will be passed along to the callback 
             * function via the pUserData parameter
            */
            void populateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
                createInfo->sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT   | 
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT   | 
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo->messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT       | 
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT    | 
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                createInfo->pfnUserCallback = debugCallback;
                createInfo->pUserData       = VK_NULL_HANDLE;
                createInfo->pNext           = VK_NULL_HANDLE;
                createInfo->flags           = 0;
            }
            
            void createDebugMessenger (uint32_t deviceInfoId) {
                if (!isValidationLayersEnabled())
                    return;

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                VkDebugUtilsMessengerCreateInfoEXT createInfo;
                populateDebugMessengerCreateInfo (&createInfo);

                /* Next, we need to pass this struct to vkCreateDebugUtilsMessengerEXT function to create the handle to 
                 * the debug messenger object (VkDebugUtilsMessengerEXT object) and associate it with our instance
                */
                VkResult result = CreateDebugUtilsMessengerEXT (deviceInfo->resource.instance, 
                                                                &createInfo, 
                                                                VK_NULL_HANDLE, 
                                                                &m_debugMessenger);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKValidationLog) << "Failed to set up debug messenger " 
                                                  << "[" << deviceInfoId << "]"
                                                  << " "
                                                  << "[" << string_VkResult (result) << "]"
                                                  << std::endl;
                    throw std::runtime_error ("Failed to set up debug messenger");
                }
            }

            void cleanUp (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                if (isValidationLayersEnabled())
                    DestroyDebugUtilsMessengerEXT (deviceInfo->resource.instance, 
                                                   &m_debugMessenger, 
                                                   VK_NULL_HANDLE);            
            }
    };

    /* Static variables are essentially syntactic sugar around global variables. Just like global variables, they must 
     * be defined in exactly one source file
    */
    Log::Record* VKValidation::m_VKValidationLog;
}   // namespace Core
#endif  // VK_VALIDATION_H