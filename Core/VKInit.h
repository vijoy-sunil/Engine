#ifndef VK_INIT_H
#define VK_INIT_H

#include "VKInstance.h"
#include "VKValidation.h"
#include "VKSurface.h"
#include "VKPhysicalDevice.h"
#include "VKLogicalDevice.h"
#include "VKSwapChain.h"
#include "VKRenderPass.h"
#include "VKPipeline.h"
#include "VKCmdBuffer.h"
#include "VKDrawFrame.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKInit: protected VKInstance, 
                  protected VKValidation,
                  protected VKSurface,
                  protected VKPhysicalDevice,
                  protected VKLogicalDevice,
                  protected VKSwapChain,
                  protected VKRenderPass,
                  protected VKPipeline,
                  protected VKCmdBuffer,
                  protected VKDrawFrame {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKInitLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 13;

        public:
            VKInit (const char* applicationName,
                    const std::string vertexShaderPath,
                    const std::string fragementShaderPath,
                    uint32_t maxFramesInFlight): VKInstance  (applicationName),
                                                 VKPipeline  (vertexShaderPath, fragementShaderPath),
                                                 VKCmdBuffer (maxFramesInFlight) {
                m_VKInitLog = LOG_INIT (m_instanceId, 
                                        Log::VERBOSE, 
                                        Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                        "./Build/Log/");
                LOG_INFO (m_VKInitLog) << "Constructor called" << std::endl; 
            }

            ~VKInit (void) {
                LOG_INFO (m_VKInitLog) << "Destructor called" << std::endl;
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void initVulkan (void) {
                /* Initialize the Vulkan library by creating an instance. The instance is the connection between your 
                 * application and the Vulkan library and creating it involves specifying some details about your 
                 * application to the driver
                */
                createInstance();
                /* A valid instance to have been created before setting up debug messenger
                */
                setupDebugMessenger();
                /* The window surface needs to be created right after the instance creation
                */
                createSurface();
                /* Next, we need to look for and select a graphics card in the system that supports the features we need
                */
                pickPhysicalDevice();
                /* After selecting a physical device to use we need to set up a logical device to interface with it
                */
                createLogicalDevice();
                /* Create swap chain
                */
                createSwapChain();
                /* Create basic image view for every image in the swap chain so that we can use them as color targets 
                 * later on
                */
                createImageViews();
                /* Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer attachments 
                 * that will be used while rendering. We need to specify how many color and depth buffers there will be, 
                 * how many samples to use for each of them and how their contents should be handled throughout the 
                 * rendering operations. All of this information is wrapped in a render pass object, for which we'll 
                 * create a new createRenderPass function
                */
                createRenderPass();
                /* Graphics pipeline is the sequence of operations that take the vertices and textures of your meshes 
                 * all the way to the pixels in the render targets (ex: window)
                */
                createGraphicsPipeline();
                /* The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer 
                 * object which is created in createFrameBuffers function
                */
                createFrameBuffers();
                /* Create command pool and command buffers
                */
                createCommandPool();
                createCommandBuffers();
                /* Create synchronization primitives (semaphores and fences)
                */
                createSyncObjects();
            }
    };
}   // namespace Renderer
#endif  // VK_INIT_H