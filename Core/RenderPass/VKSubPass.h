#ifndef VK_SUB_PASS_H
#define VK_SUB_PASS_H

#include "VKRenderPassMgr.h"

namespace Core {
    /* The idea of sub passes is that you have multiple operations in a sequence that operate on the same pixels in the
     * frame buffer, which is mainly useful for things like deferred rendering
     *
     * A graphics pipeline describes a set of operations that usually take vertices from an input buffer and ultimately
     * write pixels to an output frame buffer. This task of writing pixels to one or more frame buffers constitutes a
     * single sub pass. The sub pass describes which frame buffers will be accessed (read/written) by the graphics
     * pipeline and in which state they should be at various stages in the pipeline (e.g. they should be writable right
     * before the fragment shader starts running). It is possible that this is all of your rendering and then you can
     * wrap this single sub pass into a render pass and call it a day
     *
     * However, let's say you want to render various post-processing effects like bloom, depth-of-field and motion blur
     * one after another to composite the final shot. Let's assume you already have your scene rendered to a frame buffer.
     * Then you could apply the post-processing effects by having:
     *
     * render pass 1
     * - sub pass: render scene with added bloom to a new frame buffer
     * render pass 2
     * - sub pass: add blur to bloom frame buffer and output it to a new frame buffer
     * render pass 3
     * - sub pass: add motion blur to depth-of-field frame buffer and output to the final frame buffer
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
     * In other words, sub passes control the state and usage of your frame buffers at the point that they start being
     * used by the graphics pipeline and at the point when they stop being used. They don't affect the passing of
     * variables between shaders and pipeline stages, that is controlled by the pipeline itself. They are really designed
     * to allow you to efficiently pass images between graphics pipelines and not within them
    */
    class VKSubPass: protected virtual VKRenderPassMgr {
        private:
            Log::Record* m_VKSubPassLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKSubPass (void) {
                m_VKSubPassLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~VKSubPass (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Barriers vs render pass mechanisms (sub pass dependencies vs layout transitions):
             * Barriers work with anything; they don't care where the image comes from, was used for, or where it is
             * going. Render pass mechanisms only work for stuff that happens in a render pass and primarily deal with
             * images attached to render passes (implicit layout transitions only work on attachments). So during a
             * render pass, you can only change layout using the render pass mechanism, or you must be outside the
             * render pass
            */
            void createDependency (uint32_t renderPassInfoId,
                                   VkDependencyFlags flags,
                                   uint32_t srcSubPass,
                                   uint32_t dstSubPass,
                                   VkPipelineStageFlags srcStageMask,
                                   VkAccessFlags srcAccessMask,
                                   VkPipelineStageFlags dstStageMask,
                                   VkAccessFlags dstAccessMask) {

                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkSubpassDependency dependency;
                dependency.dependencyFlags = flags;
                /* Note that, stage masks relate to execution order, while access masks relate to memory/cache access
                 *
                 * Execution order is like a dependency chain between the two sub passess, with the stage masks saying
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
                 * srcSubpass is the index of the sub pass we're dependant on. If we wanted to depend on a sub pass that's
                 * part of a previous render pass, we could just pass in VK_SUBPASS_EXTERNAL here instead. Note that, this
                 * would mean "wait for all of the sub passes within all of the render passes before this one", this also
                 * includes the implicit sub pass that takes care of image layout transitions
                 *
                 * dstSubpass is the index to the current sub pass, i.e. the one this dependency exists for
                 *
                 * The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless
                 * one of the sub passes is VK_SUBPASS_EXTERNAL)
                */
                dependency.srcSubpass    = srcSubPass;
                dependency.dstSubpass    = dstSubPass;
                /* srcStageMask is a bitmask of all of the Vulkan "stages" (basically, steps of the rendering process) we
                 * are asking Vulkan to finish executing within srcSubpass before we move on to dstSubpass
                 *
                 * srcAccessMask is a bitmask of all the Vulkan memory access types used by srcSubpass
                */
                dependency.srcStageMask  = srcStageMask;
                dependency.srcAccessMask = srcAccessMask;
                /* dstStageMask is a bitmask of all of the Vulkan stages in dstSubpass that we're NOT allowed to execute
                 * until after the stages in srcStageMask have completed within srcSubpass
                 *
                 * dstAccessMask is a bitmask of all the Vulkan memory access types we're going to use in dstSubpass
                */
                dependency.dstStageMask  = dstStageMask;
                dependency.dstAccessMask = dstAccessMask;

                renderPassInfo->resource.dependencies.push_back (dependency);
            }

            void createSubPass (uint32_t renderPassInfoId,
                                const std::vector <VkAttachmentReference>& inputAttachments,
                                const std::vector <VkAttachmentReference>& colorAttachments,
                                const VkAttachmentReference* depthStencilAttachment,
                                const std::vector <VkAttachmentReference>& resolveAttachments) {

                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);

                VkSubpassDescription subPass;
                subPass.flags                   = 0;
                subPass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subPass.inputAttachmentCount    = static_cast <uint32_t> (inputAttachments.size());
                subPass.pInputAttachments       = inputAttachments.data();
                subPass.preserveAttachmentCount = 0;
                subPass.pPreserveAttachments    = VK_NULL_HANDLE;
                subPass.colorAttachmentCount    = static_cast <uint32_t> (colorAttachments.size());
                /* The index of the attachment in this array is directly referenced from the fragment shader with the
                 * layout (location = ?) out directive
                */
                subPass.pColorAttachments       = colorAttachments.data();
                /* Unlike color attachments, a sub pass can only use a single depth (+stencil) attachment. That is why
                 * pDepthStencilAttachment accepts only a single attachment reference and not an array of references
                */
                subPass.pDepthStencilAttachment = depthStencilAttachment;
                /* This will let the render pass define a multi sample resolve operation which will let us render the
                 * image to screen
                */
                subPass.pResolveAttachments     = resolveAttachments.data();

                renderPassInfo->resource.subPasses.push_back (subPass);
            }
    };
}   // namespace Core
#endif  // VK_SUB_PASS_H