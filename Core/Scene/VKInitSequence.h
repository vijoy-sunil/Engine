#ifndef VK_INIT_SEQUENCE_H
#define VK_INIT_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Device/VKInstance.h"
#include "../Device/VKSurface.h"
#include "../Device/VKLogDevice.h"
#include "../Image/VKSwapChainImage.h"
#include "../Image/VKTextureImage.h"
#include "../Image/VKDepthImage.h"
#include "../Image/VKMultiSampleImage.h"
#include "../Buffer/VKVertexBuffer.h"
#include "../Buffer/VKIndexBuffer.h"
#include "../Buffer/VKUniformBuffer.h"
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
#include "../Pipeline/VKPipelineLayout.h"
#include "../Model/VKTextureSampler.h"
#include "../Model/VKDescriptor.h"

using namespace Collections;

namespace Renderer {
    class VKInitSequence: protected virtual VKWindow,
                          protected virtual VKInstance,
                          protected virtual VKSurface,
                          protected virtual VKLogDevice,
                          protected VKSwapChainImage,
                          protected VKTextureImage,
                          protected VKDepthImage,
                          protected VKMultiSampleImage,                   
                          protected VKVertexBuffer,
                          protected VKIndexBuffer,
                          protected virtual VKUniformBuffer,
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
                          protected VKPipelineLayout,
                          protected virtual VKTextureSampler,
                          protected virtual VKDescriptor {
        private:
            /* Set upper bound lod for the texture sampler It is recommended that to sample from the entire mipmap chain, 
             * set minLod to 0.0, and set maxLod to a level of detail high enough that the computed level of detail will 
             * never be clamped. Assuming the standard approach of halving the dimensions of a texture for each miplevel, 
             * a max lod of 13 would be appropriate for a 4096x4096 source texture
            */
            const float m_maxLod = 13.0;

            static Log::Record* m_VKInitSequenceLog;
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKInitSequence (void) {
                m_VKInitSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
            }

            ~VKInitSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void runSequence (uint32_t modelInfoId, 
                              uint32_t renderPassInfoId,
                              uint32_t pipelineInfoId, 
                              uint32_t resourceId) {

                auto modelInfo  = getModelInfo (modelInfoId);
                auto deviceInfo = getDeviceInfo();
#if DEBUG_DIASBLE
                LOG_INFO (m_VKInitSequenceLog) << "Disabling validation layers and logging" 
                                               << std::endl;
                disableValidationLayers();
                LOG_CLEAR_ALL_CONFIGS;
#else
                enableValidationLayers();
#endif
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG WINDOW                                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                initWindow (resourceId, g_windowSettings.width, g_windowSettings.height);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Window " 
                                               << "[" << resourceId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INSTANCE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                createInstance();
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Instance" 
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEBUG MESSENGER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                setupDebugMessenger();
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Debug messenger" 
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SURFACE                                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */          
                createSurface (resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Surface " 
                                               << "[" << resourceId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PHY DEVICE                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                pickPhysicalDevice (resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Phy device " 
                                               << "[" << resourceId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG LOG DEVICE                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                createLogicalDevice (resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Log device " 
                                               << "[" << resourceId << "]"
                                               << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SWAP CHAIN RESOURCES                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                createSwapChainResources (modelInfo->id.swapChainImageInfo, resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Swap chain resources " 
                                               << "[" << modelInfo->id.swapChainImageInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl;   
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE RESOURCES                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                createTextureResources (modelInfo->id.textureImageInfo, 
                                        resourceId, 
                                        modelInfo->meta.textureImagePath);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture resources " 
                                               << "[" << modelInfo->id.textureImageInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DEPTH RESOURCES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthResources (modelInfo->id.depthImageInfo, resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Depth resources " 
                                               << "[" << modelInfo->id.depthImageInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG MULTI SAMPLE RESOURCES                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleResources (modelInfo->id.multiSampleImageInfo, resourceId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Multi sample resources " 
                                               << "[" << modelInfo->id.multiSampleImageInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl;    
                /* |------------------------------------------------------------------------------------------------|
                 * | IMPORT MODEL                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                importOBJModel (modelInfoId);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Import model " 
                                               << "[" << modelInfoId << "]"
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG VERTEX BUFFER                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                createVertexBuffer (modelInfo->id.vertexBufferInfo, 
                                    resourceId,
                                    modelInfo->meta.uniqueVerticesCount * sizeof (modelInfo->resource.vertices[0]),
                                    modelInfo->resource.vertices.data());
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Vertex buffer " 
                                               << "[" << modelInfo->id.vertexBufferInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG INDEX BUFFER                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                createIndexBuffer (modelInfo->id.indexBufferInfo, 
                                   resourceId,
                                   modelInfo->meta.indicesCount * sizeof (modelInfo->resource.indices[0]),
                                   modelInfo->resource.indices.data());
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Index buffer " 
                                               << "[" << modelInfo->id.indexBufferInfo << "]"
                                               << " "
                                               << "[" << resourceId << "]"
                                               << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG UNIFORM BUFFERS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Define the data we want the vertex shader to have in a C struct like below. This data will be copied 
                 * to a VkBuffer and accessible through a uniform buffer object descriptor from the vertex shader. We can 
                 * exactly match the definition in the shader using data types in GLM. The data in the matrices is binary 
                 * compatible with the way the shader expects it, so we can later just memcpy a UniformBufferObject to a 
                 * VkBuffer
                 * 
                 * Alignment requirements specifies how exactly the data in the C++ structure should match with the 
                 * uniform definition in the shader. Vulkan expects the data in your structure to be aligned in memory in 
                 * a specific way, for example:
                 * 
                 * (1) Scalars have to be aligned by N (= 4 bytes given 32 bit floats)
                 * (2) A vec2 must be aligned by 2N (= 8 bytes)
                 * (3) A vec3 or vec4 must be aligned by 4N (= 16 bytes)
                 * (4) A nested structure must be aligned by the base alignment of its members rounded up to a multiple 
                 * of 16
                 * (5) A mat4 matrix must have the same alignment as a vec4
                 * 
                 * An example to show where alignment requirement are met and not met:
                 * 
                 * A shader with just three mat4 fields already meets the alignment requirements 
                 * struct UniformBufferObject {
                 *      glm::mat4 model;
                 *      glm::mat4 view;
                 *      glm::mat4 proj;
                 * };
                 * 
                 * As each mat4 is 4 x 4 x 4 = 64 bytes in size, model has an offset of 0, view has an offset of 64 and 
                 * proj has an offset of 128. All of these are multiples of 16 and that's why it will work fine. Whereas 
                 * the below struct fails alignment requirements,
                 * struct UniformBufferObject {
                 *      glm::vec2 foo;
                 *      glm::mat4 model;
                 *      glm::mat4 view;
                 *      glm::mat4 proj;
                 * };
                 * 
                 * The new structure starts with a vec2 which is only 8 bytes in size and therefore throws off all of the 
                 * offsets. Now model has an offset of 8, view an offset of 72 and proj an offset of 136, none of which 
                 * are multiples of 16
                 * 
                 * To fix this problem we can use the alignas specifier introduced in C++11:
                 * struct UniformBufferObject {
                 *      glm::vec2 foo;
                 *      alignas (16) glm::mat4 model;
                 *      glm::mat4 view;
                 *      glm::mat4 proj;
                 * };
                 * 
                 * Luckily there is a way to not have to think about these alignment requirements most of the time. We 
                 * can define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES right before including GLM. This will force GLM to use a 
                 * version of vec2 and mat4 that has the alignment requirements already specified for us. If you add this
                 * definition then you can remove the alignas specifier. Unfortunately this method can break down if you 
                 * start using nested structures. Consider the following definition in the C++ code:
                 * struct Foo {
                 *      glm::vec2 v;
                 * };
                 *  
                 * struct UniformBufferObject {
                 *      Foo f1;
                 *      Foo f2;
                 * };
                 * 
                 * And the following shader definition:
                 * struct Foo {
                 *      vec2 v;
                 * };
                 *  
                 * layout (binding = 0) uniform UniformBufferObject {
                 *      Foo f1;
                 *      Foo f2;
                 * } ubo;
                 * 
                 * In this case f2 will have an offset of 8 whereas it should have an offset of 16 since it is a nested 
                 * structure. In this case you must specify the alignment yourself
                 * struct UniformBufferObject {
                 *      Foo f1;
                 *      alignas (16) Foo f2;
                 * };
                 * 
                 * These gotchas are a good reason to always be explicit about alignment. That way you won't be caught 
                 * offguard by the strange symptoms of alignment error
                */
                struct MVPMatrixUBO {
                    alignas (16) glm::mat4 model;
                    alignas (16) glm::mat4 view;
                    alignas (16) glm::mat4 proj;
                };
                /* We should have multiple uniform buffers, because multiple frames may be in flight at the same time and
                 * we don't want to update the buffer in preparation of the next frame while a previous one is still 
                 * reading from it. Thus, we need to have as many uniform buffers as we have frames in flight, and write 
                 * to a uniform buffer that is not currently being read by the GPU
                */
                for (size_t i = 0; i < g_maxFramesInFlight; i++) { 
                    uint32_t uniformBufferInfoId = modelInfo->id.uniformBufferInfo + i;     
                    createUniformBuffer (uniformBufferInfoId,
                                         resourceId,
                                         sizeof (MVPMatrixUBO));

                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Uniform buffer " 
                                                   << "[" << uniformBufferInfoId << "]"
                                                   << " "
                                                   << "[" << resourceId << "]"
                                                   << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS ATTACHMENTS                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyRenderPassInfo (renderPassInfoId);

                createMultiSampleAttachment  (renderPassInfoId, modelInfo->id.multiSampleImageInfo);
                createDepthStencilAttachment (renderPassInfoId, modelInfo->id.depthImageInfo);
                createResolveAttachment      (renderPassInfoId, modelInfo->id.swapChainImageInfo);
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
                               depthStencilAttachmentRef,
                               resolveAttachmentRefs);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS DEPENDENCIES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */                
                createDepthStencilDependency (renderPassInfoId, VK_SUBPASS_EXTERNAL, 0);
                createColorWriteDependency   (renderPassInfoId, VK_SUBPASS_EXTERNAL, 0);

                createRenderPass (renderPassInfoId);
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
                 * Note that, we are using the same depth image on each of the swapchain framebuffers. This is because 
                 * we do not need to change the depth image between frames (in flight), we can just keep clearing and 
                 * reusing the same depth image for every frame (see subpass dependency)
                */
                auto multiSampleImageInfo = getImageInfo (modelInfo->id.multiSampleImageInfo,   MULTISAMPLE_IMAGE);
                auto depthImageInfo       = getImageInfo (modelInfo->id.depthImageInfo,         DEPTH_IMAGE);
                /* Create a framebuffer for all of the images in the swap chain and use the one that corresponds to the 
                 * retrieved image at drawing time
                */
                for (uint32_t i = 0; i < deviceInfo->unique[resourceId].swapChain.size; i++) {
                    uint32_t swapChainImageInfoId = modelInfo->id.swapChainImageInfo + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, SWAPCHAIN_IMAGE);

                    auto attachments = std::vector {
                        multiSampleImageInfo->resource.imageView,
                        depthImageInfo->resource.imageView,
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (renderPassInfoId, resourceId, attachments);                    
                    LOG_INFO (m_VKInitSequenceLog) << "[OK] Frame buffer " 
                                                   << "[" << renderPassInfoId << "]"
                                                   << " "
                                                   << "[" << resourceId << "]"
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
                                             offsetof(Vertex, pos),
                                             VK_FORMAT_R32G32B32_SFLOAT),
                    getAttributeDescription (0,
                                             1,
                                             offsetof(Vertex, color),
                                             VK_FORMAT_R32G32B32_SFLOAT),
                    getAttributeDescription (0,
                                             2,
                                             offsetof(Vertex, texCoord),
                                             VK_FORMAT_R32G32_SFLOAT),
                    getAttributeDescription (0,
                                             3,
                                             offsetof(Vertex, normal),
                                             VK_FORMAT_R32G32B32_SFLOAT)
                };
                createVertexInputState   (pipelineInfoId, bindingDescriptions, attributeDescriptions);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - INPUT ASSEMBLY                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createInputAssemblyState (pipelineInfoId, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - SHADERS                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VkShaderModule vertexShaderModule;
                createShaderStage        (pipelineInfoId, 
                                          VK_SHADER_STAGE_VERTEX_BIT, 
                                          modelInfo->meta.vertexShaderBinaryPath,
                                          "main",
                                          vertexShaderModule);

                VkShaderModule fragmentShaderModule;
                createShaderStage        (pipelineInfoId, 
                                          VK_SHADER_STAGE_FRAGMENT_BIT, 
                                          modelInfo->meta.fragmentShaderBinaryPath,
                                          "main",
                                          fragmentShaderModule);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - VIEW PORT                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                createViewPortState      (pipelineInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - RASTERIZATION                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRasterizationState (pipelineInfoId, 
                                          VK_POLYGON_MODE_FILL,
                                          1.0f,
                                          VK_CULL_MODE_BACK_BIT,
                                          VK_FRONT_FACE_COUNTER_CLOCKWISE);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - MULTI SAMPLE                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                createMultiSampleState   (pipelineInfoId,
                                          modelInfo->id.multiSampleImageInfo,
                                          VK_TRUE, 0.2f);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE STATE - DEPTH STENCIL                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDepthStencilState  (pipelineInfoId,
                                          VK_TRUE,
                                          VK_TRUE,
                                          VK_FALSE, 0.0f, 1.0f,
                                          VK_FALSE, {},   {});
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
                createColorBlendState    (pipelineInfoId,
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
                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      VK_SHADER_STAGE_VERTEX_BIT,
                                      nullptr),
                    /* Another type of descriptor is the combined image sampler. This descriptor makes it possible for
                     * shaders to access an image resource through a sampler object 
                     *
                     * Note that, we intend to use the combined image sampler descriptor in the fragment shader. That's 
                     * where the color of the fragment is going to be determined. It is possible to use texture sampling
                     * in the vertex shader, for example to dynamically deform a grid of vertices by a heightmap
                    */
                    getLayoutBinding (1, 
                                      1,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                      nullptr)
                };
                createDescriptorSetLayout (pipelineInfoId, layoutBindings);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG PIPELINE LAYOUT                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createPipelineLayout      (pipelineInfoId);
                createGraphicsPipeline    (pipelineInfoId, 
                                           renderPassInfoId, 0,
                                           -1, VK_NULL_HANDLE);

                LOG_INFO (m_VKInitSequenceLog) << "[OK] Pipeline " 
                                               << "[" << pipelineInfoId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SHADER MODULES                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't 
                 * happen until the graphics pipeline is created. That means that we're allowed to destroy the shader 
                 * modules as soon as pipeline creation is finished
                */
                vkDestroyShaderModule (deviceInfo->shared.logDevice, vertexShaderModule,   nullptr);
                vkDestroyShaderModule (deviceInfo->shared.logDevice, fragmentShaderModule, nullptr);
                LOG_INFO (m_VKInitSequenceLog) << "[DELETE] Shader modules " 
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE SAMPLER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createTextureSampler (modelInfoId, 
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_TRUE,
                                      VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                      0.0, m_maxLod);
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Texture sampler " 
                                               << "[" << modelInfoId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         static_cast <uint32_t> (g_maxFramesInFlight)),
                    getPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast <uint32_t> (g_maxFramesInFlight))
                };
                createDescriptorPool (modelInfoId, 
                                      poolSizes, 
                                      static_cast <uint32_t> (g_maxFramesInFlight));
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor pool " 
                                               << "[" << modelInfoId << "]"
                                               << std::endl;  
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createDescriptorSets (modelInfoId, 
                                      pipelineInfoId, 
                                      0, 
                                      static_cast <uint32_t> (g_maxFramesInFlight));
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR SETS UPDATE                                                                  |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (size_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t uniformBufferInfoId = modelInfo->id.uniformBufferInfo + i;
                    auto bufferInfo              = getBufferInfo (uniformBufferInfoId, UNIFORM_BUFFER);
                    auto descriptorBufferInfo    = getDescriptorBufferInfo (bufferInfo->resource.buffer,
                                                                            0,
                                                                            bufferInfo->meta.size);

                    uint32_t textureImageInfoId  = modelInfo->id.textureImageInfo;
                    auto imageInfo               = getImageInfo (textureImageInfoId, TEXTURE_IMAGE);
                    auto descriptorImageInfo     = getDescriptorImageInfo  (modelInfo->resource.textureSampler,
                                                                            imageInfo->resource.imageView,
                                                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    /* The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes 
                     * an array of VkWriteDescriptorSet structs as parameter
                    */                    
                    auto descriptorWrites = std::vector {
                        getWriteBufferDescriptorSetInfo (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                         modelInfo->resource.descriptorSets[i],
                                                         descriptorBufferInfo,
                                                         0, 0, 1),

                        getWriteImageDescriptorSetInfo  (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         modelInfo->resource.descriptorSets[i],
                                                         descriptorImageInfo,
                                                         1, 0, 1)
                    };

                    updateDescriptorSets (descriptorWrites);
                }
                LOG_INFO (m_VKInitSequenceLog) << "[OK] Descriptor sets " 
                                               << "[" << modelInfoId << "]"
                                               << " "
                                               << "[" << pipelineInfoId << "]"
                                               << std::endl;

                dumpModelInfoPool();
                dumpDeviceInfoPool();
                dumpImageInfoPool();
                dumpBufferInfoPool(); 
                dumpRenderPassInfoPool();  
                dumpPipelineInfoPool();   
            }
    };

    Log::Record* VKInitSequence::m_VKInitSequenceLog;
}   // namespace Renderer
#endif  // VK_INIT_SEQUENCE_H