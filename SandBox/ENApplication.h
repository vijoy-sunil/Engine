#ifndef EN_APPLICATION_H
#define EN_APPLICATION_H

#include "../Core/Model/VKInstanceData.h"
#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"
#include "ENConfig.h"

using namespace Core;

namespace SandBox {
    class ENApplication: protected VKInstanceData,
                         protected VKInitSequence,
                         protected VKDrawSequence,
                         protected VKDeleteSequence {
        private:
            std::vector <uint32_t> m_modelInfoIds;
            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_inFlightFenceInfoBase;
            uint32_t m_imageAvailableSemaphoreInfoBase;
            uint32_t m_renderDoneSemaphoreInfoBase;
            uint32_t m_resourceId;
            uint32_t m_sceneInfoId;
            uint32_t m_deviceResourcesCount;

        public:
            ENApplication (void) {
                /* Info id overview
                 * |------------------------|-------------------|---------------|---------------|---------------|
                 * | MODEL INFO ID          |   0               |   1           |   2           |   3           |
                 * |------------------------|-------------------|---------------|---------------|---------------|
                 * | STAGING_BUFFER_TEX     |   0, 1, 2, 3, 4                                                   |
                 * |------------------------|-------------------------------------------------------------------|
                 * | SWAPCHAIN_IMAGE        |   0, 1, 2, ...                                                    |
                 * |------------------------|-------------------------------------------------------------------|
                 * | TEXTURE_IMAGE          |   0, 1, 2, 3, 4                                                   |
                 * |------------------------|-------------------------------------------------------------------|
                 * | DEPTH_IMAGE            |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | MULTISAMPLE_IMAGE      |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * |                                                                                            |
                 * |------------------------|-------------------------------------------------------------------|
                 * | STAGING_BUFFER         |   0, 1                                                            |
                 * |------------------------|-------------------|---------------|-------------------------------|
                 * | VERTEX_BUFFER          |   0               |   UINT32_MAX  |   UINT32_MAX  |   UINT32_MAX  |
                 * |------------------------|-------------------|---------------|---------------|---------------|
                 * | INDEX_BUFFER           |   1               |   UINT32_MAX  |   UINT32_MAX  |   UINT32_MAX  |
                 * |------------------------|-------------------|---------------|---------------|---------------|
                 * | UNIFORM_BUFFER         |                                                                   |
                 * |------------------------|-------------------------------------------------------------------|
                 * | STORAGE_BUFFER         |   0, 1, ...                                                       |
                 * |------------------------|-------------------------------------------------------------------|
                 * |                                                                                            |
                 * |------------------------|-------------------------------------------------------------------|
                 * | RENDER PASS INFO ID    |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | PIPELINE INFO ID       |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | CAMERA INFO ID         |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | FEN_TRANSFER_DONE      |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | FEN_BLIT_DONE          |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | FEN_IN_FLIGHT          |   0, 1, ...                                                       |
                 * |------------------------|-------------------------------------------------------------------|
                 * | SEM_IMAGE_AVAILABLE    |   0, 1, ...                                                       |
                 * |------------------------|-------------------------------------------------------------------|
                 * | SEM_RENDER_DONE        |   0, 1, ...                                                       |
                 * |------------------------|-------------------------------------------------------------------|
                 * | RESOURCE ID            |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                 * | SCENE INFO ID          |   0                                                               |
                 * |------------------------|-------------------------------------------------------------------|
                */
                m_renderPassInfoId                = 0;
                m_pipelineInfoId                  = 0;
                m_cameraInfoId                    = 0;
                m_inFlightFenceInfoBase           = 0;
                m_imageAvailableSemaphoreInfoBase = 0;
                m_renderDoneSemaphoreInfoBase     = 0;
                m_resourceId                      = 0;
                m_sceneInfoId                     = 0;
                m_deviceResourcesCount            = 1;
            }

            ~ENApplication (void) {
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
                uint32_t totalInstancesCount = 0;
                for (auto const& [infoId, info]: g_modelImportInfoPool) {
                    readyModelInfo (infoId, 
                                    info.modelPath,
                                    info.mtlFileDirPath);

                    totalInstancesCount += importInstanceData (infoId, info.instanceDataPath);
                    m_modelInfoIds.push_back (infoId);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA INFO                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyCameraInfo (m_cameraInfoId);
                auto cameraInfo = getCameraInfo (m_cameraInfoId);

                cameraInfo->meta.position  = {0.0f, -2.0f, 1.0f};
                cameraInfo->meta.center    = {0.0f, -1.5f, 4.0f};
                cameraInfo->meta.upVector  = {0.0f, -1.0f, 0.0f};
                cameraInfo->meta.fovDeg    = 45.0f;
                cameraInfo->meta.nearPlane = 0.01f;
                cameraInfo->meta.farPlane  = 1000.0f;
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SCENE INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto sceneInfoIds = std::vector {
                    m_inFlightFenceInfoBase,
                    m_imageAvailableSemaphoreInfoBase,
                    m_renderDoneSemaphoreInfoBase
                };
                readySceneInfo (m_sceneInfoId, totalInstancesCount, sceneInfoIds);

                VKInitSequence::runSequence (m_modelInfoIds, 
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

                    VKDrawSequence::runSequence (m_modelInfoIds, 
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
                VKDeleteSequence::runSequence (m_modelInfoIds, 
                                               m_renderPassInfoId,
                                               m_pipelineInfoId,
                                               m_cameraInfoId,
                                               m_resourceId,
                                               m_sceneInfoId);
            }
    };
}   // namespace SandBox
#endif  // EN_APPLICATION_H