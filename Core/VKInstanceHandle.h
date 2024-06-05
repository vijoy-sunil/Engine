#ifndef VK_INSTANCE_HANDLE_H
#define VK_INSTANCE_HANDLE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKConfig.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKInstanceHandle {
        private:
            /* Handle to the instance
            */
            VkInstance m_instance;
            /* Handle to the log object
            */
            static Log::Record* m_VKInstanceHandleLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;
            
        public:
            VKInstanceHandle (void) {
                m_VKInstanceHandleLog = LOG_INIT (m_instanceId, 
                                                  static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE),
                                                  Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                  "./Build/Log/");
            }

            ~VKInstanceHandle (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkInstance getInstance (void) {
                return m_instance;
            }

            void setInstance (VkInstance instance) {
                m_instance = instance;
            }
    };

    Log::Record* VKInstanceHandle::m_VKInstanceHandleLog;
}   // namespace Renderer
#endif  // VK_INSTANCE_HANDLE_H