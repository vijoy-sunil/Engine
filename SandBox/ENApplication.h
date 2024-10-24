#ifndef EN_APPLICATION_H
#define EN_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"
#include "Extension/ENUI.h"
#include "Extension/ENGrid.h"
#include "Controller/ENGeneric.h"

namespace SandBox {
    class ENApplication: protected Core::VKInitSequence,
                         protected Core::VKDrawSequence,
                         protected Core::VKDeleteSequence,
                         protected ENUI,
                         protected ENGrid,
                         protected ENGeneric {
        private:
            uint32_t m_deviceInfoId;
            std::vector <uint32_t> m_modelInfoIds;
            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_sceneInfoId;
            /* Secondary ids
            */
            uint32_t m_uiRenderPassInfoId;
            uint32_t m_gridPipelineInfoId;
            uint32_t m_uiSceneInfoId;
            /* To use the right objects (command buffers, sync objects etc.) every frame, keep track of the current
             * frame in flight
            */
            uint32_t m_currentFrameInFlight;
            uint32_t m_swapChainImageId;

        public:
            ENApplication (void) {
                m_deviceInfoId         = 0;
                m_renderPassInfoId     = 0;
                m_pipelineInfoId       = 0;
                m_cameraInfoId         = 0;
                m_sceneInfoId          = 0;

                m_uiRenderPassInfoId   = 1;
                m_gridPipelineInfoId   = 1;
                m_uiSceneInfoId        = 1;

                m_currentFrameInFlight = 0;
            }

            ~ENApplication (void) {
            }

            void createScene (void) {
                /* |------------------------------------------------------------------------------------------------|
                 * | READY DEVICE INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyDeviceInfo (m_deviceInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY MODEL INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t totalInstancesCount = 0;
#if ENABLE_SAMPLE_MODELS_IMPORT
                for (auto const& [infoId, info]: g_sampleModelImportInfoPool) {
                    readyModelInfo (infoId,
                                    info.modelPath,
                                    info.mtlFileDirPath);

                    totalInstancesCount += importInstanceData (infoId, info.instanceDataPath);
                    m_modelInfoIds.push_back (infoId);
                }
#else
                for (auto const& [infoId, info]: g_staticModelImportInfoPool) {
                    readyModelInfo (infoId,
                                    info.modelPath,
                                    info.mtlFileDirPath);

                    totalInstancesCount += importInstanceData (infoId, info.instanceDataPath);
                    m_modelInfoIds.push_back (infoId);
                }

                for (auto const& [infoId, info]: g_dynamicModelImportInfoPool) {
                    readyModelInfo (infoId,
                                    info.modelPath,
                                    info.mtlFileDirPath);

                    totalInstancesCount += importInstanceData (infoId, info.instanceDataPath);
                    m_modelInfoIds.push_back (infoId);
                }
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyCameraInfo (m_cameraInfoId);
                auto cameraInfo            = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.upVector  = g_cameraSettings.upVector;
                cameraInfo->meta.nearPlane = g_cameraSettings.nearPlane;
                cameraInfo->meta.farPlane  = g_cameraSettings.farPlane;
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SCENE INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                readySceneInfo (m_sceneInfoId, totalInstancesCount);
                /* |------------------------------------------------------------------------------------------------|
                 * | RUN SEQUENCE - INIT                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKInitSequence::runSequence (m_deviceInfoId,
                                             m_modelInfoIds,
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_sceneInfoId,
                [&](void) {
                {
#if ENABLE_SAMPLE_MODELS_IMPORT
#else
                    /* Update instance textures, this is required when you need instances to have different textures
                     * applied to them compared to the parent instance (model instance id = 0). Note that, the texture
                     * ids to be updated must exist in the global texture pool
                    */
                    for (auto const& modelInstanceId: {1, 2, 3})
                        updateTexIdLUT (T0_GENERIC_NOCAP, modelInstanceId, 5, 4);
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                }
                {
                    ENUI::initExtension     (m_deviceInfoId,
                                             m_uiRenderPassInfoId,
                                             m_uiSceneInfoId,
                                             m_sceneInfoId);

                    auto cameraInfoIds = std::vector <uint32_t> {
                        m_cameraInfoId
                    };
                    readyUI                 (m_deviceInfoId,
                                             m_modelInfoIds,
                                             m_uiRenderPassInfoId,
                                             cameraInfoIds,
                                             m_uiSceneInfoId);
                }
                {
                    ENGrid::initExtension   (m_deviceInfoId,
                                             m_renderPassInfoId,
                                             m_gridPipelineInfoId,
                                             m_pipelineInfoId);
                }
                });
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CONTROLLER                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyGenericController (m_deviceInfoId);
                readyCameraController  (m_deviceInfoId, m_cameraInfoId, STADIUM);
            }

            void runScene (void) {
                auto deviceInfo  = getDeviceInfo (m_deviceInfoId);
                float frameDelta = 0.0f;
                /* |------------------------------------------------------------------------------------------------|
                 * | EVENT LOOP                                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                while (!glfwWindowShouldClose (deviceInfo->resource.window)) {
                    /* GLFW needs to poll the window system for events both to provide input to the application and to
                     * prove to the window system that the application hasn't locked up. Event processing is normally
                     * done each frame after buffer swapping. Even when you have no windows, event polling needs to be
                     * done in order to receive monitor and joystick connection events. glfwPollEvents(), processes only
                     * those events that have already been received and then returns immediately. This is the best choice
                     * when rendering continuously, like most games do
                     *
                     * Note that, if you only need to update the contents of the window when you receive new input,
                     * glfwWaitEvents() is a better choice
                    */
                    glfwPollEvents();
                /* |------------------------------------------------------------------------------------------------|
                 * | MOTION UPDATE                                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                    auto startOfFrameTime = std::chrono::high_resolution_clock::now();
                    handleKeyEvents   (startOfFrameTime);
                    /* [ X ] Update vehicle state here before camera state so that the model matrix is ready to be
                     * used by camera vectors in the same frame
                    */

#if ENABLE_SAMPLE_MODELS_IMPORT
                    updateCameraState (SAMPLE_1, 0);
#else
                    updateCameraState (VEHICLE_BASE, 0);
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                /* |------------------------------------------------------------------------------------------------|
                 * | RUN SEQUENCE - DRAW                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    VKDrawSequence::runSequence     (m_deviceInfoId,
                                                     m_modelInfoIds,
                                                     m_renderPassInfoId,
                                                     m_pipelineInfoId,
                                                     m_cameraInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight,
                                                     m_swapChainImageId,
                    [&](void) {
                    {   /* Extension to base render pass */
                        ENGrid::drawExtension       (m_gridPipelineInfoId,
                                                     m_cameraInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight);
                    }},
                    [&](void) {
                    {   /* Extension to secondary render pass, base command buffer */
                        ENUI::drawExtension         (m_deviceInfoId,
                                                     m_uiRenderPassInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight,
                                                     m_swapChainImageId,
                                                     frameDelta);
                    }},
                    [&](void) {
                    {   /* Extension to recreate swap chain deps */
                        ENUI::recreateSwapChainDeps (m_deviceInfoId,
                                                     m_uiRenderPassInfoId,
                                                     m_sceneInfoId);
                    }
                    });
                /* |------------------------------------------------------------------------------------------------|
                 * | FRAME DELTA                                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                    auto endOfFrameTime = std::chrono::high_resolution_clock::now();
                    frameDelta          = std::chrono::duration <float, std::chrono::seconds::period>
                                          (endOfFrameTime - startOfFrameTime).count();
                }
                /* Remember that all of the operations in the above render method are asynchronous. That means that when
                 * we exit the render loop, drawing and presentation operations may still be going on. Cleaning up
                 * resources while that is happening is a bad idea. To fix that problem, we should wait for the logical
                 * device to finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (deviceInfo->resource.logDevice);
            }

            void deleteScene (void) {
                /* |------------------------------------------------------------------------------------------------|
                 * | RUN SEQUENCE - DELETE                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto renderPassInfoIds = std::vector <uint32_t> {
                    m_renderPassInfoId,
                    m_uiRenderPassInfoId
                };
                auto pipelineInfoIds   = std::vector <uint32_t> {
                    m_pipelineInfoId,
                    m_gridPipelineInfoId
                };
                VKDeleteSequence::runSequence   (m_deviceInfoId,
                                                 m_modelInfoIds,
                                                 renderPassInfoIds,
                                                 pipelineInfoIds,
                                                 m_cameraInfoId,
                                                 m_sceneInfoId,
                [&](void) {
                {
                    UIImpl::cleanUp             (m_deviceInfoId);
                    ENUI::deleteExtension       (m_deviceInfoId,
                                                 m_uiSceneInfoId);
                }
                });
            }
    };
}   // namespace SandBox
#endif  // EN_APPLICATION_H