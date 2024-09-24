#ifndef EN_GRID_H
#define EN_GRID_H

#include "../../Core/Pipeline/VKVertexInput.h"
#include "../../Core/Pipeline/VKShaderStage.h"
#include "../../Core/Pipeline/VKDescriptorSetLayout.h"
#include "../../Core/Pipeline/VKPushConstantRange.h"
#include "../../Core/Pipeline/VKPipelineLayout.h"
#include "../../Core/Cmd/VKCmd.h"
#include "../../Core/Scene/VKCameraMgr.h"
#include "../../Core/Scene/VKSceneMgr.h"
#include "../../Core/Scene/VKUniform.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENGrid: protected virtual Core::VKVertexInput,
                  protected virtual Core::VKShaderStage,
                  protected virtual Core::VKDescriptorSetLayout,
                  protected virtual Core::VKPushConstantRange,
                  protected virtual Core::VKPipelineLayout,
                  protected virtual Core::VKCmd,
                  protected virtual Core::VKCameraMgr,
                  protected virtual Core::VKSceneMgr {
        private:
            Log::Record* m_ENGridLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            ENGrid (void) {
                m_ENGridLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~ENGrid (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void initExtension (uint32_t deviceInfoId,
                                uint32_t renderPassInfoId,
                                uint32_t gridPipelineInfoId,
                                uint32_t pipelineInfoId) {

                derivePipelineInfo (gridPipelineInfoId, pipelineInfoId);
                auto deviceInfo       = getDeviceInfo   (deviceInfoId);
                auto gridPipelineInfo = getPipelineInfo (gridPipelineInfoId);
                auto pipelineInfo     = getPipelineInfo (pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VERTEX INPUT                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto bindingDescriptions   = std::vector <VkVertexInputBindingDescription>   {};
                auto attributeDescriptions = std::vector <VkVertexInputAttributeDescription> {};
                createVertexInputState (gridPipelineInfoId,
                                        bindingDescriptions,
                                        attributeDescriptions);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                gridPipelineInfo->state.stages.clear();
                auto vertexShaderModule   = createShaderStage (deviceInfoId,
                                                               gridPipelineInfoId,
                                                               VK_SHADER_STAGE_VERTEX_BIT,
                                                               g_pipelineSettings.shaderStage.vertexShaderBinaryPath,
                                                               "main");

                auto fragmentShaderModule = createShaderStage (deviceInfoId,
                                                               gridPipelineInfoId,
                                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                                               g_pipelineSettings.shaderStage.fragmentShaderBinaryPath,
                                                               "main");
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SET LAYOUT                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto layoutBindings = std::vector <VkDescriptorSetLayoutBinding> {};
                auto bindingFlags   = std::vector <VkDescriptorBindingFlags>     {};
                createDescriptorSetLayout (deviceInfoId,
                                           gridPipelineInfoId,
                                           layoutBindings,
                                           bindingFlags,
                                           0);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PUSH CONSTANT RANGES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPushConstantRange (gridPipelineInfoId,
                                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                         0,
                                         sizeof (Core::SceneDataVertPC));
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE LAYOUT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPipelineLayout (deviceInfoId, gridPipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Add a pipeline derivative. Note that, we are only allowed to either use a handle or index of the
                 * base pipeline, as we are using the handle, we must set the index to -1. The create derivative bit
                 * specifies that the pipeline to be created will be a child of a previously created parent pipeline
                */
                createGraphicsPipeline (deviceInfoId,
                                        renderPassInfoId,
                                        gridPipelineInfoId,
                                        0, -1,
                                        pipelineInfo->resource.pipeline,
                                        VK_PIPELINE_CREATE_DERIVATIVE_BIT);

                LOG_INFO (m_ENGridLog) << "[OK] Pipeline "
                                       << "[" << gridPipelineInfoId << "]"
                                       << " "
                                       << "[" << renderPassInfoId << "]"
                                       << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SHADER MODULES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                vkDestroyShaderModule (deviceInfo->resource.logDevice, vertexShaderModule,   VK_NULL_HANDLE);
                vkDestroyShaderModule (deviceInfo->resource.logDevice, fragmentShaderModule, VK_NULL_HANDLE);
                LOG_INFO (m_ENGridLog) << "[DELETE] Shader modules"
                                       << std::endl;
            }

            void drawExtension (uint32_t gridPipelineInfoId,
                                uint32_t cameraInfoId,
                                uint32_t sceneInfoId,
                                uint32_t currentFrameInFlight) {

                auto cameraInfo = getCameraInfo (cameraInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);

                Core::SceneDataVertPC sceneData;
                sceneData.viewMatrix       = cameraInfo->transform.viewMatrix;
                sceneData.projectionMatrix = cameraInfo->transform.projectionMatrix;

                bindPipeline        (gridPipelineInfoId,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                updatePushConstants (gridPipelineInfoId,
                                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                     0, sizeof (Core::SceneDataVertPC), &sceneData,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                draw                (6, 1, 0, 0,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);
            }
    };
}   // namespace SandBox
#endif  // EN_GRID_H