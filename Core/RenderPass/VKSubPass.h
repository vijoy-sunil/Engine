#ifndef VK_SUBPASS_H
#define VK_SUBPASS_H

#include "VKRenderPassMgr.h"

using namespace Collections;

namespace Renderer {
    /* The idea of sub passes is that you have multiple operations in a sequence that operate on the same pixels in the 
     * frame buffer, which is mainly useful for things like deferred rendering
     * 
     * A graphics pipeline describes a set of operations that usually take vertices from an input buffer and ultimately 
     * write pixels to an output framebuffer. This task of writing pixels to one or more framebuffers constitutes a single
     * sub pass. The sub pass describes which framebuffers will be accessed (read/written) by the graphics pipeline and 
     * in which state they should be at various stages in the pipeline (e.g. they should be writable right before the 
     * fragment shader starts running). It is possible that this is all of your rendering and then you can wrap this
     *  single sub pass into a render pass and call it a day
     * 
     * However, let's say you want to render various post-processing effects like bloom, depth-of-field and motion blur 
     * one after another to composite the final shot. Let's assume you already have your scene rendered to a framebuffer. 
     * Then you could apply the post-processing effects by having:
     * 
     * render pass 1
     * - sub pass: render scene with added bloom to a new framebuffer
     * render pass 2
     * - sub pass: add blur to bloom framebuffer and output it to a new framebuffer
     * render pass 3
     * - sub pass: add motion blur to depth-of-field framebuffer and output to the final framebuffer
     * 
     * This approach works, but the problem is that we have to write the pixels to memory every time, only to read them 
     * back right away in the next operation. We can do this more efficiently by having a single render pass and multiple 
     * sub passes:
     * 
     * render pass
     * - sub pass 1: apply bloom to scene and output
     * - sub pass 2: apply blur to previous output
     * - sub pass 3: apply depth-of-field to previous output
     * 
     * Each sub pass may run a different graphics pipeline, but sub passes describe that they're reading from attachments 
     * that have been written by the sub pass right before. This allows the graphics driver to optimize the memory 
     * operations to much more efficiently execute all these operations in a row because it can chain them together
     * 
     * There is a catch however: you may only use sub passes like this if the fragment shader at each pixel only reads 
     * from the exact same pixel in the previous operation's output. That's why it is best used for post-processing 
     * effects and deferred rendering and less useful for chaining other operations. If you need to read other pixels, 
     * then you will have to use multiple render passes 
     * 
     * In other words, sub passes control the state and usage of your framebuffers at the point that they start being used
     * by the graphics pipeline and at the point when they stop being used. They don't affect the passing of variables 
     * between shaders and pipeline stages, that is controlled by the pipeline itself. They are really designed to allow 
     * you to efficiently pass images between graphics pipelines and not within them
    */
    class VKSubPass: protected virtual VKRenderPassMgr {
        private:
            static Log::Record* m_VKSubPassLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKSubPass (void) {
                m_VKSubPassLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKSubPass (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Remember that the subpasses in a render pass automatically take care of image layout transitions. These 
             * transitions are controlled by subpass dependencies, which specify memory and execution dependencies 
             * between subpasses. Note that, the operations right before and right after a subpass also count as implicit
             * "subpasses"
             * 
             * There are two built-in dependencies that take care of the transition at the start of the render pass and at
             * the end of the render pass, but the former does not occur at the right time. It assumes that the transition
             * occurs at the start of the pipeline, but we haven't acquired the image yet at that point (see sync object
             * wait operation)
             * 
             * Solution: (We choose option #2)
             * (1) We could change the waitStages for the image available semaphore to 
             * VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to ensure that the render passes don't begin until the image is available
             * 
             * (2) We can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage (this 
             * stage specifies the stage of the pipeline after blending where the final color values are output from the 
             * pipeline)
             * 
             * Image layout transition
             * Before the render pass, the layout of the image will be transitioned to the layout you specify, for example
             * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. However, by default this happens at the beginning of the pipeline
             * at which point we haven't acquired the image yet (we acquire it in the 
             * VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage per the sync object wait operation). That means that we
             * need to change the behaviour of the render pass to also only change the layout once we've come to that 
             * stage
             * 
             * The stage masks in the subpass dependency allow the subpass to already begin before the image is available 
             * up until the point where it needs to write to it
            */
            void createColorWriteDependency (uint32_t renderPassInfoId,
                                             uint32_t srcSubPass, 
                                             uint32_t dstSubPass) {

                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkSubpassDependency dependency{};
                /* Note that, stage masks relate to execution order, while access masks relate to memory/cache access
                 *
                 * Execution order is like a dependency chain between the two subpassess, with the stage masks saying 
                 * which stages of the destination depend on the source. Stage masks are useful because they limit the 
                 * dependency to only the stages that actually are dependent, while allowing other stages to occur. So 
                 * like abstractally speaking, if the 5th stage of B (B5) depends on 3rd stage of A, then B1-B4 can still 
                 * run before A is fully complete. Once B gets to stage 5 it must wait until A3 has completed before 
                 * continuing
                 * 
                 * Access masks, however, relate to memory availability/visibility. Somewhat suprising is that just 
                 * because you set up an execution dependency where for example, A (the src) writes to some resource and 
                 * then B (dst) reads from the resource. Even if B executes after A, that doesn't mean B will "see" the 
                 * changes A has made, because of caching! It is very possible that even though A has finished, it has 
                 * made its changes to a memory cache that hasn't been made available/"flushed". So in the dependency you 
                 * could use, for example
                 * 
                 * srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT
                 * dstAccessMask = VK_ACCESS_MEMORY_READ_BIT
                 * 
                 * The src access mask says that the memory A writes to should be made available/"flushed" to like the 
                 * shared gpu memory, and the dst access mask says that the memory/cache B reads from should first pull 
                 * from the shared gpu memory. This way B is reading from up to date memory, and not stale cache data
                 * 
                 * Note that, VK_ACCESS_NONE means that there is no memory dependency the barrier introduces
                 *  
                 * srcSubpass is the index of the subpass we're dependant on. If we wanted to depend on a subpass that's 
                 * part of a previous render pass, we could just pass in VK_SUBPASS_EXTERNAL here instead. Note that, this
                 * would mean "wait for all of the subpasses within all of the render passes before this one", this also
                 * includes the implicit subpass that takes care of image layout transitions
                 * 
                 * dstSubpass is the index to the current subpass, i.e. the one this dependency exists for
                 * 
                 * The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless 
                 * one of the subpasses is VK_SUBPASS_EXTERNAL)
                */
                dependency.srcSubpass = srcSubPass;
                dependency.dstSubpass = dstSubPass;
                /* srcStageMask is a bitmask of all of the Vulkan "stages" (basically, steps of the rendering process) we
                 * are asking Vulkan to finish executing within srcSubpass before we move on to dstSubpass
                 *
                 * srcAccessMask is a bitmask of all the Vulkan memory access types used by srcSubpass
                 * 
                 * We need to wait for the swap chain to finish reading from the image before we can access it. This can 
                 * be accomplished by waiting on the color attachment output stage itself
                */
                dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = VK_ACCESS_NONE;
                /* dstStageMask is a bitmask of all of the Vulkan stages in dstSubpass that we're NOT allowed to execute 
                 * until after the stages in srcStageMask have completed within srcSubpass
                 * 
                 * dstAccessMask is a bitmask of all the Vulkan memory access types we're going to use in dstSubpass
                 * 
                 * The operations that should wait on this are, in the color attachment stage and involve the writing of 
                 * the color attachment, These settings will prevent the transition from happening until it's actually 
                 * necessary: when we want to start writing to it
                */
                dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                renderPassInfo->resource.dependencies.push_back (dependency);
            }

            /* It is possible that multiple frames are rendered simultaneously by the GPU. This is a problem when using 
             * a single depth buffer, because one frame could overwrite the depth buffer while a previous frame is still 
             * rendering to it. To prevent this, we add a new subpass dependency that synchronizes accesses to the depth 
             * attachment
             *  
             * This dependency tells Vulkan that the depth attachment in a renderpass cannot be used before previous 
             * render passes have finished using it
            */
            void createDepthStencilDependency (uint32_t renderPassInfoId, 
                                               uint32_t srcSubPass, 
                                               uint32_t dstSubPass) {
                
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkSubpassDependency dependency{};
                dependency.srcSubpass = srcSubPass;
                dependency.dstSubpass = dstSubPass;
                /* The depth image is first accessed in the early fragment test pipeline stage and we need to make sure 
                 * that there is no conflict between the transitioning of the depth image and it being cleared as part of 
                 * its load operation (VK_ATTACHMENT_LOAD_OP_CLEAR)
                */
                dependency.srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | 
                                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask = VK_ACCESS_NONE;

                dependency.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | 
                                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; 

                renderPassInfo->resource.dependencies.push_back (dependency);            
            }

            void createSubPass (uint32_t renderPassInfoId,
                                const std::vector <VkAttachmentReference>& colorAttachments,
                                const VkAttachmentReference& depthStencilAttachment,
                                const std::vector <VkAttachmentReference>& resolveAttachments) {

                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkSubpassDescription subPass{};
                subPass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subPass.colorAttachmentCount = static_cast <uint32_t> (colorAttachments.size());
                /* The index of the attachment in this array is directly referenced from the fragment shader with the 
                 * layout(location = ?) out vec4 outColor directive
                */
                subPass.pColorAttachments = colorAttachments.data();
                /* Unlike color attachments, a subpass can only use a single depth (+stencil) attachment. That is why 
                 * pDepthStencilAttachment accepts only a single attachment reference and not an array of references
                */
                subPass.pDepthStencilAttachment = &depthStencilAttachment;
                /* This will let the render pass define a multisample resolve operation which will let us render the image
                 * to screen
                */
                subPass.pResolveAttachments = resolveAttachments.data();

                renderPassInfo->resource.subPasses.push_back (subPass);
            }
    };

    Log::Record* VKSubPass::m_VKSubPassLog;
}   // namespace Renderer
#endif  // VK_SUBPASS_H