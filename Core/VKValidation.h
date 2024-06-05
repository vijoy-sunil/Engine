#ifndef VK_VALIDATION_H
#define VK_VALIDATION_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKInstanceHandle.h"
#include "../Collections/Log/include/Log.h"
#include <vector>
#include <set>

using namespace Collections;

namespace Renderer {
    class VKValidation: protected virtual VKInstanceHandle {
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
            /* Handle to the log object
            */
            static Log::Record* m_VKValidationLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
            /* Logging to a circular buffer requires us to specify the buffer capacity. A multiple of 3 will allow us to
             * save the validation message as a whole (msg, severity and type)
            */
            const size_t m_logBufferCapacity = 3;

            /* Check if required validation layers are supported
            */
            bool checkValidationLayerSupport (void) {
                /* Query all available layers
                */
                uint32_t layerCount = 0;
                vkEnumerateInstanceLayerProperties (&layerCount, nullptr);
                std::vector <VkLayerProperties> availableLayers (layerCount);
                vkEnumerateInstanceLayerProperties (&layerCount, availableLayers.data());

                LOG_INFO (m_VKValidationLog) << "Available validation layers" << std::endl;
                for (const auto& layer: availableLayers)
                    LOG_INFO (m_VKValidationLog) << layer.layerName << "," << layer.specVersion << std::endl;
                
                LOG_INFO (m_VKValidationLog) << "Required validation layers" << std::endl;
                for (const auto& layer: m_validationLayers)
                    LOG_INFO (m_VKValidationLog) << layer << std::endl;

                /* Use a set of strings here to represent the unconfirmed required layers. That way we can easily tick
                 * them off while enumerating the sequence of available layers
                */
                std::set <std::string> requiredLayers (m_validationLayers.begin(), m_validationLayers.end());
                for (const auto& layer: availableLayers)
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
                LOG_WARNING (m_VKValidationLog) << "Validation layer/msg: " << pCallbackData-> pMessage << std::endl;
                LOG_WARNING (m_VKValidationLog) << "Validation layer/msg severity: " << messageSeverity << std::endl;
                LOG_WARNING (m_VKValidationLog) << "Validation layer/msg type: " << messageType << std::endl;

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
                if (func != nullptr)
                    return func (instance, pCreateInfo, pAllocator, pDebugMessenger);
                else
                    return VK_ERROR_EXTENSION_NOT_PRESENT;
            }

            /* Destroy the handle to the debug messenger, similarly to vkCreateDebugUtilsMessengerEXT the function needs 
             * to be explicitly loaded
            */
            void DestroyDebugUtilsMessengerEXT (VkInstance instance, 
                                                VkDebugUtilsMessengerEXT debugMessenger, 
                                                const VkAllocationCallbacks* pAllocator) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, 
                                                                  "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr)
                    func (instance, debugMessenger, pAllocator);
            }

        public:
            VKValidation (void) {
                m_enableValidationLayers = false;
                m_validationLayersSupported = false;
                m_VKValidationLog = LOG_INIT (m_instanceId, 
                                              static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & 
                                                                         (Log::WARNING | Log::ERROR)),
                                              Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE | Log::TO_FILE_BUFFER_CIRCULAR, 
                                              "./Build/Log/",
                                              m_logBufferCapacity);
            }

            ~VKValidation (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void enableValidationLayers (void) {
                m_enableValidationLayers = true;

                if (checkValidationLayerSupport())
                    m_validationLayersSupported = true;
            }

            bool isValidationLayersEnabled (void) {
                return m_enableValidationLayers;
            }

            bool isValidationLayersSupported (void) {
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
             * 
             * NOTE: We need this as a separate function rather than being used inside the setup debug messenger function
            */
            void populateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
                createInfo.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                createInfo.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT   | 
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT   | 
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT       | 
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT    | 
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                createInfo.pfnUserCallback  = debugCallback;
                createInfo.pUserData        = nullptr;
            }
            
            void setupDebugMessenger (void) {
                if (!m_enableValidationLayers)
                    return;

                VkDebugUtilsMessengerCreateInfoEXT createInfo{};
                populateDebugMessengerCreateInfo (createInfo);

                /* Next, we need to pass this struct to vkCreateDebugUtilsMessengerEXT function to create the handle to 
                 * the debug messenger object (VkDebugUtilsMessengerEXT object) and associate it with our instance
                */
                if (CreateDebugUtilsMessengerEXT (getInstance(), 
                                                  &createInfo, 
                                                  nullptr, 
                                                  &m_debugMessenger) != VK_SUCCESS) {
                    LOG_ERROR (m_VKValidationLog) << "Failed to set up debug messenger" << std::endl;
                    throw std::runtime_error ("Failed to set up debug messenger");
                }
            }

            void cleanUp (void) {
                /* Destroy debug messenger handle
                */
                if (isValidationLayersEnabled())
                    DestroyDebugUtilsMessengerEXT (getInstance(), m_debugMessenger, nullptr);            
            }
    };

    Log::Record* VKValidation::m_VKValidationLog;
}   // namespace Renderer
#endif  // VK_VALIDATION_H