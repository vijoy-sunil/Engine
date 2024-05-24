#ifndef VK_SURFACE_H
#define VK_SURFACE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKInstance.h"
#include "VKWindow.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKSurface: protected VKInstance,
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
            const size_t m_instanceId = 12;

        public:
            VKSurface (void) {
                m_VKSurfaceLog = LOG_INIT (m_instanceId, 
                                          Log::VERBOSE, 
                                          Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                          "./Build/Log/");
                LOG_INFO (m_VKSurfaceLog) << "Constructor called" << std::endl; 
            }

            ~VKSurface (void) {
                LOG_INFO (m_VKSurfaceLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            VkSurfaceKHR getSurface (void) {
                return m_surface;
            }

            void createSurface (void) {
                VkResult result = glfwCreateWindowSurface (getInstance(), getWindow(), nullptr, &m_surface);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKSurfaceLog) << "Failed to create surface" << " " << result << std::endl;
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