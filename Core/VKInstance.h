#ifndef VK_INSTANCE_H
#define VK_INSTANCE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKValidation.h"
#include "../Collections/Log/include/Log.h"
#include <vector>
#include <set>

using namespace Collections;

namespace Renderer {
    class VKInstance: protected VKValidation {
        private:
            /* List on instance level extensions
            */
            std::vector <const char*> m_instanceExtensions;
            /* Application name
            */
            const char* m_applicationName = APPLICATION_NAME;
            /* Handle to the log object
            */
            static Log::Record* m_VKInstanceLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 9;

            void getInstanceExtensions (void) {
                /* Since Vulkan is a platform agnostic API, it can not interface directly with the window system on its 
                 * own. To establish the connection between Vulkan and the window system to present results to the 
                 * screen, we need to use the WSI (Window System Integration) extensions (ex: VK_KHR_surface) (included 
                 * in glfw extensions)
                */
                uint32_t glfwExtensionCount = 0;
                const char** glfwExtensions;
                glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);

                for (uint32_t i = 0; i < glfwExtensionCount; i++)
                    m_instanceExtensions.emplace_back (glfwExtensions[i]);

#if __APPLE__
                /* If using MacOS with the latest MoltenVK sdk, you may get VK_ERROR_INCOMPATIBLE_DRIVER (-9) returned 
                 * from vkCreateInstance. Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension 
                 * is mandatory. To get over this error, first add the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 
                 * bit to VkInstanceCreateInfo struct's flags, then add VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME 
                 * to instance enabled extension list. 
                 * 
                 * Also, the "VK_KHR_get_physical_device_properties2" extension must be enabled for the Vulkan instance 
                 * because it's listed as a dependency for the "VK_KHR_portability_subset" device extension
                */
                m_instanceExtensions.emplace_back (VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                m_instanceExtensions.emplace_back (VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

                /* The validation layers will print debug messages to the standard output by default, but we can also 
                 * handle them ourselves by providing an explicit callback in our program. Set up a debug messenger 
                 * extension with a callback using the VK_EXT_debug_utils extension.
                */
                if (isValidationLayersEnabled())
                    m_instanceExtensions.emplace_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            void deleteInstanceExtensions (void) {
                m_instanceExtensions.clear();
            }

            bool checkInstanceExtensionSupport (void) {
                /* Query all available extensions, to allocate an array to hold the extension details we first need to 
                 * know how many there are. You can request just the number of extensions by leaving the latter parameter 
                 * empty
                */
                uint32_t extensionCount = 0;
                vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);
                std::vector <VkExtensionProperties> availableExtensions  (extensionCount);
                vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, availableExtensions.data());

                LOG_INFO (m_VKInstanceLog) << "Available instance extensions" << std::endl;
                for (const auto& extension: availableExtensions)
                    LOG_INFO (m_VKInstanceLog) << extension.extensionName 
                                               << "," 
                                               << extension.specVersion 
                                               << std::endl;
                
                LOG_INFO (m_VKInstanceLog) << "Required instance extensions" << std::endl;
                for (const auto& extension: m_instanceExtensions)
                    LOG_INFO (m_VKInstanceLog) << extension << std::endl;

                /* Use a set of strings here to represent the unconfirmed required extensions. That way we can easily 
                 * tick them off while enumerating the sequence of available extensions
                */
                std::set <std::string> requiredExtensions (m_instanceExtensions.begin(), m_instanceExtensions.end());
                for (const auto& extension: availableExtensions)
                    requiredExtensions.erase (extension.extensionName);

                return requiredExtensions.empty();
            }

        public:
            VKInstance (void) {
                m_VKInstanceLog = LOG_INIT (m_instanceId, 
                                            static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & 
                                                                       (Log::WARNING | Log::ERROR)),
                                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                            "./Build/Log/");
            }

            ~VKInstance (void) { 
                LOG_CLOSE (m_instanceId);
            }
            
        protected:
            void createInstance (void) {
                /* Clear and populate instance extension list every time you create an instance
                */
                deleteInstanceExtensions();
                getInstanceExtensions();

                /* This data is technically optional when creating an instance, but it may provide some useful 
                 * information to the driver in order to optimize our specific application
                */
                VkApplicationInfo appInfo{};
                /* Many structures in Vulkan require you to explicitly specify the type of structure in the sType member
                */
                appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                appInfo.pApplicationName = m_applicationName;
                appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.pEngineName = "No Engine";
                appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.apiVersion = VK_API_VERSION_1_0;

                /* This next struct is not optional and tells the Vulkan driver which global extensions and validation 
                 * layers we want to use
                */
                VkInstanceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                createInfo.pApplicationInfo = &appInfo;

                /* Why do we need a separated debug messenger struct? 
                 * The vkCreateDebugUtilsMessengerEXT call requires a valid instance to have been created and 
                 * vkDestroyDebugUtilsMessengerEXT must be called before the instance is destroyed. This currently leaves 
                 * us unable to debug any issues in the vkCreateInstance and vkDestroyInstance calls. However, you'll see 
                 * that there is a way to create a separate debug utils messenger specifically for those two function 
                 * calls. It requires you to simply pass a pointer to a VkDebugUtilsMessengerCreateInfoEXT struct in the 
                 * pNext extension field of VkInstanceCreateInfo
                */
                VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

                /* Setup validation layers
                 * Vulkan allows you to enable extensive checks through a feature known as validation layers. Validation 
                 * layers are pieces of code that can be inserted between the API and the graphics driver to do things 
                 * like running extra checks on function parameters and tracking memory management problems. The nice 
                 * thing is that you can enable them during development and then completely disable them when releasing 
                 * your application for zero overhead.
                */
                if (isValidationLayersEnabled() && !checkValidationLayerSupport()) {
                    LOG_WARNING (m_VKInstanceLog) << "Required validation layers not available" << std::endl;
                    createInfo.enabledLayerCount = 0;
                    createInfo.pNext = nullptr;
                }
                else if (isValidationLayersEnabled()) {
                    createInfo.enabledLayerCount = static_cast <uint32_t> (getValidationLayers().size());
                    createInfo.ppEnabledLayerNames = getValidationLayers().data();

                    /* By creating an additional debug messenger this way it will automatically be used during 
                     * vkCreateInstance and vkDestroyInstance and cleaned up after that
                    */
                    populateDebugMessengerCreateInfo (debugCreateInfo);
                    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
                }

                /* Setup instance extensions
                */
                createInfo.enabledExtensionCount = static_cast <uint32_t> (m_instanceExtensions.size());
                createInfo.ppEnabledExtensionNames = m_instanceExtensions.data();
#if __APPLE__
                createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
                /* Verify instance extension support
                */
                if (!checkInstanceExtensionSupport()) {
                    LOG_ERROR (m_VKInstanceLog) << "Required instance extensions not available" << std::endl;
                    throw std::runtime_error ("Required instance extensions not available");
                }

                /* We are ready to create an instance, nearly all Vulkan functions return a value of type VkResult that 
                 * is either VK_SUCCESS or an error code
                */
                VkInstance instance;
                VkResult result = vkCreateInstance (&createInfo, nullptr, &instance);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKInstanceLog) << "Failed to create instance" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create instance");
                }
                setInstance (instance);
            }

            void cleanUp (void) {
                /* The VkInstance should be only destroyed right before the program exits, all of the other Vulkan 
                 * resources that we create should be cleaned up before the instance is destroyed
                */
                vkDestroyInstance (getInstance(), nullptr);
            }
    };

    /* static variables are essentially syntactic sugar around global variables. Just like global variables, they must 
     * be defined in exactly one source file
    */
    Log::Record* VKInstance::m_VKInstanceLog;
}   // namespace Renderer
#endif  // VK_INSTANCE_H