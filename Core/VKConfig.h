#ifndef VK_CONFIG_H
#define VK_CONFIG_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core {
    #define ENABLE_LOGGING                                           (true)
    #define ENABLE_AUTO_PICK_QUEUE_FAMILY_INDICES                    (true)

    struct CollectionSettings {
        /* Collection instance id range assignments
         * Reserved     [0]
         * Core/        [1, 100]
        */
        uint32_t instanceId                                          = 1;
        const char* logSaveDirPath                                   = "Build/Log/Core/";
    } g_collectionSettings;

    struct WindowSettings {
        const int width                                              = 1280;
        const int height                                             = 720;
        const char* titlePrefix                                      = "WINDOW_";
    } g_windowSettings;

    struct QueueSettings {
        /* Note that, the below indices are used only if the macro that allows manual picking of queue indices is enabled
        */
        const uint32_t graphicsFamilyIndex                           = 0;
        const uint32_t presentFamilyIndex                            = 1;
        const uint32_t transferFamilyIndex                           = 2;
    } g_queueSettings;

    struct PipelineSettings {
        struct InputAssembly {
            const VkPrimitiveTopology topology                       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            const VkBool32 restartEnable                             = VK_FALSE;
        } inputAssembly;

        struct ShaderStage {
            const char* vertexShaderBinaryPath                       = "Build/Bin/defaultShaderVert.spv";
            const char* fragmentShaderBinaryPath                     = "Build/Bin/defaultShaderFrag.spv";
        } shaderStage;

        struct Rasterization {
            const VkPolygonMode polygonMode                          = VK_POLYGON_MODE_FILL;
            const VkCullModeFlags cullMode                           = VK_CULL_MODE_NONE;
            const VkFrontFace frontFace                              = VK_FRONT_FACE_CLOCKWISE;
            const float lineWidth                                    = 1.0f;
        } rasterization;

        struct MultiSample {
            const VkBool32 sampleShadingEnable                       = VK_TRUE;
            const float minSampleShading                             = 0.2f;
        } multiSample;

        struct DepthStencil {
            const VkBool32 depthTestEnable                           = VK_TRUE;
            const VkBool32 depthWriteEnable                          = VK_TRUE;
            const VkBool32 depthBoundsTestEnable                     = VK_FALSE;
            const VkBool32 stencilTestEnable                         = VK_FALSE;
            const float minDepthBounds                               = 0.0f;
            const float maxDepthBounds                               = 1.0f;
        } depthStencil;

        struct ColorBlend {
            const VkLogicOp logicOp                                  = VK_LOGIC_OP_COPY;
            const VkBool32 blendEnable                               = VK_TRUE;
            const VkBool32 logicOpEnable                             = VK_FALSE;
            const float blendConstantR                               = 0.0f;
            const float blendConstantG                               = 0.0f;
            const float blendConstantB                               = 0.0f;
            const float blendConstantA                               = 0.0f;
        } colorBlend;

        struct DescriptorSetLayout {
            const VkDescriptorBindingFlags bindingFlagsSSBO          = 0;
            const VkDescriptorBindingFlags bindingFlagsCIS           = 0;
            const VkDescriptorSetLayoutCreateFlags layoutCreateFlags = 0;
        } descriptorSetLayout;

        /* The allow derivative flag specifies that the pipeline to be created is allowed to be the parent of a pipeline
         * that will be created in a subsequent pipeline creation call. Pipeline derivatives can be used for pipelines
         * that share most of their state, depending on the implementation this may result in better performance for
         * pipeline switching and faster creation time
        */
        const VkPipelineCreateFlags pipelineCreateFlags              = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    } g_pipelineSettings;

    struct TextureSamplerSettings {
        const VkFilter filter                                        = VK_FILTER_LINEAR;
        const VkSamplerAddressMode addressMode                       = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        const VkSamplerMipmapMode mipMapMode                         = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        const VkBool32 anisotropyEnable                              = VK_TRUE;
        const float mipLodBias                                       = 0.0f;
        const float minLod                                           = 0.0f;
        /* Set upper bound lod for the texture sampler. It is recommended that to sample from the entire mipmap chain,
         * set min lod to 0.0, and set max lod to a level of detail high enough that the computed level of detail will
         * never be clamped. Assuming the standard approach of halving the dimensions of a texture for each miplevel,
         * a max lod of 13 would be appropriate for a 4096x4096 source texture
        */
        const float maxLod                                           = 13.0f;
    } g_textureSamplerSettings;

    struct DescriptorSettings {
        const VkDescriptorPoolCreateFlags poolCreateFlags            = 0;
    } g_descriptorSettings;

    struct CoreSettings {
        /* As of now, we are required to wait on the previous frame to finish before we can start rendering the next
         * which results in unnecessary idling of the host. The way to fix this is to allow multiple frames to be
         * in-flight at once, that is to say, allow the rendering of one frame to not interfere with the recording of
         * the next. Any resource that is accessed and modified during rendering must be duplicated. Thus, we need
         * multiple command buffers, semaphores, and fences etc. First, define how many frames should be processed
         * concurrently
         *
         * We choose the number 2 because we don't want the CPU to get too far ahead of the GPU. With 2 frames in flight,
         * the CPU and the GPU can be working on their own tasks at the same time. If the CPU finishes early, it will
         * wait till the GPU finishes rendering before submitting more work. With 3 or more frames in flight, the CPU
         * could get ahead of the GPU, adding frames of latency as shown in the scenario below:
         *
         * What happens if frames in flight > swap chain size?
         * If they were, it could result in clashes over resource usage. In a case with 3 images and 6 frames, Frame 1
         * may be tied to Image 1, and Frame 4 could also be tied to Image 1. While Frame 1 is presenting, Frame 4 could
         * begin drawing in theory. But in practise would cause delays in execution because no image can be acquired from
         * the swap chain yet
        */
        const uint32_t maxFramesInFlight                             = 2;
        const char* defaultDiffuseTexturePath                        = "Asset/Texture/tex_16x16_empty.png";
    } g_coreSettings;
}   // namespace Core
#endif  // VK_CONFIG_H