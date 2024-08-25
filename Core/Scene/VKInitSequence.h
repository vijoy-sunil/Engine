#ifndef VK_INIT_SEQUENCE_H
#define VK_INIT_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Device/VKInstance.h"
#include "../Device/VKSurface.h"
#include "../Device/VKLogDevice.h"
#include "../Model/VKModelMgr.h"
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
#include "VKCameraMgr.h"
#include "VKTextureSampler.h"
#include "VKDescriptor.h"
#include "VKSyncObject.h"

using namespace Collections;

namespace Core {
    class VKInitSequence: protected virtual VKWindow,
                          protected virtual VKInstance,
                          protected virtual VKSurface,
                          protected virtual VKLogDevice,
                          protected virtual VKModelMgr,
                          protected virtual VKSwapChainImage,
                          protected VKTextureImage,
                          protected virtual VKDepthImage,
                          protected virtual VKMultiSampleImage,
                          protected VKVertexBuffer,
                          protected VKIndexBuffer,
                          protected virtual VKStorageBuffer,
                          protected VKAttachment,
                          protected VKSubPass,
                          protected virtual VKFrameBuffer,
                          protected VKVertexInput,
                          protected VKInputAssembly,
                          protected VKShaderStage,
                          protected VKViewPort,
                          protected VKRasterization,
                          protected VKMultiSample,
                          protected VKDepthStencil,
                          protected VKColorBlend,
                          protected VKDynamicState,
                          protected VKDescriptorSetLayout,
                          protected VKPushConstantRange,
                          protected VKPipelineLayout,
                          protected virtual VKCmdBuffer,
                          protected virtual VKCmd,
                          protected virtual VKCameraMgr,
                          protected virtual VKTextureSampler,
                          protected virtual VKDescriptor,
                          protected virtual VKSyncObject {
        private:
            /* Set upper bound lod for the texture sampler. It is recommended that to sample from the entire mipmap chain, 
             * set min lod to 0.0, and set max lod to a level of detail high enough that the computed level of detail will 
             * never be clamped. Assuming the standard approach of halving the dimensions of a texture for each miplevel, 
             * a max lod of 13 would be appropriate for a 4096x4096 source texture
            */
            const float m_maxLod = 13.0f;

            static Log::Record* m_VKInitSequenceLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKInitSequence (void) {
                m_VKInitSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKInitSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void runSequence (const std::vector <uint32_t>& modelInfoIds,
                              uint32_t renderPassInfoId,
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId, 
                              uint32_t sceneInfoId, 
                              uint32_t deviceInfoId) {

                auto modelInfoBase = getModelInfo  (*modelInfoIds.begin());
                auto sceneInfo     = getSceneInfo  (sceneInfoId);
                auto deviceInfo    = getDeviceInfo (deviceInfoId);
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
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Import model " 
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SWAP CHAIN RESOURCES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSwapChainResources (sceneInfo->id.swapChainImageInfoBase, deviceInfoId);
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
                    createTextureResources (infoId, deviceInfoId, path.c_str());
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture resources " 
                                                   << "[" << infoId << "]"
                                                   << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEPTH RESOURCES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthResources (sceneInfo->id.depthImageInfo, deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Depth resources " 
                                               << "[" << sceneInfo->id.depthImageInfo << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG MULTI SAMPLE RESOURCES                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleResources (sceneInfo->id.multiSampleImageInfo, deviceInfoId);
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
   
                    infoId == *modelInfoIds.begin() ? modelInfo->id.vertexBufferInfos.push_back (vertexBufferInfoId):
                                                      modelInfo->id.vertexBufferInfos.push_back (UINT32_MAX);
                }
                
                createVertexBuffer (vertexBufferInfoId, 
                                    deviceInfoId,
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
                    
                    infoId == *modelInfoIds.begin() ? modelInfo->id.indexBufferInfo = indexBufferInfoId:
                                                      modelInfo->id.indexBufferInfo = UINT32_MAX;
                }

                createIndexBuffer (indexBufferInfoId, 
                                   deviceInfoId,
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
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) { 
                    uint32_t storageBufferInfoId = sceneInfo->id.storageBufferInfoBase + i;    
                    createStorageBuffer (storageBufferInfoId,
                                         deviceInfoId,
                                         sceneInfo->meta.totalInstancesCount * sizeof (InstanceDataSSBO));

                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Storage buffer " 
                                                   << "[" << storageBufferInfoId << "]"
                                                   << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS ATTACHMENTS                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyRenderPassInfo (renderPassInfoId);

                createMultiSampleAttachment  (renderPassInfoId, sceneInfo->id.multiSampleImageInfo);
                createDepthStencilAttachment (renderPassInfoId, sceneInfo->id.depthImageInfo);
                createResolveAttachment      (renderPassInfoId, sceneInfo->id.swapChainImageInfoBase);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto colorAttachmentRefs = std::vector {
                    getAttachmentReference (0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                };
                auto depthStencilAttachmentRef = 
                    getAttachmentReference (1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                auto resolveAttachmentRefs = std::vector {
                    getAttachmentReference (2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                };
                createSubPass (renderPassInfoId,
                               colorAttachmentRefs,
                               &depthStencilAttachmentRef,
                               resolveAttachmentRefs);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS DEPENDENCIES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */                
                createDepthStencilDependency (renderPassInfoId, VK_SUBPASS_EXTERNAL, 0);
                createColorWriteDependency   (renderPassInfoId, VK_SUBPASS_EXTERNAL, 0);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRenderPass (renderPassInfoId, deviceInfoId);
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
                 * to store more than one sample per pixel. Once a multisampled buffer is created, it has to be 
                 * resolved to the default framebuffer (which stores only a single sample per pixel). This is why we 
                 * have to create an additional render target. We only need one render target since only one drawing 
                 * operation is active at a time, just like with the depth buffer
                 * 
                 * Note that, we are using the same depth image on each of the swap chain framebuffers. This is because 
                 * we do not need to change the depth image between frames (in flight), we can just keep clearing and 
                 * reusing the same depth image for every frame (see subpass dependency)
                */
                auto multiSampleImageInfo = getImageInfo (sceneInfo->id.multiSampleImageInfo, MULTISAMPLE_IMAGE);
                auto depthImageInfo       = getImageInfo (sceneInfo->id.depthImageInfo,       DEPTH_IMAGE);
                /* Create a framebuffer for all of the images in the swap chain and use the one that corresponds to the 
                 * retrieved image at drawing time
                */
                for (uint32_t i = 0; i < deviceInfo->meta.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, SWAPCHAIN_IMAGE);

                    auto attachments = std::vector {
                        multiSampleImageInfo->resource.imageView,
                        depthImageInfo->resource.imageView,
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (renderPassInfoId, deviceInfoId, attachments);                    
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Frame buffer " 
                                                   << "[" << renderPassInfoId << "]"
                                                   << " "
                                                   << "[" << deviceInfoId << "]"
                                                   << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VERTEX INPUT                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyPipelineInfo (pipelineInfoId);

                auto bindingDescriptions = std::vector {
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
                createInputAssemblyState (pipelineInfoId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto vertexShaderModule   = createShaderStage (pipelineInfoId, 
                                                               deviceInfoId,
                                                               VK_SHADER_STAGE_VERTEX_BIT, 
                                                               g_pathSettings.vertexShaderBinary, "main");

                auto fragmentShaderModule = createShaderStage (pipelineInfoId, 
                                                               deviceInfoId,
                                                               VK_SHADER_STAGE_FRAGMENT_BIT, 
                                                               g_pathSettings.fragmentShaderBinary, "main");
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
                                          VK_POLYGON_MODE_FILL,
                                          1.0f,
                                          VK_CULL_MODE_NONE,
                                          VK_FRONT_FACE_CLOCKWISE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - MULTI SAMPLE                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleState (pipelineInfoId,
                                        sceneInfo->id.multiSampleImageInfo,
                                        VK_TRUE, 0.2f);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - DEPTH STENCIL                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthStencilState (pipelineInfoId,
                                         VK_TRUE,
                                         VK_TRUE,
                                         VK_FALSE, 0.0f, 1.0f,
                                         VK_FALSE, 
                                         VK_NULL_HANDLE, VK_NULL_HANDLE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - COLOR BLEND                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto colorBlendAttachments = std::vector {
                    getColorBlendAttachment (VK_FALSE)
                };
                auto blendConstants = std::vector {
                    0.0f, 0.0f, 0.0f, 0.0f
                };
                createColorBlendState (pipelineInfoId,
                                       VK_FALSE,
                                       VK_LOGIC_OP_COPY,
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
                 * | CONFIG DESCRIPTOR SET LAYOUT                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto layoutBindings = std::vector {
                    getLayoutBinding (0, 
                                      1,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                      VK_SHADER_STAGE_VERTEX_BIT,
                                      VK_NULL_HANDLE),

                    /* Another commonly used type of descriptor is the combined image sampler, which is a single 
                     * descriptor type associated with both a sampler and an image resource, combining both a sampler 
                     * and sampled image descriptor into a single descriptor. Note that, it is possible to use texture 
                     * sampling in the vertex shader, for example to dynamically deform a grid of vertices by a 
                     * heightmap
                    */
                    getLayoutBinding (1,
                                      static_cast <uint32_t> (getTextureImagePool().size()),
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      VK_SHADER_STAGE_FRAGMENT_BIT,
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
                auto bindingFlags = std::vector <VkDescriptorBindingFlags> {
                    0,
                    0
                };
                createDescriptorSetLayout (pipelineInfoId, deviceInfoId, layoutBindings, bindingFlags, 0);
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
                createPipelineLayout (pipelineInfoId, deviceInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createGraphicsPipeline (pipelineInfoId, 
                                        renderPassInfoId, 
                                        deviceInfoId, 
                                        0, -1, 
                                        VK_NULL_HANDLE);

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
                 * | CONFIG CAMERA MATRIX                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                createCameraMatrix (cameraInfoId, deviceInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Camera matrix " 
                                               << "[" << cameraInfoId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE SAMPLER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createTextureSampler (sceneInfoId, 
                                      deviceInfoId,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_TRUE,
                                      VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                      0.0f, m_maxLod);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture sampler " 
                                               << "[" << sceneInfoId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, g_maxFramesInFlight),

                    getPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast <uint32_t> 
                                (getTextureImagePool().size()) * g_maxFramesInFlight)
                };
                createDescriptorPool (sceneInfoId, 
                                      deviceInfoId,
                                      poolSizes, 
                                      g_maxFramesInFlight, 
                                      0);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor pool " 
                                               << "[" << sceneInfoId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t descriptorSetLayoutId = 0;
                createDescriptorSets (sceneInfoId, 
                                      pipelineInfoId, 
                                      descriptorSetLayoutId, 
                                      deviceInfoId,
                                      g_maxFramesInFlight);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = sceneInfo->id.storageBufferInfoBase + i; 
                    auto bufferInfo              = getBufferInfo (storageBufferInfoId, STORAGE_BUFFER);
                    auto descriptorBufferInfos   = std::vector {
                        getDescriptorBufferInfo (bufferInfo->resource.buffer,
                                                 0,
                                                 sceneInfo->meta.totalInstancesCount * sizeof (InstanceDataSSBO))
                    };

                    uint32_t textureCount = static_cast <uint32_t> (getTextureImagePool().size());
                    std::vector <VkDescriptorImageInfo> descriptorImageInfos (textureCount);
                    for (auto const& [path, infoId]: getTextureImagePool()) {
                        auto imageInfo               = getImageInfo (infoId, TEXTURE_IMAGE);
                        descriptorImageInfos[infoId] = getDescriptorImageInfo (sceneInfo->resource.textureSampler,
                                                                               imageInfo->resource.imageView,
                                                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    }

                    /* The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes 
                     * an array of VkWriteDescriptorSet structs as parameter
                    */                    
                    auto writeDescriptorSets = std::vector {
                        getWriteBufferDescriptorSetInfo (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                         sceneInfo->resource.descriptorSets[i],
                                                         descriptorBufferInfos,
                                                         0, 0, 1),

                        getWriteImageDescriptorSetInfo  (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         sceneInfo->resource.descriptorSets[i],
                                                         descriptorImageInfos,
                                                         1, 0, textureCount)
                    };

                    updateDescriptorSets (deviceInfoId, writeDescriptorSets);
                }
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor sets " 
                                               << "[" << sceneInfoId << "]"
                                               << " "
                                               << "[" << pipelineInfoId << "]"
                                               << " "
                                               << "[" << descriptorSetLayoutId << "]"
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
                createFence (transferOpsFenceInfoId, deviceInfoId, FEN_TRANSFER_DONE, 0);
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
                    copyBufferToImage (transferOpsCommandBuffers[0],
                                       infoId, STAGING_BUFFER_TEX, 0,
                                       infoId, TEXTURE_IMAGE,      
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                }

                for (auto const& infoId: modelInfoBase->id.vertexBufferInfos) {
                    copyBufferToBuffer (transferOpsCommandBuffers[0],
                                        infoId, STAGING_BUFFER, 0,
                                        infoId, VERTEX_BUFFER,  0);
                }

                copyBufferToBuffer (transferOpsCommandBuffers[0],
                                    modelInfoBase->id.indexBufferInfo, STAGING_BUFFER, 0,
                                    modelInfoBase->id.indexBufferInfo, INDEX_BUFFER,   0);

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
                VKBufferMgr::cleanUp (modelInfoBase->id.indexBufferInfo, deviceInfoId, STAGING_BUFFER);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer " 
                                               << "[" << modelInfoBase->id.indexBufferInfo << "]"
                                               << std::endl;  

                for (auto const& infoId: modelInfoBase->id.vertexBufferInfos) {
                    VKBufferMgr::cleanUp (infoId, deviceInfoId, STAGING_BUFFER);
                    LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer "
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }

                for (auto const& [path, infoId]: getTextureImagePool()) {
                    VKBufferMgr::cleanUp (infoId, deviceInfoId, STAGING_BUFFER_TEX);
                    LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Staging buffer (Tex) " 
                                                   << "[" << infoId << "]"
                                                   << std::endl;
                }              
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TRANSFER OPS - FENCE                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                cleanUpFence (transferOpsFenceInfoId, deviceInfoId, FEN_TRANSFER_DONE);
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
                createFence (blitOpsFenceInfoId, deviceInfoId, FEN_BLIT_DONE, 0);
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
                    blitImageToMipMaps (blitOpsCommandBuffers[0], infoId, TEXTURE_IMAGE);
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
                cleanUpFence (blitOpsFenceInfoId, deviceInfoId, FEN_BLIT_DONE);
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
                                       g_maxFramesInFlight, 
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
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    /* On the very first frame, we immediately wait on in flight fence to be signaled. This fence is only 
                     * signaled after a frame has finished rendering, yet since this is the first frame, there are no 
                     * previous frames in which to signal the fence! Thus vkWaitForFences() blocks indefinitely, waiting 
                     * on something which will never happen. To combat this, create the fence in the signaled state, so 
                     * that the first call to vkWaitForFences() returns immediately since the fence is already signaled
                    */
                    uint32_t drawOpsInFlightFenceInfoId = sceneInfo->id.inFlightFenceInfoBase + i;
                    createFence (drawOpsInFlightFenceInfoId, deviceInfoId, FEN_IN_FLIGHT, VK_FENCE_CREATE_SIGNALED_BIT);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops fence " 
                                                   << "[" << drawOpsInFlightFenceInfoId << "]"
                                                   << std::endl;

                    uint32_t drawOpsImageAvailableSemaphoreInfoId = sceneInfo->id.imageAvailableSemaphoreInfoBase + i;
                    createSemaphore (drawOpsImageAvailableSemaphoreInfoId, deviceInfoId, SEM_IMAGE_AVAILABLE);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops semaphore " 
                                                   << "[" << drawOpsImageAvailableSemaphoreInfoId << "]"
                                                   << std::endl;

                    uint32_t drawOpsRenderDoneSemaphoreInfoId = sceneInfo->id.renderDoneSemaphoreInfoBase + i;
                    createSemaphore (drawOpsRenderDoneSemaphoreInfoId, deviceInfoId, SEM_RENDER_DONE);
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Draw ops semaphore " 
                                                   << "[" << drawOpsRenderDoneSemaphoreInfoId << "]"
                                                   << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpModelInfoPool();
                dumpImageInfoPool();
                dumpBufferInfoPool(); 
                dumpRenderPassInfoPool();  
                dumpPipelineInfoPool();  
                dumpCameraInfoPool(); 
                dumpFenceInfoPool();
                dumpSemaphoreInfoPool();
                dumpSceneInfoPool();
                dumpDeviceInfoPool();
            }
    };

    Log::Record* VKInitSequence::m_VKInitSequenceLog;
}   // namespace Core
#endif  // VK_INIT_SEQUENCE_H