#ifndef VK_RUN_H
#define VK_RUN_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKBase.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKRun: protected VKBase {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKRunLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 14;

            void renderLoop (void) {
                /* Add an event loop to keep the application running until either an error occurs or the window is closed
                */
                while (!glfwWindowShouldClose (getWindow())) {
                    glfwPollEvents();
                    drawFrame();
                }
                /* Remember that all of the operations in drawFrame are asynchronous. That means that when we exit the 
                 * loop in render loop, drawing and presentation operations may still be going on. Cleaning up resources 
                 * while that is happening is a bad idea. To fix that problem, we should wait for the logical device to 
                 * finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (getLogicalDevice());
            }

        public:
            VKRun (void) {
                m_VKRunLog = LOG_INIT (m_instanceId, 
                                       Log::VERBOSE, 
                                       Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                       "./Build/Log/");
                LOG_INFO (m_VKRunLog) << "Constructor called" << std::endl; 
            }

            ~VKRun (void) {
                LOG_INFO (m_VKRunLog) << "Destructor called" << std::endl;
                LOG_CLOSE (m_instanceId);
            }

            void runSequence (void) {
                /* Enable validation layers
                */
                enableValidationLayers();

                initWindow();
                initVulkan();
                renderLoop();
                destroyVulkan();
                VKWindow::cleanUp();
            }
    };

    Log::Record* VKRun::m_VKRunLog;
}   // namespace Renderer
#endif  // VK_RUN_H