#ifndef VK_RECORD_H
#define VK_RECORD_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKPipeline.h"
#include "VKFrameBuffer.h"
#include "../Collections/Log/include/Log.h"
#include <vulkan/vk_enum_string_helper.h>

using namespace Collections;

namespace Renderer {
    class VKRecord: protected VKVertexBuffer,
                    protected VKIndexBuffer,
                    protected VKPipeline,
                    protected virtual VKFrameBuffer {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKRecordLog;
            /* instance id for logger
            */
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKRecord (void) {
                m_VKRecordLog = LOG_INIT (m_instanceId, 
                                          static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE), 
                                          Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                          "./Build/Log/");
            }

            ~VKRecord (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Command buffer recording writes the commands we want to execute into a command buffer
            */
            void recordCommandBuffer (VkCommandBuffer commandBuffer,
                                      VkCommandBufferUsageFlags flags,
                                      VkBuffer srcBuffer, 
                                      VkBuffer dstBuffer, 
                                      VkDeviceSize size) {
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
                */
                beginInfo.flags = flags;
                /* The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which 
                 * state to inherit from the calling primary command buffers
                */
                beginInfo.pInheritanceInfo = nullptr;   

                /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly 
                 * reset it. It's not possible to append commands to a buffer at a later time.
                */
                VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRecordLog) << "Failed to begin recording command buffer " 
                                              << "[" << string_VkResult (result) << "]"
                                              << std::endl;
                    throw std::runtime_error ("Failed to begin recording command buffer");
                }             

                /* (1) Copy cmd
                 *
                 * Contents of buffers are transferred using the vkCmdCopyBuffer command. It takes the source and 
                 * destination buffers as arguments, and an array of regions to copy. The regions are defined in 
                 * VkBufferCopy structs and consist of a source buffer offset, destination buffer offset and size
                */
                VkBufferCopy copyRegion{};
                copyRegion.srcOffset = 0; 
                copyRegion.dstOffset = 0;
                copyRegion.size = size;
                vkCmdCopyBuffer (commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

                /* Finish recording command
                */
                result = vkEndCommandBuffer (commandBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRecordLog) << "Failed to record command buffer " 
                                              << "[" << string_VkResult (result) << "]" 
                                              << std::endl;
                    throw std::runtime_error ("Failed to record command buffer");
                } 
            }

            /* The VkCommandBuffer used will be passed in as a parameter, as well as the index of the current swapchain 
             * image we want to write to
            */
            void recordCommandBuffer (VkCommandBuffer commandBuffer, 
                                      uint32_t imageIndex, 
                                      uint32_t currentFrame) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0;
                beginInfo.pInheritanceInfo = nullptr;   

                VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRecordLog) << "Failed to begin recording command buffer " 
                                              << "[" << string_VkResult (result) << "]" 
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
                renderPassInfo.framebuffer = getFrameBuffers()[imageIndex];
                /* The next two parameters define the size of the render area. The render area defines where shader loads 
                 * and stores will take place. The pixels outside this region will have undefined values. It should match 
                 * the size of the attachments for best performance
                */
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = getSwapChainExtent();
                /* The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used 
                 * as load operation for the color attachment. We've defined the clear color to simply be black with 100% 
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

                /* (4) Bind vertex buffer and index buffer
                 * The vkCmdBindVertexBuffers function is used to bind vertex buffers to bindings, which is already set 
                 * up in createGraphicsPipeline function. The first two parameters, besides the command buffer, specify 
                 * the offset and number of bindings we're going to specify vertex buffers for. The last two parameters 
                 * specify the array of vertex buffers to bind and the byte offsets to start reading vertex data from
                */
                VkBuffer vertexBuffers[] = {getVertexBuffer()};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers (commandBuffer, 0, 1, vertexBuffers, offsets);
                /* The vkCmdBindIndexBuffer binds the index buffer, just like we did for the vertex buffer. The 
                 * difference is that you can only have a single index buffer. It's unfortunately not possible to use 
                 * different indices for each vertex attribute, so we do still have to completely duplicate vertex data 
                 * even if just one attribute varies. For example,
                 * 
                 * vertex attribute 1
                 * {
                 *      [x1, y1],
                 *      [x2, y2],
                 *      [x3, y3]
                 * }
                 * 
                 * vertex attribute 2
                 * {
                 *      [a1, b1, c1],
                 *      [a2, b2, c2],
                 *      [a3, b3, c3]
                 * }
                 * 
                 * index data 1
                 * {
                 *      0, 1, 2, 0, 1, 2
                 * }
                 * 
                 * index data 2
                 * {
                 *      0, 1, 2, 1, 1, 1
                 * }
                 * 
                 * Let us say this is the case where multiple same vertices (attribute 1) can have different normals 
                 * (attribute 2). But this is not possible, and we will need to duplicate the data so each unique vertex 
                 * has its own data, as stated above
                 * 
                 * vertex attribute 1
                 * {
                 *      [x1, y1],
                 *      [x2, y2],
                 *      [x3, y3],
                 *      [x1, y1],
                 *      [x2, y2],
                 *      [x3, y3]
                 * }
                 * 
                 * vertex attribute 2
                 * {
                 *      [a1, b1, c1],
                 *      [a2, b2, c2],
                 *      [a3, b3, c3],
                 *      [a2, b2, c2],
                 *      [a2, b2, c2],
                 *      [a2, b2, c2],
                 * }
                 * 
                 * index data
                 * {
                 *      0, 1, 2, 3, 4, 5  
                 * }
                */
                VkBuffer indexBuffer = getIndexBuffer();
                vkCmdBindIndexBuffer (commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                /* (5) Bind the descriptor set corresponding to currentFrame
                 * Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore we 
                 * need to specify if we want to bind descriptor sets to the graphics or compute pipeline. The next 
                 * parameter is the pipeline layout that the descriptors are based on.
                 * 
                 * The next three parameters specify the index of the first descriptor set, the number of sets to bind, 
                 * and the array of sets to bind.
                 * 
                 * The last two parameters specify an array of offsets that are used for dynamic descriptors
                */
                vkCmdBindDescriptorSets (commandBuffer, 
                                         VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                         getPipelineLayout(), 
                                         0, 
                                         1, 
                                         &getDescriptorSets()[currentFrame], 
                                         0, 
                                         nullptr);

                /* (6) Draw cmd
                 *
                 * The actual vkCmdDraw function is a bit anticlimactic, but it's so simple because of all the 
                 * information we specified in advance
                 * vertexCount: Number of vertices
                 * instanceCount: Used for instanced rendering, use 1 if you're not doing that
                 * firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex
                 * firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex
                 * 
                 * vkCmdDraw (commandBuffer, static_cast <uint32_t> (getVertices().size()), 1, 0, 0);
                 * 
                 * Since we are using an index buffer we will remove the vkCmdDraw and replace it with vkCmdDrawIndexed
                 * indexCount: Number of indices, this represents the number of vertices that will be passed to the 
                 * vertex shader
                 * firstIndex: Specifies an offset into the index buffer, using a value of 1 would cause the graphics 
                 * card to start reading at the second index
                 * vertexOffset: Specifies an offset to add to the indices in the index buffer
                */
                vkCmdDrawIndexed (commandBuffer, static_cast <uint32_t> (getIndices().size()), 1, 0, 0, 0);

                /* (7) End render pass cmd
                */
                vkCmdEndRenderPass (commandBuffer);

                /* Finish recording command
                */
                result = vkEndCommandBuffer (commandBuffer);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKRecordLog) << "Failed to record command buffer " 
                                              << "[" << string_VkResult (result) << "]" 
                                              << std::endl;
                    throw std::runtime_error ("Failed to record command buffer");
                }   
            }
    };

    Log::Record* VKRecord::m_VKRecordLog;
}   // namespace Renderer
#endif  // VK_RECORD_H