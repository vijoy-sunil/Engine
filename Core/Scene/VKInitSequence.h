#ifndef VK_INIT_SEQUENCE_H
#define VK_INIT_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Device/VKInstance.h"
#include "../Device/VKSurface.h"
#include "../Device/VKLogDevice.h"
#include "../Model/VKInstanceData.h"
#include "../Image/VKSwapChainImage.h"
#include "../Image/VKTextureImage.h"
#include "../Image/VKDepthImage.h"
#include "../Image/VKMultiSampleImage.h"
#include "../Buffer/VKVertexBuffer.h"
#include "../Buffer/VKIndexBuffer.h"
#include "../Buffer/VKStorageBuffer.h"
#include "../RenderPass/VKAttachment.h"
#include "../RenderPass/VKSubPass.h"
#include "../RenderPass/VKFrameBuffer.h"
#include "../Pipeline/VKVertexInput.h"
#include "../Pipeline/VKInputAssembly.h"
#include "../Pipeline/VKShaderStage.h"
#include "../Pipeline/VKViewPort.h"
#include "../Pipeline/VKRasterization.h"
#include "../Pipeline/VKMultiSample.h"
#include "../Pipeline/VKDepthStencil.h"
#include "../Pipeline/VKColorBlend.h"
#include "../Pipeline/VKDynamicState.h"
#include "../Pipeline/VKDescriptorSetLayout.h"
#include "../Pipeline/VKPushConstantRange.h"
#include "../Pipeline/VKPipelineLayout.h"
#include "../Cmd/VKCmdBuffer.h"
#include "../Cmd/VKCmd.h"
#include "VKTextureSampler.h"
#include "VKDescriptor.h"
#include "VKSyncObject.h"

namespace Core {
    class VKInitSequence: protected virtual VKWindow,
                          protected virtual VKInstance,
                          protected virtual VKSurface,
                          protected virtual VKLogDevice,
                          protected virtual VKInstanceData,
                          protected virtual VKSwapChainImage,
                          protected virtual VKTextureImage,
                          protected virtual VKDepthImage,
                          protected virtual VKMultiSampleImage,
                          protected virtual VKVertexBuffer,
                          protected virtual VKIndexBuffer,
                          protected virtual VKStorageBuffer,
                          protected virtual VKAttachment,
                          protected virtual VKSubPass,
                          protected virtual VKFrameBuffer,
                          protected virtual VKVertexInput,
                          protected VKInputAssembly,
                          protected virtual VKShaderStage,
                          protected VKViewPort,
                          protected virtual VKRasterization,
                          protected VKMultiSample,
                          protected virtual VKDepthStencil,
                          protected VKColorBlend,
                          protected VKDynamicState,
                          protected virtual VKDescriptorSetLayout,
                          protected virtual VKPushConstantRange,
                          protected virtual VKPipelineLayout,
                          protected virtual VKCmdBuffer,
                          protected virtual VKCmd,
                          protected virtual VKTextureSampler,
                          protected virtual VKDescriptor,
                          protected virtual VKSyncObject {
        private:
            Log::Record* m_VKInitSequenceLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKInitSequence (void) {
                m_VKInitSequenceLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKInitSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            template <typename T>
            void runSequence (uint32_t deviceInfoId,
                              const std::vector <uint32_t>& modelInfoIds,
                              uint32_t renderPassInfoId,
                              uint32_t pipelineInfoId,
                              uint32_t sceneInfoId,
                              T extensions) {

                auto deviceInfo    = getDeviceInfo (deviceInfoId);
                auto modelInfoBase = getModelInfo  (modelInfoIds[0]);
                auto sceneInfo     = getSceneInfo  (sceneInfoId);
#if ENABLE_LOGGING
                enableValidationLayers();
#else
                LOG_INFO (m_VKInitSequenceLog) << "Disabling validation layers and logging"
                                               << std::endl;
                disableValidationLayers();
                LOG_CLEAR_ALL_CONFIGS;
#endif  // ENABLE_LOGGING
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG WINDOW                                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createWindow (deviceInfoId, g_windowSettings.width, g_windowSettings.height);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Window "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INSTANCE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createInstance (deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Instance "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEBUG MESSENGER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDebugMessenger (deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Debug messenger "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SURFACE                                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSurface (deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Surface "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PHY DEVICE                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                pickPhyDevice (deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Phy device "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG LOG DEVICE                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                createLogDevice (deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Log device "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | IMPORT MODEL                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& infoId: modelInfoIds) {
                    importOBJModel (infoId);
                    /* Populate texture image info id look up table for all model instances
                    */
                    auto modelInfo = getModelInfo (infoId);
                    for (uint32_t i = 0; i < modelInfo->meta.instancesCount; i++) {
                        for (auto const& texId: modelInfo->id.diffuseTextureImageInfos)
                            updateTexIdLUT (infoId, i, texId, texId);
                    }
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Import model "
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SWAP CHAIN RESOURCES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSwapChainResources (deviceInfoId, sceneInfo->id.swapChainImageInfoBase);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Swap chain resources "
                                               << "[" << sceneInfo->id.swapChainImageInfoBase << "]"
                                               << " "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE RESOURCES - DIFFUSE TEXTURE                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Create texture resources from the texture image pool, this is to ensure that duplicate texture images
                 * across models are not loaded again
                */
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    auto texturePaths = std::vector <const char*> {
                        path.c_str()
                    };
                    createTextureResources (deviceInfoId,
                                            infoId,
                                            1,
                                            texturePaths,
                                            0,
                                            VK_IMAGE_VIEW_TYPE_2D);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture resources "
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEPTH RESOURCES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthResources (deviceInfoId, sceneInfo->id.depthImageInfo);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Depth resources "
                                               << "[" << sceneInfo->id.depthImageInfo << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG MULTI SAMPLE RESOURCES                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleResources (deviceInfoId, sceneInfo->id.multiSampleImageInfo);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Multi sample resources "
                                               << "[" << sceneInfo->id.multiSampleImageInfo << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG VERTEX BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                std::vector <Vertex> combinedVertices;
                size_t combinedVerticesCount = 0;
                uint32_t vertexBufferInfoId  = getNextInfoIdFromBufferType (STAGING_BUFFER);
                /* Combine all vertex buffers to a single buffer. Note that, only the first model will have access to
                 * the vertex buffer info id, and the remaining models will have it set to UINT32_MAX to indicate that
                 * their vertex buffers are owned by another model
                */
                for (auto const& infoId: modelInfoIds) {
                    auto modelInfo         = getModelInfo (infoId);
                    combinedVerticesCount += modelInfo->meta.verticesCount;

                    combinedVertices.reserve (combinedVerticesCount);
                    combinedVertices.insert  (combinedVertices.end(), modelInfo->meta.vertices.begin(),
                                                                      modelInfo->meta.vertices.end());

                    infoId == modelInfoIds[0] ? modelInfo->id.vertexBufferInfos.push_back (vertexBufferInfoId):
                                                modelInfo->id.vertexBufferInfos.push_back (UINT32_MAX);
                }

                createVertexBuffer (deviceInfoId,
                                    vertexBufferInfoId,
                                    combinedVerticesCount * sizeof (Vertex),
                                    combinedVertices.data());

                LOG_INFO (m_VKInitSequenceLog) << "[OK] Vertex buffer "
                                               << "[" << vertexBufferInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INDEX BUFFER                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                std::vector <uint32_t> combinedIndices;
                size_t combinedIndicesCount = 0;
                uint32_t indexBufferInfoId  = getNextInfoIdFromBufferType (STAGING_BUFFER);

                for (auto const& infoId: modelInfoIds) {
                    auto modelInfo        = getModelInfo (infoId);
                    combinedIndicesCount += modelInfo->meta.indicesCount;

                    combinedIndices.reserve (combinedIndicesCount);
                    combinedIndices.insert  (combinedIndices.end(), modelInfo->meta.indices.begin(),
                                                                    modelInfo->meta.indices.end());

                    infoId == modelInfoIds[0] ? modelInfo->id.indexBufferInfo = indexBufferInfoId:
                                                modelInfo->id.indexBufferInfo = UINT32_MAX;
                }

                createIndexBuffer (deviceInfoId,
                                   indexBufferInfoId,
                                   combinedIndicesCount * sizeof (uint32_t),
                                   combinedIndices.data());

                LOG_INFO (m_VKInitSequenceLog) << "[OK] Index buffer "
                                               << "[" << indexBufferInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG STORAGE BUFFERS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Uniform buffers are great for small, read only data. But what if you want data you don’t know the
                 * size of in the shader? Or data that can be writeable. You use storage buffers for that. Storage
                 * buffers are usually slightly slower than uniform buffers, but they can be much, much bigger. With
                 * storage buffers, you can have an unsized array in a shader with whatever data you want. A common use
                 * for them is to store the transformation data of all the models in the scene. Shader storage buffers
                 * are created in the same way as uniform buffers. They also work in mostly the same way, they just have
                 * different properties like increased maximum size, and being writeable in shaders
                */

                /* We should have multiple buffers, because multiple frames may be in flight at the same time and we
                 * don't want to update the buffer in preparation of the next frame while a previous one is still reading
                 * from it. Thus, we need to have as many buffers as we have frames in flight, and write to a buffer that
                 * is not currently being read by the GPU
                */
                for (uint32_t i = 0; i < g_coreSettings.maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = sceneInfo->id.storageBufferInfoBase + i;
                    createStorageBuffer (deviceInfoId,
                                         storageBufferInfoId,
                                         sceneInfo->meta.totalInstancesCount * sizeof (InstanceDataSSBO));

                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Storage buffer "
                                                   << "[" << storageBufferInfoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY RENDER PASS INFO                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyRenderPassInfo (renderPassInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS ATTACHMENTS                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* For multi-sampled rendering in Vulkan, the multi-sampled image is treated separately from the final
                 * single-sampled image. This provides separate control over what values need to reach memory, since
                 * like the depth buffer, the multi-sampled image may only need to be accessed during the processing of
                 * a tile. For this reason, if the multi-sampled image is not required after the render pass, it can be
                 * created with VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT and bound to an allocation created with
                 * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT. The multi-sampled attachment storeOp can then be set to
                 * VK_ATTACHMENT_STORE_OP_DONT_CARE in the VkAttachmentDescription, so that (at least on tiled renderers)
                 * the full multi-sampled attachment does not need to be written to memory, which can save a lot of
                 * bandwidth
                 *
                 * Note that, multi sampled images cannot be presented directly. We first need to resolve them to a
                 * regular image
                */
                createAttachment    (sceneInfo->id.multiSampleImageInfo,
                                     renderPassInfoId,
                                     MULTI_SAMPLE_IMAGE,
                                     0,
                                     VK_ATTACHMENT_LOAD_OP_CLEAR,       VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                     VK_ATTACHMENT_LOAD_OP_DONT_CARE,   VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                     VK_IMAGE_LAYOUT_UNDEFINED,         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

                /* Create depth attachment. Note that, the format should be the same as the depth image itself. We don't
                 * care about storing the depth data (storeOp), because it will not be used after drawing has finished.
                 * This may allow the hardware to perform additional optimizations
                */
                createAttachment    (sceneInfo->id.depthImageInfo,
                                     renderPassInfoId,
                                     DEPTH_IMAGE,
                                     0,
                                     VK_ATTACHMENT_LOAD_OP_CLEAR,       VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                     VK_ATTACHMENT_LOAD_OP_DONT_CARE,   VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                     VK_IMAGE_LAYOUT_UNDEFINED,         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

                createAttachment    (sceneInfo->id.swapChainImageInfoBase,
                                     renderPassInfoId,
                                     SWAP_CHAIN_IMAGE,
                                     0,
                                     VK_ATTACHMENT_LOAD_OP_DONT_CARE,   VK_ATTACHMENT_STORE_OP_STORE,
                                     VK_ATTACHMENT_LOAD_OP_DONT_CARE,   VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                     VK_IMAGE_LAYOUT_UNDEFINED,         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* The first transition for an attached image of a render pass will be from the initialLayout for the
                 * render pass to the reference layout for the first sub pass that uses the image. The last transition
                 * for an attached image will be from reference layout of the final sub pass that uses the attachment to
                 * the finalLayout for the render pass
                */
                auto inputAttachmentRefs       = std::vector <VkAttachmentReference> {
                };
                auto colorAttachmentRefs       = std::vector {
                    getAttachmentReference (0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                };
                auto depthStencilAttachmentRef =
                    getAttachmentReference (1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                auto resolveAttachmentRefs     = std::vector {
                    getAttachmentReference (2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                };
                createSubPass (renderPassInfoId,
                               inputAttachmentRefs,
                               colorAttachmentRefs,
                               &depthStencilAttachmentRef,
                               resolveAttachmentRefs);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS DEPENDENCIES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* It is possible that multiple frames are rendered simultaneously by the GPU. This is a problem when
                 * using a single depth buffer, because one frame could overwrite the depth buffer while a previous
                 * frame is still rendering to it. To prevent this, we add a sub pass dependency that synchronizes
                 * accesses to the depth attachment
                 *
                 * This dependency tells vulkan that the depth attachment in a render pass cannot be used before previous
                 * render passes have finished using it
                 *
                 * Note that, the depth image is first accessed in the early fragment test pipeline stage and we need to
                 * make sure that there is no conflict between the transitioning of the depth image and it being cleared
                 * as part of its load operation (VK_ATTACHMENT_LOAD_OP_CLEAR)
                */
                createDependency (renderPassInfoId,
                                  0,
                                  VK_SUBPASS_EXTERNAL, 0,
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                  VK_ACCESS_NONE,
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
                /* Remember that the sub passes in a render pass automatically take care of image layout transitions.
                 * These transitions are controlled by sub pass dependencies, which specify memory and execution
                 * dependencies between sub passes. Note that, the operations right before and right after a sub pass
                 * also count as implicit "sub passes"
                 *
                 * There are two built-in dependencies that take care of the transition at the start of the render pass
                 * and at the end of the render pass, but the former does not occur at the right time. It assumes that
                 * the transition occurs at the start of the pipeline, but we haven't acquired the image yet at that
                 * point (see sync object wait operation)
                 *
                 * Solution: (We choose option #2)
                 * (1) We could change the waitStages for the image available semaphore to
                 * VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to ensure that the render passes don't begin until the image is
                 * available
                 *
                 * (2) We can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
                 * (this stage specifies the stage of the pipeline after blending where the final color values are
                 * output from the pipeline)
                 *
                 * Image layout transition
                 * Before the render pass, the layout of the image will be transitioned to the layout you specify, for
                 * example VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. However, by default this happens at the beginning
                 * of the pipeline at which point we haven't acquired the image yet (we acquire it in the
                 * VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage per the sync object wait operation). That means
                 * that we need to change the behaviour of the render pass to also only change the layout once we've
                 * come to that stage
                 *
                 * The stage masks in the sub pass dependency allow the sub pass to already begin before the image is
                 * available up until the point where it needs to write to it
                */
                createDependency (renderPassInfoId,
                                  0,
                                  VK_SUBPASS_EXTERNAL, 0,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                  VK_ACCESS_NONE,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRenderPass (deviceInfoId, renderPassInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Render pass "
                                               << "[" << renderPassInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG FRAME BUFFERS                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Define attachments for every frame buffer
                 * In MSAA, each pixel is sampled in an offscreen buffer which is then rendered to the screen. This
                 * new buffer is slightly different from regular images we've been rendering to - they have to be able
                 * to store more than one sample per pixel. Once a multi sampled buffer is created, it has to be
                 * resolved to the default frame buffer (which stores only a single sample per pixel). This is why we
                 * have to create an additional render target. We only need one render target since only one drawing
                 * operation is active at a time, just like with the depth buffer
                 *
                 * Note that, we are using the same depth image on each of the swap chain frame buffers. This is because
                 * we do not need to change the depth image between frames (in flight), we can just keep clearing and
                 * reusing the same depth image for every frame (see sub pass dependency)
                */
                auto multiSampleImageInfo = getImageInfo (sceneInfo->id.multiSampleImageInfo, MULTI_SAMPLE_IMAGE);
                auto depthImageInfo       = getImageInfo (sceneInfo->id.depthImageInfo,       DEPTH_IMAGE);
                /* Create a frame buffer for all of the images in the swap chain and use the one that corresponds to the
                 * retrieved image at drawing time
                */
                for (uint32_t i = 0; i < deviceInfo->params.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, SWAP_CHAIN_IMAGE);

                    auto attachments = std::vector {
                        multiSampleImageInfo->resource.imageView,
                        depthImageInfo->resource.imageView,
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (deviceInfoId, renderPassInfoId, attachments);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Frame buffer "
                                                   << "[" << renderPassInfoId << "]"
                                                   << " "
                                                   << "[" << deviceInfoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY PIPELINE INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyPipelineInfo (pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VERTEX INPUT                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto bindingDescriptions   = std::vector {
                    getBindingDescription (0, sizeof (Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                };
                auto attributeDescriptions = std::vector {
                    getAttributeDescription (0,
                                             0,
                                             offsetof (Vertex, pos),
                                             VK_FORMAT_R32G32B32_SFLOAT),
                    getAttributeDescription (0,
                                             1,
                                             offsetof (Vertex, texCoord),
                                             VK_FORMAT_R32G32_SFLOAT),
                    getAttributeDescription (0,
                                             2,
                                             offsetof (Vertex, normal),
                                             VK_FORMAT_R32G32B32_SFLOAT),
                    getAttributeDescription (0,
                                             3,
                                             offsetof (Vertex, texId),
                                             VK_FORMAT_R32_UINT)
                };
                createVertexInputState (pipelineInfoId, bindingDescriptions, attributeDescriptions);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - INPUT ASSEMBLY                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createInputAssemblyState (pipelineInfoId,
                                          g_pipelineSettings.inputAssembly.topology,
                                          g_pipelineSettings.inputAssembly.restartEnable);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto vertexShaderModule   = createShaderStage (deviceInfoId,
                                                               pipelineInfoId,
                                                               VK_SHADER_STAGE_VERTEX_BIT,
                                                               g_pipelineSettings.shaderStage.
                                                               vertexShaderBinaryPath,
                                                               "main");

                auto fragmentShaderModule = createShaderStage (deviceInfoId,
                                                               pipelineInfoId,
                                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                                               g_pipelineSettings.shaderStage.
                                                               fragmentShaderBinaryPath,
                                                               "main");
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VIEW PORT                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                createViewPortState (pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - RASTERIZATION                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRasterizationState (pipelineInfoId,
                                          g_pipelineSettings.rasterization.polygonMode,
                                          g_pipelineSettings.rasterization.lineWidth,
                                          g_pipelineSettings.rasterization.cullMode,
                                          g_pipelineSettings.rasterization.frontFace);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - MULTI SAMPLE                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleState (sceneInfo->id.multiSampleImageInfo,
                                        pipelineInfoId,
                                        g_pipelineSettings.multiSample.sampleShadingEnable,
                                        g_pipelineSettings.multiSample.minSampleShading);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - DEPTH STENCIL                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthStencilState (pipelineInfoId,
                                         g_pipelineSettings.depthStencil.depthTestEnable,
                                         g_pipelineSettings.depthStencil.depthWriteEnable,
                                         g_pipelineSettings.depthStencil.depthBoundsTestEnable,
                                         g_pipelineSettings.depthStencil.depthCompareOp,
                                         g_pipelineSettings.depthStencil.minDepthBounds,
                                         g_pipelineSettings.depthStencil.maxDepthBounds,
                                         g_pipelineSettings.depthStencil.stencilTestEnable,
                                         VK_NULL_HANDLE, VK_NULL_HANDLE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - COLOR BLEND                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto colorBlendAttachments = std::vector {
                    getColorBlendAttachment (g_pipelineSettings.colorBlend.blendEnable)
                };
                auto blendConstants = std::vector {
                    g_pipelineSettings.colorBlend.blendConstantR,
                    g_pipelineSettings.colorBlend.blendConstantG,
                    g_pipelineSettings.colorBlend.blendConstantB,
                    g_pipelineSettings.colorBlend.blendConstantA
                };
                createColorBlendState (pipelineInfoId,
                                       g_pipelineSettings.colorBlend.logicOpEnable,
                                       g_pipelineSettings.colorBlend.logicOp,
                                       blendConstants,
                                       colorBlendAttachments);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - DYNAMIC STATES                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto dynamicStates = std::vector {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR
                };
                createDynamicState (pipelineInfoId, dynamicStates);
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
                /* Info on some of the available binding flags
                 * (1) VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                 * This flag indicates that if descriptors in this binding are updated between when the descriptor set is
                 * bound in a command buffer and when that command buffer is submitted to a queue, then the submission
                 * will use the most recently set descriptors for this binding and the updates do not invalidate the
                 * command buffer
                 *
                 * After enabling the desired feature support for updating after bind, an application needs to setup the
                 * following in order to use a descriptor that can update after bind
                 * (a) The VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT flag for any
                 * VkDescriptorSetLayout the descriptor is from
                 * (b) The VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT flag for any VkDescriptorPool the
                 * descriptor is allocated from
                 * (c) The VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT for each binding in the VkDescriptorSetLayout
                 * that the descriptor will use
                 *
                 * More info:
                 * https://docs.vulkan.org/guide/latest/extensions/VK_EXT_descriptor_indexing.html#:~:text=The%20key%20
                 * word%20here%20is,dynamic%20uniform%20indexing%20in%20GLSL
                 *
                 * (2) VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                 * With the partially bound feature an application developer isn’t required to update all the descriptors
                 * at time of use. An example would be if an application’s GLSL has
                 *
                 * layout (set = 0, binding = 0) uniform sampler2D textureSampler[64];
                 *
                 * but only binds the first 32 slots in the array. This also relies on the the application knowing that
                 * it will not index into the unbound slots in the array
                */
                auto perFrameBindingFlags = std::vector <VkDescriptorBindingFlags> {
                    g_pipelineSettings.descriptorSetLayout.bindingFlagsSSBO
                };
                createDescriptorSetLayout (deviceInfoId,
                                           pipelineInfoId,
                                           perFrameLayoutBindings,
                                           perFrameBindingFlags,
                                           g_pipelineSettings.descriptorSetLayout.layoutCreateFlags);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SET LAYOUT - COMMON                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto commonLayoutBindings = std::vector {
                    /* Another commonly used type of descriptor is the combined image sampler, which is a single
                     * descriptor type associated with both a sampler and an image resource, combining both a sampler
                     * and sampled image descriptor into a single descriptor. Note that, it is possible to use texture
                     * sampling in the vertex shader, for example to dynamically deform a grid of vertices by a
                     * heightmap
                    */
                    getLayoutBinding (0,
                                      static_cast <uint32_t> (getTextureImagePool().size()),
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                      VK_NULL_HANDLE)
                };
                auto commonBindingFlags = std::vector <VkDescriptorBindingFlags> {
                    g_pipelineSettings.descriptorSetLayout.bindingFlagsCIS
                };
                createDescriptorSetLayout (deviceInfoId,
                                           pipelineInfoId,
                                           commonLayoutBindings,
                                           commonBindingFlags,
                                           g_pipelineSettings.descriptorSetLayout.layoutCreateFlags);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PUSH CONSTANT RANGES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPushConstantRange (pipelineInfoId,
                                         VK_SHADER_STAGE_VERTEX_BIT,
                                         0,
                                         sizeof (SceneDataVertPC));
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE LAYOUT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPipelineLayout (deviceInfoId, pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createGraphicsPipeline (deviceInfoId,
                                        renderPassInfoId,
                                        pipelineInfoId,
                                        0, -1,
                                        VK_NULL_HANDLE,
                                        g_pipelineSettings.pipelineCreateFlags);

                LOG_INFO (m_VKInitSequenceLog) << "[OK] Pipeline "
                                               << "[" << pipelineInfoId << "]"
                                               << " "
                                               << "[" << renderPassInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SHADER MODULES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't
                 * happen until the graphics pipeline is created. That means that we're allowed to destroy the shader
                 * modules as soon as pipeline creation is finished
                */
                vkDestroyShaderModule (deviceInfo->resource.logDevice, vertexShaderModule,   VK_NULL_HANDLE);
                vkDestroyShaderModule (deviceInfo->resource.logDevice, fragmentShaderModule, VK_NULL_HANDLE);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Shader modules"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE SAMPLER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createTextureSampler (deviceInfoId,
                                      sceneInfoId,
                                      g_textureSamplerSettings.filter,
                                      g_textureSamplerSettings.addressMode,
                                      g_textureSamplerSettings.anisotropyEnable,
                                      g_textureSamplerSettings.mipMapMode,
                                      g_textureSamplerSettings.mipLodBias,
                                      g_textureSamplerSettings.minLod,
                                      g_textureSamplerSettings.maxLod);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture sampler "
                                               << "[" << sceneInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 g_coreSettings.maxFramesInFlight),

                    getPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 static_cast <uint32_t> (getTextureImagePool().size()))
                };
                createDescriptorPool (deviceInfoId,
                                      sceneInfoId,
                                      poolSizes,
                                      g_coreSettings.maxFramesInFlight + 1,
                                      g_descriptorSettings.poolCreateFlags);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor pool "
                                               << "[" << sceneInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS - PER FRAME                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t perFrameDescriptorSetLayoutIdx = 0;
                createDescriptorSets (deviceInfoId,
                                      pipelineInfoId,
                                      sceneInfoId,
                                      perFrameDescriptorSetLayoutIdx,
                                      g_coreSettings.maxFramesInFlight,
                                      PER_FRAME_SET);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS - COMMON                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t commonDescriptorSetLayoutIdx = 1;
                createDescriptorSets (deviceInfoId,
                                      pipelineInfoId,
                                      sceneInfoId,
                                      commonDescriptorSetLayoutIdx,
                                      1,
                                      COMMON_SET);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE - PER FRAME                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < g_coreSettings.maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = sceneInfo->id.storageBufferInfoBase + i;
                    auto bufferInfo              = getBufferInfo (storageBufferInfoId, STORAGE_BUFFER);
                    auto descriptorBufferInfos   = std::vector {
                        getDescriptorBufferInfo (bufferInfo->resource.buffer,
                                                 0,
                                                 sceneInfo->meta.totalInstancesCount * sizeof (InstanceDataSSBO))
                    };

                    /* The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes
                     * an array of VkWriteDescriptorSet structs as parameter
                    */
                    auto writeDescriptorSets = std::vector {
                        getWriteBufferDescriptorSetInfo (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                         sceneInfo->resource.perFrameDescriptorSets[i],
                                                         descriptorBufferInfos,
                                                         0, 0, 1)
                    };

                    updateDescriptorSets (deviceInfoId, writeDescriptorSets);
                }
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor sets "
                                               << "[" << sceneInfoId << "]"
                                               << " "
                                               << "[" << pipelineInfoId << "]"
                                               << " "
                                               << "[" << perFrameDescriptorSetLayoutIdx << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE - COMMON                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t textureCount = static_cast <uint32_t> (getTextureImagePool().size());
                std::vector <VkDescriptorImageInfo> descriptorImageInfos (textureCount);
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    auto imageInfo               = getImageInfo (infoId, TEXTURE_IMAGE);
                    descriptorImageInfos[infoId] = getDescriptorImageInfo (sceneInfo->resource.textureSampler,
                                                                           imageInfo->resource.imageView,
                                                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }

                auto writeDescriptorSets = std::vector {
                    getWriteImageDescriptorSetInfo  (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                     sceneInfo->resource.commonDescriptorSet,
                                                     descriptorImageInfos,
                                                     0, 0, textureCount)
                };

                updateDescriptorSets (deviceInfoId, writeDescriptorSets);

                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor sets "
                                               << "[" << sceneInfoId << "]"
                                               << " "
                                               << "[" << pipelineInfoId << "]"
                                               << " "
                                               << "[" << commonDescriptorSetLayoutIdx << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - COMMAND POOL AND BUFFER                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that the command buffers that we will be submitting to the transfer queue will be short lived, so
                 * we will choose the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag. And, this buffer copy command requires a
                 * queue family that supports transfer operations, which is indicated using VK_QUEUE_TRANSFER_BIT
                */
                auto transferOpsCommandPool = getCommandPool (deviceInfoId,
                                                              VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                              deviceInfo->meta.transferFamilyIndex.value());
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Transfer ops command pool "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;
                /* Note that we are only requesting one command buffer from the pool, since it is recommended to combine
                 * all the transfer operations in a single command buffer and execute them asynchronously for higher
                 * throughput
                */
                auto transferOpsCommandBuffers = std::vector {
                    getCommandBuffers (deviceInfoId, transferOpsCommandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                };
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - FENCE                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t transferOpsFenceInfoId = 0;
                createFence (deviceInfoId, transferOpsFenceInfoId, FEN_TRANSFER_DONE, 0);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Transfer ops fence "
                                               << "[" << transferOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TRANSFER OPS - RECORD AND SUBMIT                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* We're only going to use the command buffer once and wait (vkQueueWaitIdle/vkWaitForFences) until the
                 * copy operation has finished executing. It's good practice to tell the driver about our intent using
                 * VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
                */
                beginRecording (transferOpsCommandBuffers[0],
                                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                VK_NULL_HANDLE);

                for (auto const& [path, infoId]: getTextureImagePool()) {
                    copyBufferToImage (infoId, infoId,
                                       STAGING_BUFFER, TEXTURE_IMAGE,
                                       0,
                                       0,
                                       transferOpsCommandBuffers[0]);
                }

                for (auto const& infoId: modelInfoBase->id.vertexBufferInfos) {
                    copyBufferToBuffer (infoId, infoId,
                                        STAGING_BUFFER, VERTEX_BUFFER,
                                        0, 0,
                                        transferOpsCommandBuffers[0]);
                }

                copyBufferToBuffer (modelInfoBase->id.indexBufferInfo,
                                    modelInfoBase->id.indexBufferInfo,
                                    STAGING_BUFFER, INDEX_BUFFER,
                                    0, 0,
                                    transferOpsCommandBuffers[0]);

                endRecording (transferOpsCommandBuffers[0]);

                VkSubmitInfo transferOpsSubmitInfo{};
                transferOpsSubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                transferOpsSubmitInfo.commandBufferCount = static_cast <uint32_t> (transferOpsCommandBuffers.size());
                transferOpsSubmitInfo.pCommandBuffers    = transferOpsCommandBuffers.data();
                VkResult result = vkQueueSubmit (deviceInfo->resource.transferQueue,
                                                 1,
                                                 &transferOpsSubmitInfo,
                                                 getFenceInfo (transferOpsFenceInfoId, FEN_TRANSFER_DONE)->resource.fence);

                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKInitSequenceLog) << "Failed to submit transfer ops command buffer "
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
                /* Wait for fence
                 * Unlike the draw commands, there are no events we need to wait on. We just want to execute the transfer
                 * on the buffers immediately. There are again two possible ways to wait on this transfer to complete
                 *
                 * (1) We could use a fence and wait with vkWaitForFences, or
                 * (2) Simply wait for the transfer queue to become idle via vkQueueWaitIdle (getTransferQueue());
                 *
                 * A fence would allow you to schedule multiple transfers simultaneously and wait for all of them
                 * complete, instead of executing one at a time. That may give the driver more opportunities to optimize
                */
                LOG_INFO (m_VKInitSequenceLog) << "[WAITING] Transfer ops fence "
                                               << "[" << transferOpsFenceInfoId << "]"
                                               << std::endl;
                vkWaitForFences (deviceInfo->resource.logDevice,
                                 1,
                                 &getFenceInfo (transferOpsFenceInfoId, FEN_TRANSFER_DONE)->resource.fence,
                                 VK_TRUE,
                                 UINT64_MAX);

                vkResetFences   (deviceInfo->resource.logDevice,
                                 1,
                                 &getFenceInfo (transferOpsFenceInfoId, FEN_TRANSFER_DONE)->resource.fence);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Transfer ops fence reset "
                                               << "[" << transferOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY STAGING BUFFERS                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (deviceInfoId, modelInfoBase->id.indexBufferInfo, STAGING_BUFFER);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer "
                                               << "[" << modelInfoBase->id.indexBufferInfo << "]"
                                               << std::endl;

                for (auto const& infoId: modelInfoBase->id.vertexBufferInfos) {
                    VKBufferMgr::cleanUp (deviceInfoId, infoId, STAGING_BUFFER);
                    LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer "
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }

                for (auto const& [path, infoId]: getTextureImagePool()) {
                    VKBufferMgr::cleanUp (deviceInfoId, infoId, STAGING_BUFFER);
                    LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer "
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - FENCE                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                cleanUpFence (deviceInfoId, transferOpsFenceInfoId, FEN_TRANSFER_DONE);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Transfer ops fence "
                                               << "[" << transferOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - COMMAND POOL                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (deviceInfoId, transferOpsCommandPool);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Transfer ops command pool"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG BLIT OPS - COMMAND POOL AND BUFFER                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto blitOpsCommandPool = getCommandPool (deviceInfoId,
                                                          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                          deviceInfo->meta.graphicsFamilyIndex.value());
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Blit ops command pool "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;

                auto blitOpsCommandBuffers = std::vector {
                    getCommandBuffers (deviceInfoId, blitOpsCommandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                };
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG BLIT OPS - FENCE                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t blitOpsFenceInfoId = 0;
                createFence (deviceInfoId, blitOpsFenceInfoId, FEN_BLIT_DONE, 0);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Blit ops fence "
                                               << "[" << blitOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG BLIT OPS - RECORD AND SUBMIT                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                beginRecording (blitOpsCommandBuffers[0],
                                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                VK_NULL_HANDLE);

                for (auto const& [path, infoId]: getTextureImagePool()) {
                    blitImageToMipMaps (infoId, TEXTURE_IMAGE, 0, blitOpsCommandBuffers[0]);
                }

                endRecording (blitOpsCommandBuffers[0]);

                VkSubmitInfo blitOpsSubmitInfo{};
                blitOpsSubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                blitOpsSubmitInfo.commandBufferCount = static_cast <uint32_t> (blitOpsCommandBuffers.size());
                blitOpsSubmitInfo.pCommandBuffers    = blitOpsCommandBuffers.data();
                result = vkQueueSubmit (deviceInfo->resource.graphicsQueue,
                                        1,
                                        &blitOpsSubmitInfo,
                                        getFenceInfo (blitOpsFenceInfoId, FEN_BLIT_DONE)->resource.fence);

                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKInitSequenceLog) << "Failed to submit blit ops command buffer "
                                                    << "[" << deviceInfoId << "]"
                                                    << " "
                                                    << "[" << string_VkResult (result) << "]"
                                                    << std::endl;
                    throw std::runtime_error ("Failed to submit blit ops command buffer");
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG BLIT OPS - WAIT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                LOG_INFO (m_VKInitSequenceLog) << "[WAITING] Blit ops fence "
                                               << "[" << blitOpsFenceInfoId << "]"
                                               << std::endl;
                vkWaitForFences (deviceInfo->resource.logDevice,
                                 1,
                                 &getFenceInfo (blitOpsFenceInfoId, FEN_BLIT_DONE)->resource.fence,
                                 VK_TRUE,
                                 UINT64_MAX);

                vkResetFences   (deviceInfo->resource.logDevice,
                                 1,
                                 &getFenceInfo (blitOpsFenceInfoId, FEN_BLIT_DONE)->resource.fence);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Blit ops fence reset "
                                               << "[" << blitOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY BLIT OPS - FENCE                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                cleanUpFence (deviceInfoId, blitOpsFenceInfoId, FEN_BLIT_DONE);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Blit ops fence "
                                               << "[" << blitOpsFenceInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY BLIT OPS - COMMAND POOL                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (deviceInfoId, blitOpsCommandPool);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Blit ops command pool"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - COMMAND POOL AND BUFFERS                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* We will be recording a command buffer every frame, so we want to be able to reset and rerecord over
                 * it. Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command
                 * pool. And, we're going to record commands for drawing, which is why we've chosen the graphics queue
                 * family
                */
                auto drawOpsCommandPool = getCommandPool (deviceInfoId,
                                                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                                          deviceInfo->meta.graphicsFamilyIndex.value());
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops command pool "
                                               << "[" << deviceInfoId << "]"
                                               << std::endl;

                auto drawOpsCommandBuffers = std::vector {
                    getCommandBuffers (deviceInfoId,
                                       drawOpsCommandPool,
                                       g_coreSettings.maxFramesInFlight,
                                       VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                };

                sceneInfo->resource.commandPool    = drawOpsCommandPool;
                sceneInfo->resource.commandBuffers = drawOpsCommandBuffers;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DRAW OPS - FENCE AND SEMAPHORES                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* We'll need one fence to make sure only one frame is rendering at a time, one semaphore to signal that
                 * an image has been acquired from the swap chain and is ready for rendering, another one to signal that
                 * rendering has finished and presentation can happen, but since we can handle multiple frames in flight,
                 * each frame should have its own set of semaphores and fence
                */
                for (uint32_t i = 0; i < g_coreSettings.maxFramesInFlight; i++) {
                    /* On the very first frame, we immediately wait on in flight fence to be signaled. This fence is only
                     * signaled after a frame has finished rendering, yet since this is the first frame, there are no
                     * previous frames in which to signal the fence! Thus vkWaitForFences() blocks indefinitely, waiting
                     * on something which will never happen. To combat this, create the fence in the signaled state, so
                     * that the first call to vkWaitForFences() returns immediately since the fence is already signaled
                    */
                    uint32_t drawOpsInFlightFenceInfoId = sceneInfo->id.inFlightFenceInfoBase + i;
                    createFence (deviceInfoId, drawOpsInFlightFenceInfoId, FEN_IN_FLIGHT, VK_FENCE_CREATE_SIGNALED_BIT);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops fence "
                                                   << "[" << drawOpsInFlightFenceInfoId << "]"
                                                   << std::endl;

                    uint32_t drawOpsImageAvailableSemaphoreInfoId = sceneInfo->id.imageAvailableSemaphoreInfoBase + i;
                    createSemaphore (deviceInfoId, drawOpsImageAvailableSemaphoreInfoId, SEM_IMAGE_AVAILABLE);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops semaphore "
                                                   << "[" << drawOpsImageAvailableSemaphoreInfoId << "]"
                                                   << std::endl;

                    uint32_t drawOpsRenderDoneSemaphoreInfoId = sceneInfo->id.renderDoneSemaphoreInfoBase + i;
                    createSemaphore (deviceInfoId, drawOpsRenderDoneSemaphoreInfoId, SEM_RENDER_DONE);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops semaphore "
                                                   << "[" << drawOpsRenderDoneSemaphoreInfoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG EXTENSIONS                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                extensions();
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpDeviceInfoPool();
                dumpModelInfoPool();
                dumpImageInfoPool();
                dumpBufferInfoPool();
                dumpRenderPassInfoPool();
                dumpPipelineInfoPool();
                dumpFenceInfoPool();
                dumpSemaphoreInfoPool();
                dumpSceneInfoPool();
            }
    };
}   // namespace Core
#endif  // VK_INIT_SEQUENCE_H