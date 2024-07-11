#ifndef RD_APPLICATION_H
#define RD_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"

namespace Renderer {
    class RDApplication: protected VKInitSequence,
                         protected VKDeleteSequence {
        private:
            uint32_t m_deviceResourcesCount;

            uint32_t m_modelInfoId;
            uint32_t m_vertexBufferInfoId;
            uint32_t m_indexBufferInfoId;
            uint32_t m_uniformBufferInfoId;
            uint32_t m_swapChainImageInfoId;
            uint32_t m_textureImageInfoId;
            uint32_t m_depthImageInfoId;
            uint32_t m_multiSampleImageInfoId;

            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_resourceId;

        public:
            RDApplication (void) {
                m_deviceResourcesCount   = 1;

                m_modelInfoId            = 0;
                m_vertexBufferInfoId     = 1;
                m_indexBufferInfoId      = 2;
                m_uniformBufferInfoId    = 3;
                m_swapChainImageInfoId   = 0;
                m_textureImageInfoId     = 0;
                m_depthImageInfoId       = 0;
                m_multiSampleImageInfoId = 0;

                m_renderPassInfoId       = 0;
                m_pipelineInfoId         = 0;
                m_resourceId             = 0;
            }

            ~RDApplication (void) {
            }

            void createScene (void) {
                setDeviceResourceCount (m_deviceResourcesCount);
                auto infoIds = std::vector {
                                                m_vertexBufferInfoId, 
                                                m_indexBufferInfoId, 
                                                m_uniformBufferInfoId,
                                                m_swapChainImageInfoId, 
                                                m_textureImageInfoId, 
                                                m_depthImageInfoId, 
                                                m_multiSampleImageInfoId
                                           };
                /* Scene #1
                 * Single model - single texture
                */
                readyModelInfo (m_modelInfoId,
                                g_pathSettings.model,
                                g_pathSettings.textureImage,
                                g_pathSettings.vertexShaderBinary,
                                g_pathSettings.fragmentShaderBinary,
                                infoIds);

                VKInitSequence::runSequence (m_modelInfoId, 
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_resourceId);
            }

            void runScene (void) {
                auto deviceInfo = getDeviceInfo();
                /* Event loop to keep the application running until either an error occurs or the window is closed
                */
                while (!glfwWindowShouldClose (deviceInfo->unique[m_resourceId].window)) {
                    glfwPollEvents();
                }
                /* Remember that all of the operations in the above render method are asynchronous. That means that when
                 * we exit the render loop, drawing and presentation operations may still be going on. Cleaning up
                 * resources while that is happening is a bad idea. To fix that problem, we should wait for the logical 
                 * device to finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (deviceInfo->shared.logDevice);
            }

            void deleteScene (void) {
                VKDeleteSequence::runSequence (m_modelInfoId, 
                                               m_renderPassInfoId,
                                               m_pipelineInfoId,
                                               m_resourceId);
            }
    };
}   // namespace Renderer
#endif  // RD_APPLICATION_H