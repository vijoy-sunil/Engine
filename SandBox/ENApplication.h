#ifndef EN_APPLICATION_H
#define EN_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"
#include "Configs/ENModel.h"
#include "Configs/ENEnvironment.h"

using namespace Core;

namespace SandBox {
    class ENApplication: protected VKInitSequence,
                         protected VKDrawSequence,
                         protected VKDeleteSequence {
        private:
            uint32_t m_deviceInfoId;
            std::vector <uint32_t> m_modelInfoIds;
            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_inFlightFenceInfoBase;
            uint32_t m_imageAvailableSemaphoreInfoBase;
            uint32_t m_renderDoneSemaphoreInfoBase;
            uint32_t m_sceneInfoId;
            /* To use the right objects (command buffers, sync objects etc.) every frame, keep track of the current 
             * frame in flight
            */
            uint32_t m_currentFrameInFlight;

        public:
            ENApplication (void) {
                m_deviceInfoId                    = 0;
                m_renderPassInfoId                = 0;
                m_pipelineInfoId                  = 0;
                m_cameraInfoId                    = 0;
                m_inFlightFenceInfoBase           = 0;
                m_imageAvailableSemaphoreInfoBase = 0;
                m_renderDoneSemaphoreInfoBase     = 0;
                m_sceneInfoId                     = 0;
                m_currentFrameInFlight            = 0;
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
                auto cameraInfo = getCameraInfo (m_cameraInfoId);

                cameraInfo->meta.position  = {0.0f, -80.0f,  0.0f};
                cameraInfo->meta.center    = {0.0f,   0.0f, 10.0f};
                cameraInfo->meta.upVector  = {0.0f,   0.0f,  1.0f};
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

                VKInitSequence::runSequence (m_deviceInfoId, 
                                             m_modelInfoIds, 
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_cameraInfoId,
                                             m_sceneInfoId,
                                             [&](void) {
                /* |------------------------------------------------------------------------------------------------|
                 * | EDIT CONFIGS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                {
#if ENABLE_SAMPLE_MODELS_IMPORT
#else
                    /* Update instance textures, this is required when you need instances to have different textures
                     * applied to them compared to the parent instance (model instance id = 0). Note that, the texture
                     * ids to be updated must exist in the global texture pool
                    */
                    for (auto const& modelInstanceId: {1, 2, 3})
                        updateTexIdLUT (4, modelInstanceId, 3, 5);
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                }
                {
                    /* Add a pipeline derivative. Note that, we are only allowed to either use a handle or index of the 
                     * base pipeline, as we are using the handle, we must set the index to -1. The create derivative bit
                     * specifies that the pipeline to be created will be a child of a previously created parent pipeline
                    */
                    uint32_t gridPipelineInfoId = m_pipelineInfoId + 1;
                    readyPipelineInfo  (gridPipelineInfoId);
                    derivePipelineInfo (gridPipelineInfoId, m_pipelineInfoId);

                    auto deviceInfo       = getDeviceInfo   (m_deviceInfoId);
                    auto gridPipelineInfo = getPipelineInfo (gridPipelineInfoId);
                    auto basePipelineInfo = getPipelineInfo (m_pipelineInfoId);

                    /* Add/edit configs that are missing/different from base pipeline
                    */
                    auto bindingDescriptions   = std::vector <VkVertexInputBindingDescription>   {};
                    auto attributeDescriptions = std::vector <VkVertexInputAttributeDescription> {};
                    createVertexInputState    (gridPipelineInfoId, 
                                               bindingDescriptions, 
                                               attributeDescriptions);

                    gridPipelineInfo->state.stages.clear();
                    auto vertexShaderModule   = createShaderStage (m_deviceInfoId, 
                                                                   gridPipelineInfoId,
                                                                   VK_SHADER_STAGE_VERTEX_BIT, 
                                                                   g_gridSettings.vertexShaderBinaryPath, 
                                                                   "main");

                    auto fragmentShaderModule = createShaderStage (m_deviceInfoId, 
                                                                   gridPipelineInfoId,
                                                                   VK_SHADER_STAGE_FRAGMENT_BIT, 
                                                                   g_gridSettings.fragmentShaderBinaryPath,
                                                                   "main");

                    auto layoutBindings = std::vector <VkDescriptorSetLayoutBinding> {};
                    auto bindingFlags   = std::vector <VkDescriptorBindingFlags>     {};
                    createDescriptorSetLayout (m_deviceInfoId, 
                                               gridPipelineInfoId, 
                                               layoutBindings, 
                                               bindingFlags, 
                                               0);

                    createPushConstantRange   (gridPipelineInfoId, 
                                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                               0, 
                                               sizeof (SceneDataVertPC));

                    createPipelineLayout      (m_deviceInfoId, gridPipelineInfoId);

                    createGraphicsPipeline    (m_deviceInfoId,
                                               m_renderPassInfoId,
                                               gridPipelineInfoId,
                                               0, -1,
                                               basePipelineInfo->resource.pipeline, 
                                               VK_PIPELINE_CREATE_DERIVATIVE_BIT);

                    vkDestroyShaderModule (deviceInfo->resource.logDevice, vertexShaderModule,   VK_NULL_HANDLE);
                    vkDestroyShaderModule (deviceInfo->resource.logDevice, fragmentShaderModule, VK_NULL_HANDLE);
                }});
            }

            void runScene (void) {
                auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | EVENT LOOP                                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                while (!glfwWindowShouldClose (deviceInfo->resource.window)) {
                    glfwPollEvents();

                    VKDrawSequence::runSequence (m_deviceInfoId,
                                                 m_modelInfoIds, 
                                                 m_renderPassInfoId,
                                                 m_pipelineInfoId,
                                                 m_cameraInfoId,
                                                 m_sceneInfoId,
                                                 m_currentFrameInFlight,
                                                 [&](void) {
                /* |------------------------------------------------------------------------------------------------|
                 * | EDIT CONFIGS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                    {
                        auto cameraInfo = getCameraInfo (m_cameraInfoId);
                        auto sceneInfo  = getSceneInfo  (m_sceneInfoId);
                        
                        uint32_t gridPipelineInfoId = m_pipelineInfoId + 1;
                        bindPipeline        (gridPipelineInfoId,
                                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                                             sceneInfo->resource.commandBuffers[m_currentFrameInFlight]);

                        SceneDataVertPC sceneData;
                        sceneData.viewMatrix       = cameraInfo->transform.viewMatrix;
                        sceneData.projectionMatrix = cameraInfo->transform.projectionMatrix;

                        updatePushConstants (gridPipelineInfoId,
                                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             0, sizeof (SceneDataVertPC), &sceneData,
                                             sceneInfo->resource.commandBuffers[m_currentFrameInFlight]);

                        draw (6, 1, 0, 0, sceneInfo->resource.commandBuffers[m_currentFrameInFlight]);
                    }});
                }
                /* Remember that all of the operations in the above render method are asynchronous. That means that when
                 * we exit the render loop, drawing and presentation operations may still be going on. Cleaning up
                 * resources while that is happening is a bad idea. To fix that problem, we should wait for the logical 
                 * device to finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (deviceInfo->resource.logDevice);
            }

            void deleteScene (void) {
                auto pipelineInfoIds = std::vector <uint32_t> {
                    m_pipelineInfoId,   /* Base pipeline */
                    1                   /* Grid pipeline */
                };

                VKDeleteSequence::runSequence (m_deviceInfoId,
                                               m_modelInfoIds, 
                                               m_renderPassInfoId,
                                               pipelineInfoIds,
                                               m_cameraInfoId,
                                               m_sceneInfoId);
            }
    };
}   // namespace SandBox
#endif  // EN_APPLICATION_H