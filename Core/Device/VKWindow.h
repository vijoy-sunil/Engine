#ifndef VK_WINDOW_H
#define VK_WINDOW_H

#include "VKDeviceMgr.h"

using namespace Collections;

namespace Renderer {
    class VKWindow: protected virtual VKDeviceMgr {
        private:
            static Log::Record* m_VKWindowLog;
            const uint32_t m_instanceId = g_collectionsId++;
            /* Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
             * it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly
             * using this boolean
            */
            bool m_framebufferResized;

            /* The reason that we're creating a static function as a callback is because GLFW does not know how to 
             * properly call a member function with the right 'this' pointer to our VKWindow class instance. However, 
             * we do get a reference to the GLFWwindow in the callback and glfwSetWindowUserPointer function allows you 
             * to store an arbitrary pointer inside of it. The 'this' pointer can then be used to properly set the 
             * boolean to indicate that a resize has happened
            */
            static void framebufferResizeCallback (GLFWwindow* window, int width, int height) {
                /* Suppress unused parameter warning
                */
                static_cast <void> (width);
                static_cast <void> (height);
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                thisPtr->setFrameBufferResized (true);
            }

        public:
            VKWindow (void) {
                m_framebufferResized = false;
                m_VKWindowLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void setFrameBufferResized (bool val) {
                m_framebufferResized = val;
            }

            bool isFrameBufferResized (void) {
                return m_framebufferResized;
            }

            void createWindow (uint32_t resourceId, int width, int height, bool enResizing = true) {
                auto deviceInfo = getDeviceInfo();
                /* First, initialize the GLFW library. Because GLFW was originally designed to create an OpenGL context, 
                 * we need to tell it to not create an OpenGL context with a subsequent call
                */
                glfwInit();
                glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
                if (!enResizing)
                    glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

                /* Create window, note that the fourth parameter allows you to optionally specify a monitor to open the 
                 * window on and the last parameter is only relevant to OpenGL
                */
                std::string windowTitle = g_windowSettings.title + std::to_string (resourceId);
                GLFWwindow *window = glfwCreateWindow (width, 
                                                       height, 
                                                       windowTitle.c_str(), 
                                                       VK_NULL_HANDLE, 
                                                       VK_NULL_HANDLE);
                /* Set user pointer of window, this pointer is used in the callback function
                */
                glfwSetWindowUserPointer (window, this);
                /* To detect window resizes we can use the glfwSetFramebufferSizeCallback function in the GLFW framework 
                 * to set up a callback (this is done to handle resizes explicitly)
                */
                glfwSetFramebufferSizeCallback (window, framebufferResizeCallback);

                deviceInfo->unique[resourceId].window = window;
            }

            void cleanUp (uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                /* Once the window is closed, we need to clean up resources by destroying it and terminating GLFW itself
                */
                glfwDestroyWindow (deviceInfo->unique[resourceId].window);
                glfwTerminate();                
            }
    };
    
    Log::Record* VKWindow::m_VKWindowLog;
}   // namespace Renderer
#endif  // VK_WINDOW_H