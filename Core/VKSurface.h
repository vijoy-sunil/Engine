#ifndef VK_SURFACE_H
#define VK_SURFACE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKInstanceHandle.h"
#include "VKWindow.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>

using namespace Collections;

namespace Renderer {
    class VKSurface: protected virtual VKInstanceHandle,
                     protected VKWindow {
        private:
            /* VK_KHR_surface (instance level extension) exposes a VkSurfaceKHR object that represents an abstract type 
             * of surface to present rendered images to
            */
            VkSurfaceKHR m_surface;
            /* Handle to the log object
            */
            static Log::Record* m_VKSurfaceLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKSurface (void) {
                m_VKSurfaceLog = LOG_INIT (m_instanceId, 
                                           static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                           Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                           "./Build/Log/"); 
            }

            ~VKSurface (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkSurfaceKHR getSurface (void) {
                return m_surface;
            }

            void createSurface (void) {
                VkResult result = glfwCreateWindowSurface (getInstance(), getWindow(), nullptr, &m_surface);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSurfaceLog) << "Failed to create surface " 
                                               << "[" << string_VkResult (result) << "]" 
                                               << std::endl;
                    throw std::runtime_error ("Failed to create surface");
                }
            }

            void cleanUp (void) {
                /* Destroy surface
                */
                vkDestroySurfaceKHR (getInstance(), m_surface, nullptr);               
            }
    };
    
    Log::Record* VKSurface::m_VKSurfaceLog;
}   // namespace Renderer
#endif  // VK_SURFACE_H