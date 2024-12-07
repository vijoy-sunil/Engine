#ifndef EN_ANCHOR_H
#define EN_ANCHOR_H

#include "../../Core/Model/VKInstanceData.h"
#include "../../Core/Buffer/VKVertexBuffer.h"
#include "../../Core/Buffer/VKIndexBuffer.h"
#include "../../Core/Buffer/VKStorageBuffer.h"
#include "../../Core/Pipeline/VKVertexInput.h"
#include "../../Core/Pipeline/VKShaderStage.h"
#include "../../Core/Pipeline/VKRasterization.h"
#include "../../Core/Pipeline/VKDescriptorSetLayout.h"
#include "../../Core/Pipeline/VKPushConstantRange.h"
#include "../../Core/Pipeline/VKPipelineLayout.h"
#include "../../Core/Cmd/VKCmd.h"
#include "../../Core/Scene/VKCameraMgr.h"
#include "../../Core/Scene/VKDescriptor.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENAnchor: protected virtual Core::VKInstanceData,
                    protected virtual Core::VKVertexBuffer,
                    protected virtual Core::VKIndexBuffer,
                    protected virtual Core::VKStorageBuffer,
                    protected virtual Core::VKVertexInput,
                    protected virtual Core::VKShaderStage,
                    protected virtual Core::VKRasterization,
                    protected virtual Core::VKDescriptorSetLayout,
                    protected virtual Core::VKPushConstantRange,
                    protected virtual Core::VKPipelineLayout,
                    protected virtual Core::VKCmd,
                    protected virtual Core::VKCameraMgr,
                    protected virtual Core::VKDescriptor {
        private:
            Log::Record* m_ENAnchorLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            ENAnchor (void) {
                m_ENAnchorLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~ENAnchor (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void initExtension (uint32_t deviceInfoId,
                                const std::vector <uint32_t>& anchorInfoIds,
                                uint32_t renderPassInfoId,
                                uint32_t anchorPipelineInfoId,
                                uint32_t pipelineInfoId,
                                uint32_t anchorSceneInfoId) {

                auto deviceInfo      = getDeviceInfo   (deviceInfoId);
                auto anchorInfoBase  = getModelInfo    (anchorInfoIds[0]);
                auto pipelineInfo    = getPipelineInfo (pipelineInfoId);
                auto anchorSceneInfo = getSceneInfo    (anchorSceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | IMPORT MODEL                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that, we are not populating/updating the texture image info id look up table since we
                 * won't be needing any textures for anchors and its instances. Instead, we will use the table to store
                 * the color for the anchor. Although it may seem inefficient to use just 4 bytes (R, G, B, A) in
                 * the table, it is easier to populate the combined instances vector in the draw sequence
                */
                for (auto const& infoId: anchorInfoIds) {
                    importOBJModel (infoId);

                    auto anchorInfo = getModelInfo (infoId);
                    for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {
                        /* We will use the texture image info id look up table to store the color information as shown
                         * below
                         * |--------|--------|--------|--------|--------|--------|
                         * |   32b  |   32b  |   32b  |   32b  |    X   |    X   | .....
                         * |--------|--------|--------|--------|--------|--------|
                         *      |        |        |        |
                         *      |        |        |        |
                         *      v        v        v        v
                         * |--------|--------|--------|--------|
                         * |    R   |    G   |    B   |    A   |
                         * |--------|--------|--------|--------|
                        */
                        updateTexIdLUT (infoId, i, 0,  255);    /* R */
                        updateTexIdLUT (infoId, i, 4,  255);    /* G */
                        updateTexIdLUT (infoId, i, 8,  255);    /* B */
                        updateTexIdLUT (infoId, i, 12, 255);    /* A */
                    }
                    LOG_INFO (m_ENAnchorLog) << "[OK] Import model "
                                             << "[" << infoId << "]"
                                             << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG VERTEX BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                std::vector <glm::vec3> combinedVertices;
                size_t combinedVerticesCount = 0;
                uint32_t vertexBufferInfoId  = getNextInfoIdFromBufferType (Core::STAGING_BUFFER);

                for (auto const& infoId: anchorInfoIds) {
                    auto anchorInfo        = getModelInfo (infoId);
                    combinedVerticesCount += anchorInfo->meta.verticesCount;
                    /* Repack vertex data that was populated when importing the model since we only need the position
                     * vector as the vertex attribute
                    */
                    for (auto const& vertex: anchorInfo->meta.vertices)
                        combinedVertices.push_back (vertex.pos);

                    infoId == anchorInfoIds[0] ?
                              anchorInfo->id.vertexBufferInfos.push_back (vertexBufferInfoId):
                              anchorInfo->id.vertexBufferInfos.push_back (UINT32_MAX);
                }

                createVertexBuffer (deviceInfoId,
                                    vertexBufferInfoId,
                                    combinedVerticesCount * sizeof (glm::vec3),
                                    combinedVertices.data());

                LOG_INFO (m_ENAnchorLog) << "[OK] Vertex buffer "
                                         << "[" << vertexBufferInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INDEX BUFFER                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                std::vector <uint32_t> combinedIndices;
                size_t combinedIndicesCount = 0;
                uint32_t indexBufferInfoId  = getNextInfoIdFromBufferType (Core::STAGING_BUFFER);

                for (auto const& infoId: anchorInfoIds) {
                    auto anchorInfo       = getModelInfo (infoId);
                    combinedIndicesCount += anchorInfo->meta.indicesCount;

                    combinedIndices.reserve (combinedIndicesCount);
                    combinedIndices.insert  (combinedIndices.end(), anchorInfo->meta.indices.begin(),
                                                                    anchorInfo->meta.indices.end());

                    infoId == anchorInfoIds[0] ? anchorInfo->id.indexBufferInfo = indexBufferInfoId:
                                                 anchorInfo->id.indexBufferInfo = UINT32_MAX;
                }

                createIndexBuffer (deviceInfoId,
                                   indexBufferInfoId,
                                   combinedIndicesCount * sizeof (uint32_t),
                                   combinedIndices.data());

                LOG_INFO (m_ENAnchorLog) << "[OK] Index buffer "
                                         << "[" << indexBufferInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG STORAGE BUFFERS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < Core::g_coreSettings.maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = anchorSceneInfo->id.storageBufferInfoBase + i;
                    createStorageBuffer (deviceInfoId,
                                         storageBufferInfoId,
                                         anchorSceneInfo->meta.totalInstancesCount * sizeof (Core::InstanceDataSSBO));

                    LOG_INFO (m_ENAnchorLog) << "[OK] Storage buffer "
                                             << "[" << storageBufferInfoId << "]"
                                             << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY PIPELINE INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                derivePipelineInfo (anchorPipelineInfoId, pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VERTEX INPUT                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto bindingDescriptions   = std::vector {
                    getBindingDescription (0, sizeof (glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                };
                auto attributeDescriptions = std::vector {
                    getAttributeDescription (0,
                                             0,
                                             0,
                                             VK_FORMAT_R32G32B32_SFLOAT),
                };
                createVertexInputState (anchorPipelineInfoId, bindingDescriptions, attributeDescriptions);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto anchorPipelineInfo   = getPipelineInfo (anchorPipelineInfoId);
                anchorPipelineInfo->state.stages.clear();
                auto vertexShaderModule   = createShaderStage (deviceInfoId,
                                                               anchorPipelineInfoId,
                                                               VK_SHADER_STAGE_VERTEX_BIT,
                                                               g_pipelineSettings.anchorShaderStage.
                                                               vertexShaderBinaryPath,
                                                               "main");

                auto fragmentShaderModule = createShaderStage (deviceInfoId,
                                                               anchorPipelineInfoId,
                                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                                               g_pipelineSettings.anchorShaderStage.
                                                               fragmentShaderBinaryPath,
                                                               "main");
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - RASTERIZATION                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that, wideLines (VkPhysicalDeviceFeatures) specifies whether lines with width other than 1.0 are
                 * supported. If this feature is not enabled, the lineWidth member must be 1.0 unless the
                 * VK_DYNAMIC_STATE_LINE_WIDTH dynamic state is enabled, in which case the lineWidth parameter to
                 * vkCmdSetLineWidth must be 1.0. When this feature is supported, the range and granularity of
                 * supported line widths are indicated by the lineWidthRange and lineWidthGranularity members of the
                 * VkPhysicalDeviceLimits structure, respectively
                 *
                 * However, as of when this code is written, there are no native vulkan drivers on MacOS nor iOS, only
                 * emulation through MoltenVK which translates vulkan API calls to Metal API calls. Since Metal does not
                 * support wide lines, hence the features is reported as not supported in VkPhysicalDeviceFeatures
                */
                createRasterizationState (anchorPipelineInfoId,
                                          VK_POLYGON_MODE_LINE,
                                          1.0f,
                                          VK_CULL_MODE_NONE,
                                          VK_FRONT_FACE_COUNTER_CLOCKWISE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SET LAYOUT - PER FRAME                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto perFrameLayoutBindings = std::vector {
                    getLayoutBinding (0,
                                      1,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                      VK_SHADER_STAGE_VERTEX_BIT,
                                      VK_NULL_HANDLE)
                };
                auto perFrameBindingFlags = std::vector <VkDescriptorBindingFlags> {
                    0
                };
                createDescriptorSetLayout (deviceInfoId,
                                           anchorPipelineInfoId,
                                           perFrameLayoutBindings,
                                           perFrameBindingFlags,
                                           0);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PUSH CONSTANT RANGES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPushConstantRange (anchorPipelineInfoId,
                                         VK_SHADER_STAGE_VERTEX_BIT,
                                         0,
                                         sizeof (Core::SceneDataVertPC));
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE LAYOUT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPipelineLayout (deviceInfoId, anchorPipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createGraphicsPipeline (deviceInfoId,
                                        renderPassInfoId,
                                        anchorPipelineInfoId,
                                        0, -1,
                                        pipelineInfo->resource.pipeline,
                                        VK_PIPELINE_CREATE_DERIVATIVE_BIT);

                LOG_INFO (m_ENAnchorLog) << "[OK] Pipeline "
                                         << "[" << anchorPipelineInfoId << "]"
                                         << " "
                                         << "[" << renderPassInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SHADER MODULES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                vkDestroyShaderModule (deviceInfo->resource.logDevice, vertexShaderModule,   VK_NULL_HANDLE);
                vkDestroyShaderModule (deviceInfo->resource.logDevice, fragmentShaderModule, VK_NULL_HANDLE);
                LOG_INFO (m_ENAnchorLog) << "[DELETE] Shader modules"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 Core::g_coreSettings.maxFramesInFlight),
                };
                createDescriptorPool (deviceInfoId,
                                      anchorSceneInfoId,
                                      poolSizes,
                                      Core::g_coreSettings.maxFramesInFlight,
                                      0);
                LOG_INFO (m_ENAnchorLog) << "[OK] Descriptor pool "
                                         << "[" << anchorSceneInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS - PER FRAME                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t perFrameDescriptorSetLayoutIdx = 0;
                createDescriptorSets (deviceInfoId,
                                      anchorPipelineInfoId,
                                      anchorSceneInfoId,
                                      perFrameDescriptorSetLayoutIdx,
                                      Core::g_coreSettings.maxFramesInFlight,
                                      Core::PER_FRAME_SET);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE - PER FRAME                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < Core::g_coreSettings.maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = anchorSceneInfo->id.storageBufferInfoBase + i;
                    auto bufferInfo              = getBufferInfo (storageBufferInfoId, Core::STORAGE_BUFFER);
                    auto descriptorBufferInfos   = std::vector {
                        getDescriptorBufferInfo (bufferInfo->resource.buffer,
                                                 0,
                                                 anchorSceneInfo->meta.totalInstancesCount *
                                                 sizeof (Core::InstanceDataSSBO))
                    };

                    auto writeDescriptorSets = std::vector {
                        getWriteBufferDescriptorSetInfo (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                         anchorSceneInfo->resource.perFrameDescriptorSets[i],
                                                         descriptorBufferInfos,
                                                         0, 0, 1)
                    };

                    updateDescriptorSets (deviceInfoId, writeDescriptorSets);
                }
                LOG_INFO (m_ENAnchorLog) << "[OK] Descriptor sets "
                                         << "[" << anchorSceneInfoId << "]"
                                         << " "
                                         << "[" << anchorPipelineInfoId << "]"
                                         << " "
                                         << "[" << perFrameDescriptorSetLayoutIdx << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - COMMAND POOL AND BUFFER                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto transferOpsCommandPool = getCommandPool (deviceInfoId,
                                                              VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                              deviceInfo->meta.transferFamilyIndex.value());
                LOG_INFO (m_ENAnchorLog) << "[OK] Transfer ops command pool "
                                         << "[" << deviceInfoId << "]"
                                         << std::endl;

                auto transferOpsCommandBuffers = std::vector {
                    getCommandBuffers (deviceInfoId, transferOpsCommandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                };
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - SUBMIT & EXECUTE                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t transferOpsFenceInfoId = 0;
                oneTimeOpsQueueSubmit (deviceInfoId,
                                       transferOpsFenceInfoId,
                                       deviceInfo->resource.transferQueue,
                                       transferOpsCommandBuffers[0],
                [&](void) {
                {   /* Copy vertex and index buffers */
                    for (auto const& infoId: anchorInfoBase->id.vertexBufferInfos) {
                        copyBufferToBuffer (infoId, infoId,
                                            Core::STAGING_BUFFER, Core::VERTEX_BUFFER,
                                            0, 0,
                                            transferOpsCommandBuffers[0]);
                    }

                    copyBufferToBuffer     (anchorInfoBase->id.indexBufferInfo,
                                            anchorInfoBase->id.indexBufferInfo,
                                            Core::STAGING_BUFFER, Core::INDEX_BUFFER,
                                            0, 0,
                                            transferOpsCommandBuffers[0]);
                }
                });
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY STAGING BUFFERS                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (deviceInfoId, anchorInfoBase->id.indexBufferInfo, Core::STAGING_BUFFER);
                LOG_INFO (m_ENAnchorLog) << "[DELETE] Staging buffer "
                                         << "[" << anchorInfoBase->id.indexBufferInfo << "]"
                                         << std::endl;

                for (auto const& infoId: anchorInfoBase->id.vertexBufferInfos) {
                    VKBufferMgr::cleanUp (deviceInfoId, infoId, Core::STAGING_BUFFER);
                    LOG_INFO (m_ENAnchorLog) << "[DELETE] Staging buffer "
                                             << "[" << infoId << "]"
                                             << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - COMMAND POOL                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (deviceInfoId, transferOpsCommandPool);
                LOG_INFO (m_ENAnchorLog) << "[DELETE] Transfer ops command pool"
                                         << std::endl;
            }

            void drawExtension (const std::vector <uint32_t>& anchorInfoIds,
                                uint32_t anchorPipelineInfoId,
                                uint32_t cameraInfoId,
                                uint32_t anchorSceneInfoId,
                                uint32_t sceneInfoId,
                                uint32_t currentFrameInFlight) {

                auto anchorInfoBase  = getModelInfo  (anchorInfoIds[0]);
                auto cameraInfo      = getCameraInfo (cameraInfoId);
                auto anchorSceneInfo = getSceneInfo  (anchorSceneInfoId);
                auto sceneInfo       = getSceneInfo  (sceneInfoId);

                std::vector <Core::InstanceDataSSBO> combinedInstances;
                size_t combinedInstancesCount = 0;

                for (auto const& infoId: anchorInfoIds) {
                    auto anchorInfo         = getModelInfo (infoId);
                    combinedInstancesCount += anchorInfo->meta.instancesCount;

                    combinedInstances.reserve (combinedInstancesCount);
                    combinedInstances.insert  (combinedInstances.end(), anchorInfo->meta.instances.begin(),
                                                                        anchorInfo->meta.instances.end());
                }
                updateStorageBuffer (anchorSceneInfo->id.storageBufferInfoBase + currentFrameInFlight,
                                     anchorSceneInfo->meta.totalInstancesCount * sizeof (Core::InstanceDataSSBO),
                                     combinedInstances.data());

                Core::SceneDataVertPC sceneDataVert;
                sceneDataVert.viewMatrix       = cameraInfo->transform.viewMatrix;
                sceneDataVert.projectionMatrix = cameraInfo->transform.projectionMatrix;

                bindPipeline        (anchorPipelineInfoId,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                updatePushConstants (anchorPipelineInfoId,
                                     VK_SHADER_STAGE_VERTEX_BIT,
                                     0, sizeof (Core::SceneDataVertPC), &sceneDataVert,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                auto vertexBufferInfoIdsToBind = anchorInfoBase->id.vertexBufferInfos;
                auto vertexBufferOffsets       = std::vector <VkDeviceSize> {
                    0
                };
                bindVertexBuffers   (vertexBufferInfoIdsToBind,
                                     0,
                                     vertexBufferOffsets,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                bindIndexBuffer     (anchorInfoBase->id.indexBufferInfo,
                                     0,
                                     VK_INDEX_TYPE_UINT32,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                auto descriptorSetsToBind = std::vector {
                    anchorSceneInfo->resource.perFrameDescriptorSets[currentFrameInFlight],
                };
                auto dynamicOffsets       = std::vector <uint32_t> {
                };
                bindDescriptorSets  (anchorPipelineInfoId,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     0,
                                     descriptorSetsToBind,
                                     dynamicOffsets,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                uint32_t firstIndex    = 0;
                int32_t  vertexOffset  = 0;
                uint32_t firstInstance = 0;

                for (auto const& infoId: anchorInfoIds) {
                    auto anchorInfo = getModelInfo (infoId);

                    drawIndexed (infoId,
                                 firstIndex, vertexOffset, firstInstance,
                                 sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                    firstIndex    += anchorInfo->meta.indicesCount;
                    vertexOffset  += anchorInfo->meta.verticesCount;
                    firstInstance += anchorInfo->meta.instancesCount;
                }
            }
    };
}   // namespace SandBox
#endif  // EN_ANCHOR_H