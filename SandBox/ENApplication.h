#ifndef EN_APPLICATION_H
#define EN_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"
#include "Extension/ENAnchor.h"
#include "Extension/ENSkyBox.h"
#include "Extension/ENGrid.h"
#include "Extension/ENUI.h"
#include "Controller/ENGeneric.h"

namespace SandBox {
    class ENApplication: protected Core::VKInitSequence,
                         protected Core::VKDrawSequence,
                         protected Core::VKDeleteSequence,
                         protected ENAnchor,
                         protected ENSkyBox,
                         protected ENGrid,
                         protected ENUI,
                         protected ENGeneric {
        private:
            uint32_t m_deviceInfoId;
            /* Anchors are a special type of models and are used to visualize model-less instances such as lights and 
             * cameras by copying their properties like pose, color, etc. Note that, the parent info ids for the 
             * aforementioned model-less instances will be same as their corresponding anchor info ids
            */
            std::vector <uint32_t> m_anchorInfoIds;
            std::vector <uint32_t> m_modelInfoIds;

            uint32_t m_renderPassInfoId;
            uint32_t m_uiRenderPassInfoId;

            uint32_t m_pipelineInfoId;
            uint32_t m_anchorPipelineInfoId;
            uint32_t m_skyBoxPipelineInfoId;
            uint32_t m_gridPipelineInfoId;

            std::vector <uint32_t> m_cameraInfoIds;
            uint32_t m_activeCameraInfoId;

            uint32_t m_sceneInfoId;
            uint32_t m_anchorSceneInfoId;
            uint32_t m_skyBoxSceneInfoId;
            uint32_t m_uiSceneInfoId;

            std::vector <uint32_t> m_lightInfoIds;
            /* To use the right objects (command buffers, sync objects etc.) every frame, keep track of the current
             * frame in flight
            */
            uint32_t m_currentFrameInFlight;
            uint32_t m_swapChainImageId;

        public:
            ENApplication (void) {
                m_deviceInfoId         = 0;

                m_renderPassInfoId     = 0;
                m_uiRenderPassInfoId   = 1;

                m_pipelineInfoId       = 0;
                m_anchorPipelineInfoId = 1;
                m_skyBoxPipelineInfoId = 2;
                m_gridPipelineInfoId   = 3;
                /* Note that, the default active camera info id will be set to the first camera anchor instance id, which
                 * will be instance id #0
                */
                m_activeCameraInfoId   = 0;

                m_sceneInfoId          = 0;
                m_anchorSceneInfoId    = 1;
                m_skyBoxSceneInfoId    = 2;
                m_uiSceneInfoId        = 3;

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
                 * | READY ANCHOR INFO - CAMERA                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto anchorImportInfo = g_cameraAnchorImportInfoPool[ANCHOR_CAMERA];
                readyModelInfo      (ANCHOR_CAMERA,
                                     anchorImportInfo.modelPath,
                                     anchorImportInfo.mtlFileDirPath);

                importTransformData (ANCHOR_CAMERA, anchorImportInfo.transformDataPath);
                m_anchorInfoIds.push_back (ANCHOR_CAMERA);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY ANCHOR INFO - LIGHT                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& [infoId, info]: g_lightAnchorImportInfoPool) {
                    readyModelInfo  (infoId,
                                     info.modelPath,
                                     info.mtlFileDirPath);

                    importTransformData (infoId, info.transformDataPath);
                    m_anchorInfoIds.push_back (infoId);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY MODEL INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& [infoId, info]: g_trackModelImportInfoPool) {
                    readyModelInfo (infoId,
                                    info.modelPath,
                                    info.mtlFileDirPath);

                    importTransformData (infoId, info.transformDataPath);
                    m_modelInfoIds.push_back (infoId);
                }
                /* Note that, texture shininess values need to be set before importing models, so that they can be
                 * added as a vertex attribute
                */
                for (auto const& [path, val]: g_shininessImportPool)
                    setShininess (path, val);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY MODEL INFO - SKY BOX                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto modelImportInfo = g_skyBoxModelImportInfoPool[SKY_BOX];
                readyModelInfo      (SKY_BOX,
                                     modelImportInfo.modelPath,
                                     modelImportInfo.mtlFileDirPath);
                importTransformData (SKY_BOX, modelImportInfo.transformDataPath);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto anchorInfo = getModelInfo (ANCHOR_CAMERA);
                for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {
                    readyCameraInfo (i);
                    auto cameraInfo            = getCameraInfo (i);
                    cameraInfo->meta.upVector  = g_cameraSettings.upVector;
                    cameraInfo->meta.nearPlane = g_cameraSettings.nearPlane;
                    cameraInfo->meta.farPlane  = g_cameraSettings.farPlane;

                    m_cameraInfoIds.push_back (i);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SCENE INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                readySceneInfo (m_sceneInfoId,
                                0,                   /* Swap chain image info id base           */
                                0,                   /* Depth image info id                     */
                                0,                   /* Multi sample image info id              */
                                UINT32_MAX,          /* Uniform buffer info id base             */
                                0,                   /* Model storage buffer info id base       */
                                2,                   /* Light storage buffer info id base       */
                                0,                   /* In flight fence info id base            */
                                0,                   /* Image available semaphore info id base  */
                                0);                  /* Render done semaphore info id base      */
                readySceneInfo (m_skyBoxSceneInfoId,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                0,                   /* Uniform buffer info id base             */
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX);
                readySceneInfo (m_anchorSceneInfoId,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                4,                   /* Model storage buffer info id base       */
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX,
                                UINT32_MAX);
                readySceneInfo (m_uiSceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY LIGHT INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& [infoId, info]: g_lightAnchorImportInfoPool) {
                    readyLightInfo (infoId);
                    auto lightInfo          = getLightInfo (infoId);
                    auto anchorInfo         = getModelInfo (infoId);
                    uint32_t instancesCount = anchorInfo->meta.instancesCount;

                    lightInfo->meta.instances.resize (instancesCount);
                    lightInfo->meta.instancesCount =  instancesCount;

                    for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {

                        glm::vec3 position         = anchorInfo->meta.transformDatas[i].position;
                        glm::vec3 rotateAngleDeg   = anchorInfo->meta.transformDatas[i].rotateAngleDeg;
                        float yawDeg               = -rotateAngleDeg.y;
                        float pitchDeg             = -rotateAngleDeg.x;
                        glm::vec3 direction        = getDirectionVector (yawDeg, pitchDeg);

                        lightInfo->meta.instances[i].position  = position;
                        lightInfo->meta.instances[i].direction = direction;
                    }
                    /* Note that, ordering of light info ids is important, and is as show below
                     * |--------------------|--------------------|--------------------|
                     * |    Directional     |       Point        |       Spot         |
                     * |--------------------|--------------------|--------------------|
                     * |   instance ids     |   instance ids     |   instance ids     |
                     * | 0, 1, 2, ...       | 0, 1, 2, ...       | 0, 1, 2, ...       |
                     * |--------------------|--------------------|--------------------|
                    */
                    m_lightInfoIds.push_back (infoId);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | RUN SEQUENCE - INIT                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKInitSequence::runSequence (m_deviceInfoId,
                                             m_modelInfoIds,
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_sceneInfoId,
                                             m_lightInfoIds,
                [&](void) {
                {
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION INIT - SKY BOX                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                    uint32_t skyBoxImageInfoId = ENSkyBox::initExtension (m_deviceInfoId,
                                                                          SKY_BOX,
                                                                          m_renderPassInfoId,
                                                                          m_skyBoxPipelineInfoId,
                                                                          m_pipelineInfoId,
                                                                          m_skyBoxSceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION INIT - ANCHOR                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                    ENAnchor::initExtension (m_deviceInfoId,
                                             m_anchorInfoIds,
                                             m_renderPassInfoId,
                                             m_anchorPipelineInfoId,
                                             m_pipelineInfoId,
                                             m_anchorSceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION INIT - GRID                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                    ENGrid::initExtension   (m_deviceInfoId,
                                             m_renderPassInfoId,
                                             m_gridPipelineInfoId,
                                             m_pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION INIT - UI                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    /* Populate texture image pools with textures that will be used in ui. Note that, info ids map to
                     * vector of paths because images are assumed to have multiple layers
                    */
                    std::unordered_map <Core::e_textureType,
                    std::unordered_map <uint32_t, std::vector <std::string>>> uiTextureImagePools;

                    for (auto const& [path, infoId]: getTextureImagePool (Core::DIFFUSE_TEXTURE))
                        uiTextureImagePools[Core::DIFFUSE_TEXTURE][infoId]. push_back (path);

                    for (auto const& [path, infoId]: getTextureImagePool (Core::SPECULAR_TEXTURE))
                        uiTextureImagePools[Core::SPECULAR_TEXTURE][infoId].push_back (path);

                    for (auto const& [path, infoId]: getTextureImagePool (Core::EMISSION_TEXTURE))
                        uiTextureImagePools[Core::EMISSION_TEXTURE][infoId].push_back (path);

                    for (auto const& [target, path]: g_skyBoxTextureImagePool)
                        uiTextureImagePools[Core::DIFFUSE_TEXTURE][skyBoxImageInfoId].push_back (path);

                    /* Calculate number of image textures that will be used in ui, this will be used to create the
                     * descriptor pool from which the descriptor sets will be allocated
                    */
                    uint32_t uiTextureCount = 0;
                    for (auto const& [type, pool]: uiTextureImagePools) {
                        for (auto const& [infoId, paths]: pool)
                            uiTextureCount += static_cast <uint32_t> (paths.size());
                    }

                    ENUI::initExtension     (m_deviceInfoId,
                                             m_uiRenderPassInfoId,
                                             m_uiSceneInfoId,
                                             m_sceneInfoId,
                                             uiTextureCount);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY UI                                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                    auto modelInfoIds = m_modelInfoIds;
                    modelInfoIds.push_back (
                        SKY_BOX
                    );

                    auto uiViewInfo                         = getUIViewInfo();
                    uiViewInfo->cameraFocus.modelInfoId     = CYLINDER;
                    uiViewInfo->cameraFocus.modelInstanceId = 1;
                    uiViewInfo->activeCameraInfoId          = m_activeCameraInfoId;

                    readyUI (m_deviceInfoId,
                             modelInfoIds,
                             ANCHOR_CAMERA,
                             m_uiRenderPassInfoId,
                             m_uiSceneInfoId,
                             m_lightInfoIds,
                             uiTextureImagePools);
                }
                {
                    /* Overwrite default anchor colors
                    */
                    for (auto const& infoId: m_lightInfoIds) {
                        auto lightInfo = getLightInfo (infoId);

                        for (uint32_t i = 0; i < lightInfo->meta.instancesCount; i++) {
                            glm::vec4 diffuse = lightInfo->meta.instances[i].diffuse;

                            updateTexIdLUT (infoId, i, Core::DIFFUSE_TEXTURE, 0,  diffuse.x * UINT8_MAX);
                            updateTexIdLUT (infoId, i, Core::DIFFUSE_TEXTURE, 4,  diffuse.y * UINT8_MAX);
                            updateTexIdLUT (infoId, i, Core::DIFFUSE_TEXTURE, 8,  diffuse.z * UINT8_MAX);
                            updateTexIdLUT (infoId, i, Core::DIFFUSE_TEXTURE, 12, diffuse.w * UINT8_MAX);
                        }
                    }

                    for (auto const& anchorInstanceId: m_cameraInfoIds) {
                        updateTexIdLUT (ANCHOR_CAMERA, anchorInstanceId, Core::DIFFUSE_TEXTURE, 0,  0);
                        updateTexIdLUT (ANCHOR_CAMERA, anchorInstanceId, Core::DIFFUSE_TEXTURE, 4,  UINT8_MAX);
                        updateTexIdLUT (ANCHOR_CAMERA, anchorInstanceId, Core::DIFFUSE_TEXTURE, 8,  0);
                        updateTexIdLUT (ANCHOR_CAMERA, anchorInstanceId, Core::DIFFUSE_TEXTURE, 12, UINT8_MAX);
                    }
                }
                });
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CONTROLLER                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyGenericController (m_deviceInfoId);
                readyCameraController  (m_deviceInfoId);
                /* Note that, it is important to set a camera instance active and an intital camera type to avoid using
                 * an undefined camera type. (It is possible to switch out of this undefined state by switching to a
                 * valid non-drone camera type)
                */
                setCameraActive        (m_activeCameraInfoId, SPOILER);
            }

            void runScene (void) {
                auto deviceInfo   = getDeviceInfo (m_deviceInfoId);
                float elapsedTime = 0.0f;
                float frameDelta  = 0.0f;
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

                    static auto startOfRenderTime = std::chrono::high_resolution_clock::now();
                    auto startOfFrameTime         = std::chrono::high_resolution_clock::now();
                    elapsedTime                   = std::chrono::duration <float, std::chrono::seconds::period>
                                                    (startOfFrameTime - startOfRenderTime).count();

                    handleKeyEvents (startOfFrameTime);
                    /* Note that, the ordering of some/all of the code blocks below are important!
                    */
                    {   /* [ X ] Translation test */
                        auto modelInfo            = getModelInfo (CYLINDER);
                        uint32_t modelInstanceId  = 3;
                        auto& position            = modelInfo->meta.transformDatas[modelInstanceId].position;

                        position                 += glm::vec3 (0.0f, 0.0f, 0.01f);
                        createModelMatrix (CYLINDER, modelInstanceId);
                    }
                    {   /* Sky box rotation */
                        auto modelInfo            = getModelInfo (SKY_BOX);
                        uint32_t modelInstanceId  = 0;
                        auto& rotateAngleDeg      = modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg;

                        rotateAngleDeg            = glm::vec3 (0.0f, elapsedTime * 1.0f, 0.0f);
                        createModelMatrix (SKY_BOX, modelInstanceId);
                    }
                    {   /* Update active camera */
                        auto uiViewInfo           = getUIViewInfo();
                        m_activeCameraInfoId      = uiViewInfo->activeCameraInfoId;
                    }
                    {   /* Camera focus */
                        auto uiViewInfo           = getUIViewInfo();
                        setCameraFocus (uiViewInfo->cameraFocus.modelInfoId,
                                        uiViewInfo->cameraFocus.modelInstanceId);
                    }
                    {   /* Active camera anchor */
                        auto anchorInfo           = getModelInfo (ANCHOR_CAMERA);
                        uint32_t anchorInstanceId = m_activeCameraInfoId;
                        auto& position            = anchorInfo->meta.transformDatas[anchorInstanceId].position;
                        auto& rotateAngleDeg      = anchorInfo->meta.transformDatas[anchorInstanceId].rotateAngleDeg;

                        auto cameraInfo           = getCameraInfo  (anchorInstanceId);
                        glm::vec3 direction       = glm::normalize (cameraInfo->meta.direction);
                        float yawDeg              = -getYawDeg     (direction);
                        float pitchDeg            = -getPitchDeg   (direction);

                        position                  = cameraInfo->meta.position;
                        rotateAngleDeg            = glm::vec3 (pitchDeg, yawDeg, 0.0f);
                        createModelMatrix (ANCHOR_CAMERA, anchorInstanceId);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RUN SEQUENCE - DRAW                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    VKDrawSequence::runSequence     (m_deviceInfoId,
                                                     m_modelInfoIds,
                                                     m_renderPassInfoId,
                                                     m_pipelineInfoId,
                                                     m_activeCameraInfoId,
                                                     m_sceneInfoId,
                                                     m_lightInfoIds,
                                                     m_currentFrameInFlight,
                                                     m_swapChainImageId,
                    [&](void) {
                    {   /* Extension to base render pass */
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION DRAW - SKY BOX                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                        ENSkyBox::drawExtension     (SKY_BOX,
                                                     m_skyBoxPipelineInfoId,
                                                     m_activeCameraInfoId,
                                                     m_skyBoxSceneInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight);
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION DRAW - ANCHOR                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                        ENAnchor::drawExtension     (m_anchorInfoIds,
                                                     m_anchorPipelineInfoId,
                                                     m_activeCameraInfoId,
                                                     m_anchorSceneInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight);
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION DRAW - GRID                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                        ENGrid::drawExtension       (m_gridPipelineInfoId,
                                                     m_activeCameraInfoId,
                                                     m_sceneInfoId,
                                                     m_currentFrameInFlight);
                    }},
                    [&](void) {
                    {   /* Extension to secondary render pass, base command buffer */
                /* |------------------------------------------------------------------------------------------------|
                 * | EXTENSION DRAW - UI                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
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
                auto modelInfoIds      = m_modelInfoIds;
                modelInfoIds.push_back     (SKY_BOX);
                for (auto const& infoId: m_anchorInfoIds)
                    modelInfoIds.push_back (infoId);

                auto renderPassInfoIds = std::vector <uint32_t> {
                    m_renderPassInfoId,
                    m_uiRenderPassInfoId
                };
                auto pipelineInfoIds   = std::vector <uint32_t> {
                    m_pipelineInfoId,
                    m_anchorPipelineInfoId,
                    m_skyBoxPipelineInfoId,
                    m_gridPipelineInfoId
                };
                auto sceneInfoIds      = std::vector <uint32_t> {
                    m_sceneInfoId,
                    m_anchorSceneInfoId,
                    m_skyBoxSceneInfoId,
                    m_uiSceneInfoId
                };
                VKDeleteSequence::runSequence   (m_deviceInfoId,
                                                 modelInfoIds,
                                                 renderPassInfoIds,
                                                 pipelineInfoIds,
                                                 m_cameraInfoIds,
                                                 sceneInfoIds,
                                                 m_lightInfoIds,
                [&](void) {
                {
                    ENSkyBox::deleteExtension   (m_deviceInfoId);
                    UIImpl::cleanUp             (m_deviceInfoId);
                }
                });
            }
    };
}   // namespace SandBox
#endif  // EN_APPLICATION_H