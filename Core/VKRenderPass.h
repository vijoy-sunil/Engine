#ifndef VK_RENDER_PASS_H
#define VK_RENDER_PASS_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKPhyDeviceHelper.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKRenderPass: protected VKPhyDeviceHelper {
        private:
            /* Handle to render pass object
            */
            VkRenderPass m_renderPass;
            /* Handle to the log object
            */
            static Log::Record* m_VKRenderPassLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 3;
            
        public:
            VKRenderPass (void) {
                m_VKRenderPassLog = LOG_INIT (m_instanceId, 
                                              static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                              Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                              "./Build/Log/");
            }

            ~VKRenderPass (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createRenderPass (void) {
                /* Frame buffer (swap chain) attachments
                 * FBOs (frame buffer objects) are "offscreen" rendering targets. All this means is that instead of 
                 * making your picture appear on your display, you render it to some other place -- an FBO. Before you 
                 * can do this, you have to create and configure the FBO. Part of that configuration is adding a color 
                 * attachment -- a buffer to hold the per-pixel color information of the rendered picture. Maybe you stop 
                 * there, or maybe you also add a depth attachment. If you are rendering 3D geometry, and you want it to 
                 * look correct, you'll likely have to add this depth attachment
                 * 
                 * In our case we'll have just a single color buffer attachment with the same format as the swap chain 
                 * images
                */
                VkAttachmentDescription colorAttachment{};
                /* The format of the color attachment should match the format of the swap chain images, and we're not 
                 * doing anything with multisampling yet, so we'll stick to 1 sample
                */
                colorAttachment.format = getSwapChainImageFormat();
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                /* The loadOp and storeOp determine what to do with the data in the attachment before rendering and after 
                 * rendering. 
                
                 * We have the following choices for loadOp:
                 * VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
                 * VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
                 * VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
                 * 
                 * In our case we're going to use the clear operation to clear the framebuffer to black before drawing a 
                 * new frame. 
                 * 
                 * There are only two possibilities for the storeOp:
                 * VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
                 * VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering 
                 * operation
                */
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                /* The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to 
                 * stencil data. Our application won't do anything with the stencil buffer, so the results of loading and 
                 * storing are irrelevant
                */
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                /* Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, 
                 * however the layout of the pixels in memory can change based on what you're trying to do with an image. 
                 * In other words, images need to be transitioned to specific layouts that are suitable for the operation 
                 * that they're going to be involved in next
                 *
                 * Some of the most common layouts are:
                 * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
                 * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
                 * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
                 * 
                 * VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image 
                 * was in. The caveat of this special value is that the contents of the image are not guaranteed to be 
                 * preserved, but that doesn't matter since we're going to clear it anyway. We want the image to be ready 
                 * for presentation using the swap chain after rendering, which is why we use 
                 * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout
                */
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                /* Setup subpasses and attachment references
                 * The idea of sub passes is that you have multiple operations in a sequence that operate on the same 
                 * pixels in the frame buffer, which is mainly useful for things like deferred rendering
                 * 
                 * A graphics pipeline describes a set of operations that usually take vertices from an input buffer and 
                 * ultimately write pixels to an output framebuffer. This task of writing pixels to one or more 
                 * framebuffers constitutes a single sub pass. The sub pass describes which framebuffers will be 
                 * accessed (read/written) by the graphics pipeline and in which state they should be at various stages 
                 * in the pipeline (e.g. they should be writable right before the fragment shader starts running). It is 
                 * possible that this is all of your rendering and then you can wrap this single sub pass into a render 
                 * pass and call it a day.
                 * 
                 * However, let's say you want to render various post-processing effects like bloom, depth-of-field and 
                 * motion blur one after another to composite the final shot. Let's assume you already have your scene 
                 * rendered to a framebuffer. Then you could apply the post-processing effects by having:
                 * 
                 * render pass 1
                 * - sub pass: render scene with added bloom to a new framebuffer
                 * render pass 2
                 * - sub pass: add blur to bloom framebuffer and output it to a new framebuffer
                 * render pass 3
                 * - sub pass: add motion blur to depth-of-field framebuffer and output to the final framebuffer
                 * 
                 * This approach works, but the problem is that we have to write the pixels to memory every time, only 
                 * to read them back right away in the next operation. We can do this more efficiently by having a single 
                 * render pass and multiple sub passes:
                 * 
                 * render pass
                 * - sub pass 1: apply bloom to scene and output
                 * - sub pass 2: apply blur to previous output
                 * - sub pass 3: apply depth-of-field to previous output
                 * 
                 * Each sub pass may run a different graphics pipeline, but sub passes describe that they're reading from 
                 * attachments that have been written by the sub pass right before. This allows the graphics driver to 
                 * optimize the memory operations to much more efficiently execute all these operations in a row because 
                 * it can chain them together.
                 * 
                 * There is a catch however: you may only use sub passes like this if the fragment shader at each pixel 
                 * only reads from the exact same pixel in the previous operation's output. That's why it is best used 
                 * for post-processing effects and deferred rendering and less useful for chaining other operations. If 
                 * you need to read other pixels, then you will have to use multiple render passes. 
                 * 
                 * In other words, sub passes control the state and usage of your framebuffers at the point that they 
                 * start being used by the graphics pipeline and at the point when they stop being used. They don't 
                 * affect the passing of variables between shaders and pipeline stages, that is controlled by the 
                 * pipeline itself. They are ]really designed to allow you to efficiently pass images between graphics 
                 * pipelines and not within them.
                */

                /* Every subpass references one or more of the attachments that we've described earlier. These references 
                 * are themselves VkAttachmentReference structs.
                */
                VkAttachmentReference colorAttachmentRef{};
                /* The VkAttachmentReference does not reference the attachment object directly, it references the index 
                 * in the attachments array specified in VkRenderPassCreateInfo. This allows subpasses to reference the 
                 * same attachment
                */
                colorAttachmentRef.attachment = 0;
                /* The layout specifies which layout we would like the attachment to have during a subpass that uses this 
                 * reference. Vulkan will automatically transition the attachment to this layout when the subpass is 
                 * started. We intend to use the attachment to function as a color buffer and the 
                 * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance, as its name implies
                */
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                /* Setup subpass
                 * Vulkan may also support compute subpasses in the future, so we have to be explicit about this being a 
                 * graphics subpass
                */
                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                /* Specify the reference to the attachment
                */
                subpass.colorAttachmentCount = 1;
                /* The index of the attachment in this array is directly referenced from the fragment shader with the 
                 * layout(location = 0) out vec4 outColor directive.
                 *
                 * [?] Does this mean that by specifying (location = 0) in the fragment shader we effectively output the 
                 * shading result to the first color attachment in the subpass.
                 * 
                 * The following other types of attachments can be referenced by a subpass:
                 * pInputAttachments: Attachments that are read from a shader
                 * pResolveAttachments: Attachments used for multisampling color attachments
                 * pDepthStencilAttachment: Attachment for depth and stencil data
                 * pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be 
                 * preserved
                */
                subpass.pColorAttachments = &colorAttachmentRef;

                /* Setup subpass dependencies
                 * Remember that the subpasses in a render pass automatically take care of image layout transitions. These 
                 * transitions are controlled by subpass dependencies, which specify memory and execution dependencies 
                 * between subpasses. We have only a single subpass right now, but the operations right before and right 
                 * after this subpass also count as implicit "subpasses"
                 * 
                 * There are two built-in dependencies that take care of the transition at the start of the render pass 
                 * and at the end of the render pass, but the former does not occur at the right time. It assumes that 
                 * the transition occurs at the start of the pipeline, but we haven't acquired the image yet at that 
                 * point (see drawFrame)
                 * 
                 * Solution: (We choose option #2)
                 * (1) We could change the waitStages for the imageAvailableSemaphore to 
                 * VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to ensure that the render passes don't begin until the image is 
                 * available, OR
                 * 
                 * (2) We can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage (note 
                 * that this wait is not the same wait as in the draw frame function)
                 * 
                 * Image layout transition
                 * Before the render pass the layout of the image will be transitioned to the layout you specify 
                 * (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL). However, by default this happens at the beginning of the 
                 * pipeline at which point we haven't acquired the image yet (we acquire it in the 
                 * VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage per drawFrame). That means that we need to change 
                 * the behaviour of the render pass to also only change the layout once we've come to that stage
                 * 
                 * The stage masks in the subpass dependency allow the subpass to already begin before the image is 
                 * available up until the point where it needs to write to it
                */
                VkSubpassDependency dependency{};
                /* The first two fields specify the indices of the dependency and the dependent subpass. The special value 
                 * VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on 
                 * whether it is specified in srcSubpass or dstSubpass. The index 0 refers to our subpass, which is the 
                 * first and only one. The dstSubpass must always be higher than srcSubpass to prevent cycles in the 
                 * dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
                */
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                /* The next two fields specify the operations to wait on and the stages in which these operations occur. 
                 * We need to wait for the swap chain to finish reading from the image before we can access it. This can 
                 * be accomplished by waiting on the color attachment output stage itself
                 * 
                 * The 'source' is the implicit subpass and the 'destination' is our main subpass
                */
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = 0;
                /* The operations that should wait on this are in the color attachment stage and involve the writing of 
                 * the color attachment. These settings will prevent the transition from happening until it's actually 
                 * necessary (and allowed): when we want to start writing colors to it
                */
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                /* Create render pass
                */
                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = 1;
                renderPassInfo.pAttachments = &colorAttachment;
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = 1;
                renderPassInfo.pDependencies = &dependency;

                VkResult result = vkCreateRenderPass (getLogicalDevice(), 
                                                      &renderPassInfo, 
                                                      nullptr, 
                                                      &m_renderPass);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRenderPassLog) << "Failed to create render pass" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create render pass");
                }
            }

            VkRenderPass getRenderPass (void) {
                return m_renderPass;
            }

            void cleanUp (void) {
                /* Destroy render pass
                */
                vkDestroyRenderPass (getLogicalDevice(), m_renderPass, nullptr);
            }
    };

    Log::Record* VKRenderPass::m_VKRenderPassLog;
}   // namespace Renderer
#endif  // VK_RENDER_PASS_H