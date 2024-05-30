#ifndef VK_WINDOW_H
#define VK_WINDOW_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKConstants.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKWindow {
        private:
            /* Reference to the window
            */
            GLFWwindow* m_window;
            /* Window dimensions
            */
            const uint32_t m_width  = WINDOW_WIDTH;
            const uint32_t m_height = WINDOW_HEIGHT;
            /* Window title
            */
            const char* m_title = WINDOW_TITLE;
            /* Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
             * it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly
            */
            bool m_framebufferResized;
            /* Handle to the log object
            */
            static Log::Record* m_VKWindowLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 11;
            /* The reason that we're creating a static function as a callback is because GLFW does not know how to 
             * properly call a member function with the right 'this' pointer to our VKWindow class instance. However, 
             * we do get a reference to the GLFWwindow in the callback and glfwSetWindowUserPointer function allows you 
             * to store an arbitrary pointer inside of it. The 'this' pointer can then be used to properly set the 
             * boolean framebufferResized
            */
            static void framebufferResizeCallback (GLFWwindow* window, int width, int height) {
                /* Suppress unused parameter warning
                */
                static_cast <void> (width);
                static_cast <void> (height);
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                thisPtr-> setFrameBufferResized (true);
            }

        public:
            VKWindow (void) {
                m_framebufferResized = false;
                m_VKWindowLog = LOG_INIT (m_instanceId, 
                                          static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                          Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                          "./Build/Log/");
            }

            ~VKWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            GLFWwindow* getWindow (void) {
                return m_window;
            }

            void setFrameBufferResized (bool val) {
                m_framebufferResized = val;
            }

            bool isFrameBufferResized (void) {
                return m_framebufferResized;
            }

            void initWindow (void) {
                /* First, initialize the GLFW library. Because GLFW was originally designed to create an OpenGL context, 
                 * we need to tell it to not create an OpenGL context with a subsequent call
                */
                glfwInit();
                glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
                /* Disable window resizing if we are not handling it using glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE)
                */

                /* Create window, note that the fourth parameter allows you to optionally specify a monitor to open the 
                 * window on and the last parameter is only relevant to OpenGL
                */
                m_window = glfwCreateWindow (m_width, m_height, m_title, nullptr, nullptr);
                /* Set user pointer of 'm_window', this pointer is used in the callback function
                */
                glfwSetWindowUserPointer (m_window, this);
                /* To detect window resizes we can use the glfwSetFramebufferSizeCallback function in the GLFW framework 
                 * to set up a callback (this is done to handle resizes explicitly)
                */
                glfwSetFramebufferSizeCallback (m_window, framebufferResizeCallback);
            }

            void cleanUp (void) {
                /* Once the window is closed, we need to clean up resources by destroying it and terminating GLFW itself
                */
                glfwDestroyWindow (m_window);
                glfwTerminate();                
            }
    };
    
    Log::Record* VKWindow::m_VKWindowLog;
}   // namespace Renderer
#endif  // VK_WINDOW_H