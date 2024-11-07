#ifndef EN_SKY_BOX_H
#define EN_SKY_BOX_H

#include "../../Core/Model/VKModelMgr.h"
#include "../../Core/Image/VKTextureImage.h"
#include "../../Core/Buffer/VKVertexBuffer.h"
#include "../../Core/Buffer/VKIndexBuffer.h"
#include "../../Core/Pipeline/VKVertexInput.h"
#include "../../Core/Pipeline/VKShaderStage.h"
#include "../../Core/Pipeline/VKRasterization.h"
#include "../../Core/Pipeline/VKDepthStencil.h"
#include "../../Core/Pipeline/VKDescriptorSetLayout.h"
#include "../../Core/Pipeline/VKPushConstantRange.h"
#include "../../Core/Pipeline/VKPipelineLayout.h"
#include "../../Core/Cmd/VKCmdBuffer.h"
#include "../../Core/Cmd/VKCmd.h"
#include "../../Core/Scene/VKCameraMgr.h"
#include "../../Core/Scene/VKTextureSampler.h"
#include "../../Core/Scene/VKDescriptor.h"
#include "../../Core/Scene/VKSyncObject.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENSkyBox: protected virtual Core::VKModelMgr,
                    protected virtual Core::VKTextureImage,
                    protected virtual Core::VKVertexBuffer,
                    protected virtual Core::VKIndexBuffer,
                    protected virtual Core::VKVertexInput,
                    protected virtual Core::VKShaderStage,
                    protected virtual Core::VKRasterization,
                    protected virtual Core::VKDepthStencil,
                    protected virtual Core::VKDescriptorSetLayout,
                    protected virtual Core::VKPushConstantRange,
                    protected virtual Core::VKPipelineLayout,
                    protected virtual Core::VKCmdBuffer,
                    protected virtual Core::VKCmd,
                    protected virtual Core::VKCameraMgr,
                    protected virtual Core::VKTextureSampler,
                    protected virtual Core::VKDescriptor,
                    protected virtual Core::VKSyncObject {
        private:
            uint32_t m_skyBoxImageInfoId;

            Log::Record* m_ENSkyBoxLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            ENSkyBox (void) {
                m_ENSkyBoxLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~ENSkyBox (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void initExtension (uint32_t deviceInfoId,
                                uint32_t skyBoxModelInfoId,
                                uint32_t renderPassInfoId,
                                uint32_t skyBoxPipelineInfoId,
                                uint32_t pipelineInfoId,
                                uint32_t skyBoxSceneInfoId) {

                auto deviceInfo      = getDeviceInfo   (deviceInfoId);
                auto skyBoxModelInfo = getModelInfo    (skyBoxModelInfoId);
                auto pipelineInfo    = getPipelineInfo (pipelineInfoId);
                auto skyBoxSceneInfo = getSceneInfo    (skyBoxSceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | IMPORT MODEL                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                importOBJModel (skyBoxModelInfoId);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Import model "
                                         << "[" << skyBoxModelInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE RESOURCES - DIFFUSE TEXTURE                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                m_skyBoxImageInfoId = getNextInfoIdFromImageType (Core::TEXTURE_IMAGE);
                /* A cube map is a texture that contains 6 individual 2D textures that each form one side of a cube: a
                 * textured cube. Why bother combining 6 individual textures into a single entity instead of just using 6
                 * individual textures? Well, cube maps have the useful property that they can be indexed/sampled using a
                 * direction vector. If we imagine we have a cube shape that we attach such a cube map to, this direction
                 * vector would be similar to the (interpolated) local vertex position of the cube. This way we can sample
                 * the cube map using the cube's actual position vectors as long as the cube is centered on the origin
                 *
                 * Note that, we need to load the appropriate texture paths in a vector in the order as specified by the
                 * target enums. Also, the sky box texture image is not added to the global texture pool, hence will need
                 * to be deleted as part of the extension
                */
                auto texturePaths   = std::vector <const char*> {
                    g_skyBoxTextureImagePool[POSITIVE_X],
                    g_skyBoxTextureImagePool[NEGATIVE_X],
                    g_skyBoxTextureImagePool[POSITIVE_Y],
                    g_skyBoxTextureImagePool[NEGATIVE_Y],
                    g_skyBoxTextureImagePool[POSITIVE_Z],
                    g_skyBoxTextureImagePool[NEGATIVE_Z]
                };
                createTextureResources (deviceInfoId,
                                        m_skyBoxImageInfoId,
                                        6,
                                        texturePaths,
                                        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                        VK_IMAGE_VIEW_TYPE_CUBE,
                                        false);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Texture resources "
                                         << "[" << m_skyBoxImageInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG VERTEX BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Repack vertex data that was populated when importing the model since we only need the position vector
                 * as the vertex attribute
                */
                std::vector <glm::vec3> vertices;
                for (auto const& vertex: skyBoxModelInfo->meta.vertices)
                    vertices.push_back (vertex.pos);

                uint32_t vertexBufferInfoId = getNextInfoIdFromBufferType (Core::STAGING_BUFFER);
                skyBoxModelInfo->id.vertexBufferInfos.push_back (vertexBufferInfoId);

                createVertexBuffer (deviceInfoId,
                                    vertexBufferInfoId,
                                    skyBoxModelInfo->meta.verticesCount * sizeof (glm::vec3),
                                    vertices.data());

                LOG_INFO (m_ENSkyBoxLog) << "[OK] Vertex buffer "
                                         << "[" << vertexBufferInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INDEX BUFFER                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t indexBufferInfoId = getNextInfoIdFromBufferType (Core::STAGING_BUFFER);
                skyBoxModelInfo->id.indexBufferInfo = indexBufferInfoId;

                createIndexBuffer (deviceInfoId,
                                   indexBufferInfoId,
                                   skyBoxModelInfo->meta.indicesCount * sizeof (uint32_t),
                                   skyBoxModelInfo->meta.indices.data());

                LOG_INFO (m_ENSkyBoxLog) << "[OK] Index buffer "
                                         << "[" << indexBufferInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | READY PIPELINE INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                derivePipelineInfo (skyBoxPipelineInfoId, pipelineInfoId);
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
                createVertexInputState (skyBoxPipelineInfoId, bindingDescriptions, attributeDescriptions);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto skyBoxPipelineInfo   = getPipelineInfo (skyBoxPipelineInfoId);
                skyBoxPipelineInfo->state.stages.clear();
                auto vertexShaderModule   = createShaderStage (deviceInfoId,
                                                               skyBoxPipelineInfoId,
                                                               VK_SHADER_STAGE_VERTEX_BIT,
                                                               g_pipelineSettings.skyBoxShaderStage.
                                                               vertexShaderBinaryPath,
                                                               "main");

                auto fragmentShaderModule = createShaderStage (deviceInfoId,
                                                               skyBoxPipelineInfoId,
                                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                                               g_pipelineSettings.skyBoxShaderStage.
                                                               fragmentShaderBinaryPath,
                                                               "main");
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - RASTERIZATION                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRasterizationState (skyBoxPipelineInfoId,
                                          VK_POLYGON_MODE_FILL,
                                          1.0f,
                                          VK_CULL_MODE_FRONT_BIT,
                                          VK_FRONT_FACE_COUNTER_CLOCKWISE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - DEPTH STENCIL                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* There are two options when we render the sky box. First, we render the sky box first before we
                 * render all the other objects in the scene. This works great, but is too inefficient. If we render
                 * the sky box first, we're running the fragment shader for each pixel on the screen even though only a
                 * small part of the sky box will eventually be visible
                 *
                 * Second, render the sky box last. This way, the depth buffer is completely filled with all the scene's
                 * depth values so we only have to render the sky box's fragments wherever the early depth test passes,
                 * greatly reducing the number of fragment shader calls. The problem is that the sky box will most likely
                 * render on top of all other objects since it's only a 1x1x1 cube, succeeding most depth tests
                 *
                 * We need to trick the depth buffer into believing that the sky box has the maximum depth value of 1.0
                 * so that it fails the depth test wherever there's a different object in front of it. We know that
                 * perspective division is performed after the vertex shader has run, dividing the gl_Position's xyz
                 * coordinates by its w component. We also know that the z component of the resulting division is equal
                 * to that vertex's depth value. Using this information we can set the z component of the output
                 * position equal to its w component which will result in a z component that is always equal to 1.0,
                 * because when the perspective division is applied its z component translates to w / w = 1.0
                 *
                 * The resulting normalized device coordinates will then always have a z value equal to 1.0: the maximum
                 * depth value. The sky box will as a result only be rendered wherever there are no objects visible
                 * (only then it will pass the depth test, everything else is in front of the sky box)
                 *
                 * Note that, the depth buffer will be filled with values of 1.0 for the sky box, so we need to make sure
                 * the sky box passes the depth tests with values less than or equal to the depth buffer, hence why
                 * the compare operation is set to VK_COMPARE_OP_LESS_OR_EQUAL
                */
                createDepthStencilState (skyBoxPipelineInfoId,
                                         VK_TRUE,
                                         VK_TRUE,
                                         VK_FALSE,
                                         VK_COMPARE_OP_LESS_OR_EQUAL,
                                         0.0f,
                                         1.0f,
                                         VK_FALSE,
                                         VK_NULL_HANDLE, VK_NULL_HANDLE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SET LAYOUT                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto layoutBindings = std::vector {
                    getLayoutBinding (0,
                                      1,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                      VK_NULL_HANDLE)
                };
                auto bindingFlags   = std::vector <VkDescriptorBindingFlags> {
                    0
                };
                createDescriptorSetLayout (deviceInfoId,
                                           skyBoxPipelineInfoId,
                                           layoutBindings,
                                           bindingFlags,
                                           0);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PUSH CONSTANT RANGES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPushConstantRange (skyBoxPipelineInfoId,
                                         VK_SHADER_STAGE_VERTEX_BIT,
                                         0,
                                         sizeof (Core::SceneDataVertPC));
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE LAYOUT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPipelineLayout (deviceInfoId, skyBoxPipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createGraphicsPipeline (deviceInfoId,
                                        renderPassInfoId,
                                        skyBoxPipelineInfoId,
                                        0, -1,
                                        pipelineInfo->resource.pipeline,
                                        VK_PIPELINE_CREATE_DERIVATIVE_BIT);

                LOG_INFO (m_ENSkyBoxLog) << "[OK] Pipeline "
                                         << "[" << skyBoxPipelineInfoId << "]"
                                         << " "
                                         << "[" << renderPassInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SHADER MODULES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                vkDestroyShaderModule (deviceInfo->resource.logDevice, vertexShaderModule,   VK_NULL_HANDLE);
                vkDestroyShaderModule (deviceInfo->resource.logDevice, fragmentShaderModule, VK_NULL_HANDLE);
                LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Shader modules"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE SAMPLER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* We set the sampler address mode to VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE since texture coordinates
                 * that are exactly between two faces may not hit an exact face (due to some hardware limitations) so by
                 * using VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, the sampler always returns their edge values whenever we
                 * sample between faces
                */
                createTextureSampler (deviceInfoId,
                                      skyBoxSceneInfoId,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      VK_TRUE,
                                      VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                      0.0f,
                                      0.0f,
                                      13.0f);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Texture sampler "
                                         << "[" << skyBoxSceneInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                };
                createDescriptorPool (deviceInfoId,
                                      skyBoxSceneInfoId,
                                      poolSizes,
                                      1,
                                      0);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Descriptor pool "
                                         << "[" << skyBoxSceneInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t descriptorSetLayoutId = 0;
                createDescriptorSets (deviceInfoId,
                                      skyBoxPipelineInfoId,
                                      skyBoxSceneInfoId,
                                      descriptorSetLayoutId,
                                      1);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto imageInfo            = getImageInfo (m_skyBoxImageInfoId, Core::TEXTURE_IMAGE);
                auto descriptorImageInfos = std::vector {
                    getDescriptorImageInfo  (skyBoxSceneInfo->resource.textureSampler,
                                             imageInfo->resource.imageView,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                };
                auto writeDescriptorSets  = std::vector {
                    getWriteImageDescriptorSetInfo  (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                     skyBoxSceneInfo->resource.descriptorSets[0],
                                                     descriptorImageInfos,
                                                     0, 0, 1)
                };

                updateDescriptorSets (deviceInfoId, writeDescriptorSets);

                LOG_INFO (m_ENSkyBoxLog) << "[OK] Descriptor sets "
                                         << "[" << skyBoxSceneInfoId << "]"
                                         << " "
                                         << "[" << skyBoxPipelineInfoId << "]"
                                         << " "
                                         << "[" << descriptorSetLayoutId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - COMMAND POOL AND BUFFER                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto transferOpsCommandPool = getCommandPool (deviceInfoId,
                                                              VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                              deviceInfo->meta.transferFamilyIndex.value());
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Transfer ops command pool "
                                         << "[" << deviceInfoId << "]"
                                         << std::endl;

                auto transferOpsCommandBuffers = std::vector {
                    getCommandBuffers (deviceInfoId, transferOpsCommandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                };
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - FENCE                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t transferOpsFenceInfoId = 0;
                createFence (deviceInfoId, transferOpsFenceInfoId, Core::FEN_TRANSFER_DONE, 0);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Transfer ops fence "
                                         << "[" << transferOpsFenceInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - RECORD AND SUBMIT                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                beginRecording (transferOpsCommandBuffers[0],
                                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                VK_NULL_HANDLE);

                for (uint32_t layerIdx = 0; layerIdx < 6; layerIdx++) {
                    uint32_t bufferInfoId = m_skyBoxImageInfoId + layerIdx;

                    copyBufferToImage (bufferInfoId, m_skyBoxImageInfoId,
                                       Core::STAGING_BUFFER, Core::TEXTURE_IMAGE,
                                       0,
                                       layerIdx,
                                       transferOpsCommandBuffers[0]);

                    transitionImageLayout (m_skyBoxImageInfoId,
                                           Core::TEXTURE_IMAGE,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           0, 1,
                                           layerIdx, 1,
                                           transferOpsCommandBuffers[0]);
                }

                for (auto const& infoId: skyBoxModelInfo->id.vertexBufferInfos) {
                    copyBufferToBuffer (infoId, infoId,
                                        Core::STAGING_BUFFER, Core::VERTEX_BUFFER,
                                        0, 0,
                                        transferOpsCommandBuffers[0]);
                }

                copyBufferToBuffer (skyBoxModelInfo->id.indexBufferInfo,
                                    skyBoxModelInfo->id.indexBufferInfo,
                                    Core::STAGING_BUFFER, Core::INDEX_BUFFER,
                                    0, 0,
                                    transferOpsCommandBuffers[0]);

                endRecording (transferOpsCommandBuffers[0]);

                VkSubmitInfo transferOpsSubmitInfo{};
                transferOpsSubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                transferOpsSubmitInfo.commandBufferCount = static_cast <uint32_t> (transferOpsCommandBuffers.size());
                transferOpsSubmitInfo.pCommandBuffers    = transferOpsCommandBuffers.data();

                auto fenceInfo  = getFenceInfo  (transferOpsFenceInfoId, Core::FEN_TRANSFER_DONE);
                VkResult result = vkQueueSubmit (deviceInfo->resource.transferQueue,
                                                 1,
                                                 &transferOpsSubmitInfo,
                                                 fenceInfo->resource.fence);

                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_ENSkyBoxLog) << "Failed to submit transfer ops command buffer "
                                              << "[" << deviceInfoId << "]"
                                              << " "
                                              << "[" << string_VkResult (result) << "]"
                                              << std::endl;
                    throw std::runtime_error ("Failed to submit transfer ops command buffer");
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - WAIT                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                LOG_INFO (m_ENSkyBoxLog) << "[WAITING] Transfer ops fence "
                                         << "[" << transferOpsFenceInfoId << "]"
                                         << std::endl;
                vkWaitForFences (deviceInfo->resource.logDevice,
                                 1,
                                 &fenceInfo->resource.fence,
                                 VK_TRUE,
                                 UINT64_MAX);

                vkResetFences   (deviceInfo->resource.logDevice,
                                 1,
                                 &fenceInfo->resource.fence);
                LOG_INFO (m_ENSkyBoxLog) << "[OK] Transfer ops fence reset "
                                         << "[" << transferOpsFenceInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY STAGING BUFFERS                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (deviceInfoId, skyBoxModelInfo->id.indexBufferInfo, Core::STAGING_BUFFER);
                LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Staging buffer "
                                         << "[" << skyBoxModelInfo->id.indexBufferInfo << "]"
                                         << std::endl;

                for (auto const& infoId: skyBoxModelInfo->id.vertexBufferInfos) {
                    VKBufferMgr::cleanUp (deviceInfoId, infoId, Core::STAGING_BUFFER);
                    LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Staging buffer "
                                             << "[" << infoId << "]"
                                             << std::endl;
                }

                for (uint32_t layerIdx = 0; layerIdx < 6; layerIdx++) {
                    uint32_t bufferInfoId = m_skyBoxImageInfoId + layerIdx;

                    VKBufferMgr::cleanUp (deviceInfoId, bufferInfoId, Core::STAGING_BUFFER);
                    LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Staging buffer "
                                             << "[" << bufferInfoId << "]"
                                             << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - FENCE                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                cleanUpFence (deviceInfoId, transferOpsFenceInfoId, Core::FEN_TRANSFER_DONE);
                LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Transfer ops fence "
                                         << "[" << transferOpsFenceInfoId << "]"
                                         << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - COMMAND POOL                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (deviceInfoId, transferOpsCommandPool);
                LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Transfer ops command pool"
                                         << std::endl;
            }

            void drawExtension (uint32_t skyBoxModelInfoId,
                                uint32_t skyBoxPipelineInfoId,
                                uint32_t cameraInfoId,
                                uint32_t skyBoxSceneInfoId,
                                uint32_t sceneInfoId,
                                uint32_t currentFrameInFlight) {

                auto skyBoxModelInfo = getModelInfo  (skyBoxModelInfoId);
                auto cameraInfo      = getCameraInfo (cameraInfoId);
                auto skyBoxSceneInfo = getSceneInfo  (skyBoxSceneInfoId);
                auto sceneInfo       = getSceneInfo  (sceneInfoId);

                Core::SceneDataVertPC sceneDataVert;
                sceneDataVert.viewMatrix       = cameraInfo->transform.viewMatrix;
                sceneDataVert.projectionMatrix = cameraInfo->transform.projectionMatrix;

                bindPipeline        (skyBoxPipelineInfoId,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                updatePushConstants (skyBoxPipelineInfoId,
                                     VK_SHADER_STAGE_VERTEX_BIT,
                                     0, sizeof (Core::SceneDataVertPC), &sceneDataVert,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                auto vertexBufferInfoIdsToBind = skyBoxModelInfo->id.vertexBufferInfos;
                auto vertexBufferOffsets       = std::vector <VkDeviceSize> {
                    0
                };
                bindVertexBuffers   (vertexBufferInfoIdsToBind,
                                     0,
                                     vertexBufferOffsets,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                bindIndexBuffer     (skyBoxModelInfo->id.indexBufferInfo,
                                     0,
                                     VK_INDEX_TYPE_UINT32,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                auto descriptorSetsToBind = std::vector {
                    skyBoxSceneInfo->resource.descriptorSets[0]
                };
                auto dynamicOffsets       = std::vector <uint32_t> {
                };
                bindDescriptorSets  (skyBoxPipelineInfoId,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     0,
                                     descriptorSetsToBind,
                                     dynamicOffsets,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);

                drawIndexed         (skyBoxModelInfo->meta.indicesCount,
                                     skyBoxModelInfo->meta.instancesCount,
                                     0, 0, 0,
                                     sceneInfo->resource.commandBuffers[currentFrameInFlight]);
            }

            void deleteExtension (uint32_t deviceInfoId) {
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE RESOURCES - DIFFUSE TEXTURE                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (deviceInfoId, m_skyBoxImageInfoId, Core::TEXTURE_IMAGE);
                LOG_INFO (m_ENSkyBoxLog) << "[DELETE] Texture resources "
                                         << "[" << m_skyBoxImageInfoId << "]"
                                         << std::endl;
            }
    };
}   // namespace SandBox
#endif  // EN_SKY_BOX_H