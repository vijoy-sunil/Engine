#ifndef VK_BASE_H
#define VK_BASE_H

#include "VKDrawFrame.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKBase: protected VKDrawFrame {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKBaseLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 13;

        public:
            VKBase (void) {
                m_VKBaseLog = LOG_INIT (m_instanceId, 
                                        Log::VERBOSE, 
                                        Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                        "./Build/Log/");
                LOG_INFO (m_VKBaseLog) << "Constructor called" << std::endl; 
            }

            ~VKBase (void) {
                LOG_INFO (m_VKBaseLog) << "Destructor called" << std::endl;
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
                /* Create vertex and index buffer
                */
                createVertexBuffer();
                createIndexBuffer();
                /* Create synchronization primitives (semaphores and fences)
                */
                createSyncObjects();
            }

            void destroyVulkan (void) {
                /* Destroy swap chain and its dependents
                */
                VKFrameBuffer::cleanUp();
                VKImageView::cleanUp();
                VKSwapChain::cleanUp();
                /* Destroy synchronization primitives
                */
                VKDrawFrame::cleanUp();
                /* Destroy vertex buffer
                */
                VKVertexBuffer::cleanUp();
                /* Destroy command pool
                */
                VKGraphicsCmdBuffer::cleanUp();
                /* Destroy pipeline and pipeline layout
                */
                VKPipeline::cleanUp();
                /* Destroy render pas
                */
                VKRenderPass::cleanUp();
                /* Destroy logical device handle
                */
                VKLogDevice::cleanUp();
                /* Destroy debug messenger handle
                */
                VKValidation::cleanUp();
                /* Destroy surface
                */
                VKSurface::cleanUp();
                /* Destroy instance
                */
                VKInstance::cleanUp();                
            }
    };

    Log::Record* VKBase::m_VKBaseLog;
}   // namespace Renderer
#endif  // VK_BASE_H