#ifndef VK_GRAPHICS_CMD_BUFFER_H
#define VK_GRAPHICS_CMD_BUFFER_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKVertexBuffer.h"
#include "VKResizing.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKGraphicsCmdBuffer: protected VKVertexBuffer,
                               protected VKResizing {
        private:
            /* Handle to command pool
            */
            VkCommandPool m_commandPool;
            /* Set maximum number of frames in flight
            */
            const uint32_t m_maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
            /* Handle to command buffers, each frame should have its own command buffer in order to handle multiple 
             * frames in flight
            */
            std::vector <VkCommandBuffer> m_commandBuffers;
            /* Handle to the log object
            */
            static Log::Record* m_VKGraphicsCmdBufferLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 2;
            
        public:
            VKGraphicsCmdBuffer (void) {
                m_VKGraphicsCmdBufferLog = LOG_INIT (m_instanceId, 
                                                     Log::VERBOSE, 
                                                     Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                                     "./Build/Log/");
                LOG_INFO (m_VKGraphicsCmdBufferLog) << "Constructor called" << std::endl; 
            }

            ~VKGraphicsCmdBuffer (void) {
                LOG_INFO (m_VKGraphicsCmdBufferLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function 
             * calls. You have to record all of the operations you want to perform in command buffer objects. The 
             * advantage of this is that when we are ready to tell the Vulkan what we want to do, all of the commands are 
             * submitted together and Vulkan can more efficiently process the commands since all of them are available 
             * together
             * 
             * We have to create a command pool before we can create command buffers. Command pools manage the memory 
             * that is used to store the buffers and command buffers are allocated from them
            */
            void createCommandPool (void) {
                VkCommandPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                /* Command pool possible flags:
                 * (1) VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will 
                 * be short-lived, meaning that they will be reset or freed in a relatively short timeframe
                 * 
                 * (2) VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool 
                 * to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the 
                 * implicit reset when calling vkBeginCommandBuffer
                 *
                 * We will be recording a command buffer every frame, so we want to be able to reset and rerecord over 
                 * it. Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command 
                 * pool
                */
                poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                /* Command buffers are executed by submitting them on one of the device queues, like the graphics and 
                 * presentation queues we retrieved. Each command pool can only allocate command buffers that are 
                 * submitted on a single type of queue
                 * 
                 * We're going to record commands for drawing, which is why we've chosen the graphics queue family
                */
                QueueFamilyIndices queueFamilyIndices = checkQueueFamilySupport (getPhysicalDevice());
                poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

                VkResult result = vkCreateCommandPool (getLogicalDevice(), 
                                                       &poolInfo, 
                                                       nullptr, 
                                                       &m_commandPool);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsCmdBufferLog) << "Failed to create command pool" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to create command pool");
                }
            }

            /* Create command buffers for every frame in flight
            */
            void createCommandBuffers (void) {
                m_commandBuffers.resize (m_maxFramesInFlight);

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                /* Specify the command pool and number of buffers to allocate
                */
                allocInfo.commandPool = m_commandPool;
                allocInfo.commandBufferCount = static_cast <uint32_t> (m_commandBuffers.size());
                /* The level parameter specifies if the allocated command buffers are primary or secondary command buffers
                 * VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from 
                 * other command buffers
                 * VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command 
                 * buffers
                 * We won't make use of the secondary command buffer functionality here, but you can imagine that it's 
                 * helpful to reuse common operations from primary command buffers
                */
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                
                VkResult result = vkAllocateCommandBuffers (getLogicalDevice(), 
                                                            &allocInfo, 
                                                            m_commandBuffers.data());
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsCmdBufferLog) << "Failed to create command buffers" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to create command buffers");
                }           
            }

            /* Command buffer recording writes the commands we want to execute into a command buffer. The VkCommandBuffer 
             * used will be passed in as a parameter, as well as the index of the current swapchain image we want to 
             * write to
            */
            void recordCommandBuffer (VkCommandBuffer commandBuffer, uint32_t imageIndex) {
                /* We always begin recording a command buffer by calling vkBeginCommandBuffer with a small 
                 * VkCommandBufferBeginInfo structure as argument that specifies some details about the usage of this 
                 * specific command buffer
                */
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                /* The flags parameter specifies how we're going to use the command buffer
                 * (1) VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that each recording of the command buffer 
                 * will only be submitted once, and the command buffer will be reset and recorded again between each 
                 * submission
                 * 
                 * (2) VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a secondary command buffer is 
                 * considered to be entirely inside a render pass. If this is a primary command buffer, then this bit is 
                 * ignored
                 * 
                 * (3) VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that a command buffer can be resubmitted 
                 * to any queue of the same queue family while it is in the pending state, and recorded into multiple 
                 * primary command buffers
                 * 
                 * None of these flags are applicable for us right now.
                */
                beginInfo.flags = 0;
                /* The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which 
                 * state to inherit from the calling primary command buffers
                */
                beginInfo.pInheritanceInfo = nullptr;   

                /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly 
                 * reset it. It's not possible to append commands to a buffer at a later time.
                */
                VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsCmdBufferLog) << "Failed to begin recording command buffer" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to begin recording command buffer");
                }             

                /* (1) Begin render pass cmd
                 * 
                 * Drawing starts by beginning the render pass with vkCmdBeginRenderPass. The render pass is configured 
                 * using some parameters in a VkRenderPassBeginInfo struct
                */
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                /* The first parameters are the render pass itself and the attachments to bind. We created a framebuffer 
                 * for each swap chain image where it is specified as a color attachment. Thus we need to bind the 
                 * framebuffer for the swapchain image we want to draw to. Using the imageIndex parameter which was 
                 * passed in, we can pick the right framebuffer for the current swapchain image
                */
                renderPassInfo.renderPass = getRenderPass();
                renderPassInfo.framebuffer = getFrameBuffers() [imageIndex];
                /* The next two parameters define the size of the render area. The render area defines where shader loads 
                 * and stores will take place. The pixels outside this region will have undefined values. It should match 
                 * the size of the attachments for best performance
                */
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = getSwapChainExtent();
                /* The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used 
                 * as load operation for the color attachment. I've defined the clear color to simply be black with 100% 
                 * opacity
                */
                VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                /* The render pass can now begin
                 * All of the functions that record commands can be recognized by their vkCmd prefix. They all return 
                 * void, so there will be no error handling until we've finished recording
                 * 
                 * The final parameter controls how the drawing commands within the render pass will be provided.
                 * VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer 
                 * itself and no secondary command buffers will be executed.
                 * VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from 
                 * secondary command buffers.
                 * We will not be using secondary command buffers, so we'll go with the first option.
                */
                vkCmdBeginRenderPass (commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                /* (2) Bind graphics pipeline cmd
                 * 
                 * The second parameter specifies if the pipeline object is a graphics or compute pipeline
                */
                vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline());

                /* (3) Configure dynamic state cmds
                 * 
                 * Up until now, we've told Vulkan which operations to execute in the graphics pipeline and which 
                 * attachment to use in the fragment shader. Also, we did specify viewport and scissor state for this 
                 * pipeline to be dynamic. So we need to set them in the command buffer before issuing our draw command
                */
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast <float> (getSwapChainExtent().width);
                viewport.height = static_cast <float> (getSwapChainExtent().height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport (commandBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = getSwapChainExtent();
                vkCmdSetScissor (commandBuffer, 0, 1, &scissor);

                /* (4) Bind vertex buffer
                 * The vkCmdBindVertexBuffers function is used to bind vertex buffers to bindings, which is already set 
                 * up in createGraphicsPipeline function. The first two parameters, besides the command buffer, specify 
                 * the offset and number of bindings we're going to specify vertex buffers for. The last two parameters 
                 * specify the array of vertex buffers to bind and the byte offsets to start reading vertex data from
                */
                VkBuffer vertexBuffers[] = {getVertexBuffer()};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers (commandBuffer, 0, 1, vertexBuffers, offsets);

                /* (5) Draw cmd
                 *
                 * The actual vkCmdDraw function is a bit anticlimactic, but it's so simple because of all the 
                 * information we specified in advance
                 * vertexCount: Number of vertices
                 * instanceCount: Used for instanced rendering, use 1 if you're not doing that
                 * firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex
                 * firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex
                */
                vkCmdDraw (commandBuffer, static_cast <uint32_t> (getVertices().size()), 1, 0, 0);

                /* (5) End render pass cmd
                */
                vkCmdEndRenderPass (commandBuffer);

                /* Finish recording command
                */
                result = vkEndCommandBuffer (commandBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKGraphicsCmdBufferLog) << "Failed to record command buffer" 
                                                         << " " 
                                                         << result 
                                                         << std::endl;
                    throw std::runtime_error ("Failed to record command buffer");
                }            
            }

            uint32_t getMaxFramesInFlight (void) {
                return m_maxFramesInFlight;
            }

            std::vector <VkCommandBuffer>& getCommandBuffers (void) {
                return m_commandBuffers;
            }

            void cleanUp (void) {
                /* Destroy command pool, note that command buffers will be automatically freed when their command pool 
                 * is destroyed, so we don't need explicit cleanup
                */
                vkDestroyCommandPool (getLogicalDevice(), m_commandPool, nullptr);
            }
    };

    Log::Record* VKGraphicsCmdBuffer::m_VKGraphicsCmdBufferLog;
}   // namespace Renderer
#endif  // VK_GRAPHICS_CMD_BUFFER_H