#ifndef RD_APPLICATION_H
#define RD_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"

using namespace Core;

namespace SandBox {
    class RDApplication: protected VKInitSequence,
                         protected VKDrawSequence,
                         protected VKDeleteSequence {
        private:
            uint32_t m_deviceResourcesCount;
            uint32_t m_modelInfoIdBase;

            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_inFlightFenceInfoBase;
            uint32_t m_imageAvailableSemaphoreInfoBase;
            uint32_t m_renderDoneSemaphoreInfoBase;
            uint32_t m_resourceId;
            uint32_t m_sceneInfoId;

        public:
            RDApplication (void) {
                m_deviceResourcesCount            = 1;
                m_modelInfoIdBase                 = 0;
                /* Info id overview
                 * |------------------------|-------------------|---------------|---------------|
                 * | MODEL INFO ID          |   0               |   1           |   ...         |
                 * |------------------------|-------------------|---------------|---------------|
                 * | STAGING_BUFFER_TEX     |   0, 1, 2, 3, 4, 5                                |
                 * |------------------------|---------------------------------------------------|
                 * | SWAPCHAIN_IMAGE        |   0, 1, 2, ...                                    |
                 * |------------------------|-------------------|---------------|---------------|
                 * | TEXTURE_IMAGE          |   0, 1, 2         |   0, 2, 1, 3  |   4, 5, 1     |
                 * |------------------------|-------------------|---------------|---------------|
                 * | DEPTH_IMAGE            |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | MULTISAMPLE_IMAGE      |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * |                                                                            |
                 * |------------------------|---------------------------------------------------|
                 * | STAGING_BUFFER         |   0, 1, 2, 3                                      |
                 * |------------------------|-------------------|---------------|---------------|
                 * | VERTEX_BUFFER          |   0               |   UINT32_MAX  |   2           |
                 * |------------------------|-------------------|---------------|---------------|
                 * | INDEX_BUFFER           |   1               |   UINT32_MAX  |   3           |
                 * |------------------------|-------------------|---------------|---------------|
                 * | UNIFORM_BUFFER         |   0, 1, ...                                       |
                 * |------------------------|---------------------------------------------------|
                 * |                                                                            |
                 * |------------------------|---------------------------------------------------|
                 * | RENDER PASS INFO ID    |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | PIPELINE INFO ID       |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | CAMERA INFO ID         |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | FEN_TRANSFER_DONE      |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | FEN_BLIT_DONE          |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | FEN_IN_FLIGHT          |   0, 1, ...                                       |
                 * |------------------------|---------------------------------------------------|
                 * | SEM_IMAGE_AVAILABLE    |   0, 1, ...                                       |
                 * |------------------------|---------------------------------------------------|
                 * | SEM_RENDER_DONE        |   0, 1, ...                                       |
                 * |------------------------|---------------------------------------------------|
                 * | RESOURCE ID            |   0                                               |
                 * |------------------------|---------------------------------------------------|
                 * | SCENE INFO ID          |   0                                               |
                 * |------------------------|---------------------------------------------------|
                */
                m_renderPassInfoId                = 0;
                m_pipelineInfoId                  = 0;
                m_cameraInfoId                    = 0;
                m_inFlightFenceInfoBase           = 0;
                m_imageAvailableSemaphoreInfoBase = 0;
                m_renderDoneSemaphoreInfoBase     = 0;
                m_resourceId                      = 0;
                m_sceneInfoId                     = 0;
            }

            ~RDApplication (void) {
            }

            void createScene (void) {
                /* |------------------------------------------------------------------------------------------------|
                 * | READY DEVICE INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyDeviceInfo (m_deviceResourcesCount);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY MODEL INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (size_t i = 0; i < g_pathSettings.models.size(); i++) {
                    uint32_t modelInfoId = m_modelInfoIdBase + static_cast <uint32_t> (i);

                    readyModelInfo (modelInfoId, 1,
                                    g_pathSettings.models[i],
                                    g_pathSettings.mtlFileDir);
                                    
                    auto modelInfo                 = getModelInfo (modelInfoId);
                    modelInfo->meta.translate      = {0.0f,  0.0f,  0.0f};
                    modelInfo->meta.rotateAxis     = {0.0f,  1.0f,  0.0f};
                    modelInfo->meta.scale          = {1.0f,  1.0f,  1.0f};
                    modelInfo->meta.rotateAngleDeg = 0.0f; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyCameraInfo (m_cameraInfoId);
                auto cameraInfo = getCameraInfo (m_cameraInfoId);

                cameraInfo->meta.position  = {0.0f, -4.0f, -6.0f};
                cameraInfo->meta.center    = {0.0f,  0.0f,  0.0f};
                cameraInfo->meta.upVector  = {0.0f, -1.0f,  0.0f};
                cameraInfo->meta.fovDeg    = 45.0f;
                cameraInfo->meta.nearPlane = 0.1f;
                cameraInfo->meta.farPlane  = 40.0f;
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SCENE INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto sceneInfoIds = std::vector {
                    m_inFlightFenceInfoBase,
                    m_imageAvailableSemaphoreInfoBase,
                    m_renderDoneSemaphoreInfoBase
                };
                readySceneInfo (m_sceneInfoId, sceneInfoIds);

                VKInitSequence::runSequence (m_modelInfoIdBase, 
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_cameraInfoId,
                                             m_resourceId,
                                             m_sceneInfoId);
            }

            void runScene (void) {
                auto deviceInfo = getDeviceInfo();
                /* |------------------------------------------------------------------------------------------------|
                 * | EVENT LOOP                                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                while (!glfwWindowShouldClose (deviceInfo->unique[m_resourceId].window)) {
                    glfwPollEvents();

#if ENABLE_IDLE_ROTATION
                    /* Calculate the time in seconds since rendering has started with floating point accuracy
                    */
                    static auto startTime = std::chrono::high_resolution_clock::now();
                    auto currentTime      = std::chrono::high_resolution_clock::now();
                    float time            = std::chrono::duration <float, std::chrono::seconds::period> 
                                            (currentTime - startTime).count();  

                    /* Pick a model to run transform operation
                    */
                    uint32_t modelInfoId              = m_modelInfoIdBase + static_cast <uint32_t> 
                                                        (g_pathSettings.models.size()/2);
                    auto modelInfo                    = getModelInfo (modelInfoId);
                    modelInfo->meta.rotateAxis        = {0.0f,  0.0f,  1.0f};
                    modelInfo->meta.rotateAngleDeg    = time * 30.0f;
                    modelInfo->meta.updateModelMatrix = true;
#endif  // ENABLE_IDLE_ROTATION

                    VKDrawSequence::runSequence (m_modelInfoIdBase, 
                                                 m_renderPassInfoId,
                                                 m_pipelineInfoId,
                                                 m_cameraInfoId,
                                                 m_resourceId,
                                                 m_sceneInfoId);
                }
                /* Remember that all of the operations in the above render method are asynchronous. That means that when
                 * we exit the render loop, drawing and presentation operations may still be going on. Cleaning up
                 * resources while that is happening is a bad idea. To fix that problem, we should wait for the logical 
                 * device to finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (deviceInfo->shared.logDevice);
            }

            void deleteScene (void) {
                VKDeleteSequence::runSequence (m_modelInfoIdBase, 
                                               m_renderPassInfoId,
                                               m_pipelineInfoId,
                                               m_cameraInfoId,
                                               m_resourceId,
                                               m_sceneInfoId);
            }
    };
}   // namespace SandBox
#endif  // RD_APPLICATION_H