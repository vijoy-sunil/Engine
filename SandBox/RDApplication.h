#ifndef RD_APPLICATION_H
#define RD_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"

namespace Renderer {
    class RDApplication: protected VKInitSequence,
                         protected virtual VKDrawSequence,
                         protected VKDeleteSequence {
        private:
            uint32_t m_deviceResourcesCount;

            uint32_t m_modelInfoId;
            uint32_t m_vertexBufferInfoId;
            uint32_t m_indexBufferInfoId;
            std::vector <uint32_t> m_uniformBufferInfoIds;
            uint32_t m_swapChainImageInfoIdBase;
            uint32_t m_textureImageInfoId;
            uint32_t m_depthImageInfoId;
            uint32_t m_multiSampleImageInfoId;

            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_resourceId;
            uint32_t m_handOffInfoId;

            bool m_refreshModelTransform;
            bool m_refreshCameraTransform;

        public:
            RDApplication (void) {
                m_deviceResourcesCount     = 1;

                m_modelInfoId              = 0;
                m_vertexBufferInfoId       = 1;
                m_indexBufferInfoId        = 2;
                m_uniformBufferInfoIds     = { 
                                                3, 4 
                                             };
                m_swapChainImageInfoIdBase = 0;
                m_textureImageInfoId       = 0;
                m_depthImageInfoId         = 0;
                m_multiSampleImageInfoId   = 0;

                m_renderPassInfoId         = 0;
                m_pipelineInfoId           = 0;
                m_cameraInfoId             = 0;
                m_resourceId               = 0;
                m_handOffInfoId            = 0;

                m_refreshModelTransform    = false;
                m_refreshCameraTransform   = false;
            }

            ~RDApplication (void) {
            }

            void createScene (void) {
                setDeviceResourceCount (m_deviceResourcesCount);
                auto infoIds      = std::vector {
                                                    m_vertexBufferInfoId, 
                                                    m_indexBufferInfoId, 
                                                    m_swapChainImageInfoIdBase,
                                                    m_textureImageInfoId, 
                                                    m_depthImageInfoId, 
                                                    m_multiSampleImageInfoId
                                                };
                auto infoIdGroups = std::vector {
                                                    {m_uniformBufferInfoIds}
                                                };
                /* Scene #1
                 * Single model - single texture
                */
                readyModelInfo (m_modelInfoId,
                                g_pathSettings.model,
                                g_pathSettings.textureImage,
                                g_pathSettings.vertexShaderBinary,
                                g_pathSettings.fragmentShaderBinary,
                                infoIds, infoIdGroups);

                TransformInfo transformInfo{};
                transformInfo.model.translate      = {0.0f,  0.0f,  0.0f};
                transformInfo.model.rotateAxis     = {0.0f,  0.0f,  1.0f};
                transformInfo.model.scale          = {1.0f,  1.0f,  1.0f};
                transformInfo.model.rotateAngleDeg = 0.0f; 

                transformInfo.camera.position  = {0.0f,  0.0f, -2.0f};
                transformInfo.camera.center    = {0.0f,  0.0f,  0.0f};
                transformInfo.camera.upVector  = {0.0f, -1.0f,  0.0f};
                transformInfo.camera.fovDeg    = 45.0f;
                transformInfo.camera.nearPlane = 0.1f;
                transformInfo.camera.farPlane  = 10.0f;

                VKInitSequence::runSequence (m_modelInfoId, 
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_cameraInfoId,
                                             m_resourceId,
                                             m_handOffInfoId,
                                             transformInfo);
            }

            void runScene (void) {
                auto deviceInfo = getDeviceInfo();
                /* Event loop to keep the application running until either an error occurs or the window is closed
                */
                while (!glfwWindowShouldClose (deviceInfo->unique[m_resourceId].window)) {
                    glfwPollEvents();
#if 0
                    auto handOffInfo = getHandOffInfo (m_handOffInfoId);
                    /* Calculate the time in seconds since rendering has started with floating point accuracy
                    */
                    static auto startTime = std::chrono::high_resolution_clock::now();
                    auto currentTime      = std::chrono::high_resolution_clock::now();
                    float time            = std::chrono::duration <float, std::chrono::seconds::period> 
                                            (currentTime - startTime).count();  
                    
                    handOffInfo->resource.transformInfo.model.rotateAngleDeg  = time * 20.0f;

                    m_refreshModelTransform  = true;
                    m_refreshCameraTransform = false;
#endif
                    VKDrawSequence::runSequence (m_modelInfoId, 
                                                 m_renderPassInfoId,
                                                 m_pipelineInfoId,
                                                 m_cameraInfoId,
                                                 m_resourceId,
                                                 m_handOffInfoId,
                                                 m_refreshModelTransform, m_refreshCameraTransform);
                    /* Reset flags
                    */
                    m_refreshModelTransform  = false;
                    m_refreshCameraTransform = false;                    
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
                                               m_cameraInfoId,
                                               m_resourceId,
                                               m_handOffInfoId);
            }
    };
}   // namespace Renderer
#endif  // RD_APPLICATION_H