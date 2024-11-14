#ifndef VK_PIPELINE_MGR_H
#define VK_PIPELINE_MGR_H

#include "../RenderPass/VKRenderPassMgr.h"

namespace Core {
    /* An overview of the pipeline
     * Vertex/Index Buffer
     *      |
     * Input Assembler      [FIXED FUNCTION]
     * The input assembler collects the raw vertex data from the buffers you specify and may also use an index buffer to
     * repeat certain elements without having to duplicate the vertex data itself
     *      |
     * Vertex Shader        [PROGRAMMABLE]
     * The vertex shader is run for every vertex and generally applies transformations to turn vertex positions from model
     * space to screen space. It also passes per-vertex data (eg: color) down the pipeline
     *      |
     * Tessellation         [PROGRAMMABLE]
     * The tessellation shaders allow you to subdivide geometry based on certain rules to increase the mesh quality
     *      |
     * Geometry Shader      [PROGRAMMABLE]
     * The geometry shader is run on every primitive (triangle, line, point) and can discard it or output more primitives
     * than came in. This is similar to the tessellation shader, but much more flexible. However, it is not used much in
     * today's applications because the performance is not that good on most graphics cards
     *      |
     * Rasterization        [FIXED FUNCTION]
     * The rasterization stage discretizes the primitives into fragments. These are the pixel elements that they fill on
     * the frame buffer. Any fragments that fall outside the screen are discarded and the attributes outputted by the
     * vertex shader are interpolated across the fragments. Usually the fragments that are behind other primitive fragments
     * are also discarded here because of depth testing
     *      |
     * Fragement Shader     [PROGRAMMABLE]
     * The fragment shader is invoked for every fragment that survives and determines which frame buffer(s) the fragments
     * are written to and with which color and depth values
     *      |
     * Color Blending       [FIXED FUNCTION]
     * The color blending stage applies operations to mix different fragments that map to the same pixel in the
     * frame buffer. Fragments can simply overwrite each other, add up or be mixed based upon transparency
     *
     * Fixed function stages allow you to tweak their operations using parameters, but the way they work is predefined.
     * Programmable stages are programmable, which means that you can upload your own code to the graphics card to apply
     * exactly the operations you want
    */
    class VKPipelineMgr: protected virtual VKRenderPassMgr {
        private:
            struct PipelineInfo {
                struct Meta {
                    uint32_t subPassIndex;
                    int32_t basePipelineIndex;
                } meta;

                struct State {
                    VkPipelineVertexInputStateCreateInfo          vertexInput;
                    VkPipelineInputAssemblyStateCreateInfo        inputAssembly;
                    std::vector <VkPipelineShaderStageCreateInfo> stages;
                    VkPipelineDepthStencilStateCreateInfo         depthStencil;
                    VkPipelineRasterizationStateCreateInfo        rasterization;
                    VkPipelineMultisampleStateCreateInfo          multiSample;
                    VkPipelineColorBlendStateCreateInfo           colorBlend;
                    VkPipelineDynamicStateCreateInfo              dynamicState;
                    VkPipelineViewportStateCreateInfo             viewPort;
                } state;

                struct Resource {
                    VkPipelineLayout layout;
                    std::vector <VkDescriptorSetLayout> descriptorSetLayouts;
                    std::vector <VkPushConstantRange>   pushConstantRanges;
                    VkPipeline pipeline;
                    VkRenderPass renderPass;
                    /* Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. The idea
                     * of pipeline derivatives is that it is less expensive to set up pipelines when they have much
                     * functionality in common with an existing pipeline and switching between pipelines from the same
                     * parent can also be done quicker
                    */
                    VkPipeline basePipeline;
                } resource;
            };
            std::unordered_map <uint32_t, PipelineInfo> m_pipelineInfoPool;

            Log::Record* m_VKPipelineMgrLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deletePipelineInfo (uint32_t pipelineInfoId) {
                if (m_pipelineInfoPool.find (pipelineInfoId) != m_pipelineInfoPool.end()) {
                    m_pipelineInfoPool.erase (pipelineInfoId);
                    return;
                }

                LOG_ERROR (m_VKPipelineMgrLog) << "Failed to delete pipeline info "
                                               << "[" << pipelineInfoId << "]"
                                               << std::endl;
                throw std::runtime_error ("Failed to delete pipeline info");
            }

        public:
            VKPipelineMgr (void) {
                m_VKPipelineMgrLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKPipelineMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyPipelineInfo (uint32_t pipelineInfoId) {
                if (m_pipelineInfoPool.find (pipelineInfoId) != m_pipelineInfoPool.end()) {
                    LOG_ERROR (m_VKPipelineMgrLog) << "Pipeline info id already exists "
                                                   << "[" << pipelineInfoId << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Pipeline info id already exists");
                }

                PipelineInfo info{};
                m_pipelineInfoPool[pipelineInfoId] = info;
            }

            void derivePipelineInfo (uint32_t childPipelineInfoId, uint32_t pipelineInfoId) {
                if (m_pipelineInfoPool.find (childPipelineInfoId) != m_pipelineInfoPool.end()) {
                    LOG_ERROR (m_VKPipelineMgrLog) << "Pipeline info id already exists "
                                                   << "[" << childPipelineInfoId << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Pipeline info id already exists");
                }

                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                PipelineInfo info{};
                /* Note that, we wan't to be careful when shallow copying the struct members. Hence, why we are not
                 * copying the resource members
                */
                info.meta  = pipelineInfo->meta;
                info.state = pipelineInfo->state;

                m_pipelineInfoPool[childPipelineInfoId] = info;
            }

            void createGraphicsPipeline (uint32_t deviceInfoId,
                                         uint32_t renderPassInfoId,
                                         uint32_t pipelineInfoId,
                                         uint32_t subPassIndex,
                                         int32_t basePipelineIndex,
                                         VkPipeline basePipeline,
                                         VkPipelineCreateFlags pipelineCreateFlags) {

                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                auto pipelineInfo   = getPipelineInfo   (pipelineInfoId);

                pipelineInfo->meta.subPassIndex      = subPassIndex;
                pipelineInfo->meta.basePipelineIndex = basePipelineIndex;
                pipelineInfo->resource.renderPass    = renderPassInfo->resource.renderPass;
                pipelineInfo->resource.basePipeline  = basePipeline;

                VkGraphicsPipelineCreateInfo createInfo;
                createInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                createInfo.pNext               = VK_NULL_HANDLE;
                createInfo.flags               = pipelineCreateFlags;
                createInfo.pVertexInputState   = &pipelineInfo->state.vertexInput;
                createInfo.pInputAssemblyState = &pipelineInfo->state.inputAssembly;
                createInfo.pTessellationState  = VK_NULL_HANDLE;
                createInfo.stageCount          = static_cast <uint32_t> (pipelineInfo->state.stages.size());
                createInfo.pStages             = pipelineInfo->state.stages.data();
                createInfo.pDepthStencilState  = &pipelineInfo->state.depthStencil;
                createInfo.pRasterizationState = &pipelineInfo->state.rasterization;
                createInfo.pMultisampleState   = &pipelineInfo->state.multiSample;
                createInfo.pColorBlendState    = &pipelineInfo->state.colorBlend;
                createInfo.pDynamicState       = &pipelineInfo->state.dynamicState;
                createInfo.pViewportState      = &pipelineInfo->state.viewPort;

                createInfo.subpass             = pipelineInfo->meta.subPassIndex;
                createInfo.basePipelineIndex   = pipelineInfo->meta.basePipelineIndex;
                createInfo.layout              = pipelineInfo->resource.layout;
                /* Pipeline vs Render pass
                 * VkPipeline is a GPU context. Think of the GPU as a FPGA (which it isn't, but bear with me). Doing
                 * vkCmdBindPipeline would set the GPU to a given gate configuration. But since the GPU is not a FPGA, it
                 * sets the GPU to a state where it can execute the shader programs and fixed-function pipeline stages
                 * defined by the VkPipeline
                 *
                 * VkRenderPass is a data oriented thing. It is necessitated by tiled architecture GPUs. Conceptually,
                 * they divide the frame buffer up into tiles that are processed independently. Tiled-architecture GPUs
                 * need to "load" images\buffers from general-purpose RAM to "on-chip memory". When they are done they
                 * "store" their results back to RAM. This loading of attachments is done by smaller "tiles", so the
                 * on-chip memory (and therefore shaders) never sees the whole memory at the same time
                 *
                 * Loading and storing these tiles is rather slow and a good optimization strategy is to combine as many
                 * operations as possible into one cycle over the whole frame buffer. It's trivial to see that operations
                 * can be combined safely as long as they don't depend on intermediate results from other tiles
                 *
                 * Sub passes and sub pass dependencies tell the GPU drivers where these kinds of dependencies exist
                 * (or don't), so that they can group the actual render calls more effectively under the hood
                 *
                 * Note that, you can have multiple pipeline in a single render pass
                */
                createInfo.renderPass         = renderPassInfo->resource.renderPass;
                createInfo.basePipelineHandle = pipelineInfo->resource.basePipeline;
                /* Create the pipeline
                 * The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation
                 * functions in Vulkan. It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create
                 * multiple VkPipeline objects in a single call.
                 *
                 * The second parameter, for which we've passed the VK_NULL_HANDLE argument, references an optional
                 * VkPipelineCache object. A pipeline cache can be used to store and reuse data relevant to pipeline
                 * creation across multiple calls to vkCreateGraphicsPipelines and even across program executions if the
                 * cache is stored to a file. This makes it possible to significantly speed up pipeline creation at a
                 * later time
                */
                VkPipeline pipeline;
                VkResult result = vkCreateGraphicsPipelines (deviceInfo->resource.logDevice,
                                                             VK_NULL_HANDLE,
                                                             1,
                                                             &createInfo,
                                                             VK_NULL_HANDLE,
                                                             &pipeline);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKPipelineMgrLog) << "Failed to create graphics pipeline "
                                                   << "[" << pipelineInfoId << "]"
                                                   << " "
                                                   << "[" << renderPassInfoId << "]"
                                                   << " "
                                                   << "[" << string_VkResult (result) << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Failed to create graphics pipeline");
                }
                pipelineInfo->resource.pipeline = pipeline;
            }

            PipelineInfo* getPipelineInfo (uint32_t pipelineInfoId) {
                if (m_pipelineInfoPool.find (pipelineInfoId) != m_pipelineInfoPool.end())
                    return &m_pipelineInfoPool[pipelineInfoId];

                LOG_ERROR (m_VKPipelineMgrLog) << "Failed to find pipeline info "
                                               << "[" << pipelineInfoId << "]"
                                               << std::endl;
                throw std::runtime_error ("Failed to find pipeline info");
            }

            void dumpPipelineInfoPool (void) {
                LOG_INFO (m_VKPipelineMgrLog) << "Dumping pipeline info pool"
                                              << std::endl;

                for (auto const& [key, val]: m_pipelineInfoPool) {
                    LOG_INFO (m_VKPipelineMgrLog) << "Pipeline info id "
                                                  << "[" << key << "]"
                                                  << std::endl;

                    LOG_INFO (m_VKPipelineMgrLog) << "Sub pass index "
                                                  << "[" << val.meta.subPassIndex << "]"
                                                  << std::endl;

                    LOG_INFO (m_VKPipelineMgrLog) << "Base pipeline index "
                                                  << "[" << val.meta.basePipelineIndex << "]"
                                                  << std::endl;

                    LOG_INFO (m_VKPipelineMgrLog) << "Descriptor set layouts count "
                                                  << "[" << val.resource.descriptorSetLayouts.size() << "]"
                                                  << std::endl;

                    LOG_INFO (m_VKPipelineMgrLog) << "Push constant ranges count "
                                                  << "[" << val.resource.pushConstantRanges.size() << "]"
                                                  << std::endl;
                }
            }

            void cleanUp (uint32_t deviceInfoId, uint32_t pipelineInfoId) {
                auto deviceInfo   = getDeviceInfo   (deviceInfoId);
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);

                vkDestroyPipeline       (deviceInfo->resource.logDevice, pipelineInfo->resource.pipeline, VK_NULL_HANDLE);
                vkDestroyPipelineLayout (deviceInfo->resource.logDevice, pipelineInfo->resource.layout,   VK_NULL_HANDLE);

                for (auto const& descriptorSetLayout: pipelineInfo->resource.descriptorSetLayouts)
                    vkDestroyDescriptorSetLayout (deviceInfo->resource.logDevice, descriptorSetLayout, VK_NULL_HANDLE);
                pipelineInfo->resource.descriptorSetLayouts.clear();

                deletePipelineInfo (pipelineInfoId);
            }
    };
}   // namespace Core
#endif  // VK_PIPELINE_MGR_H
