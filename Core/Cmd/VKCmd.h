#ifndef VK_CMD_H
#define VK_CMD_H

#include "../Image/VKImageMgr.h"
#include "../Buffer/VKBufferMgr.h"
#include "../Pipeline/VKPipelineMgr.h"

using namespace Collections;

namespace Core {
    class VKCmd: protected virtual VKImageMgr,
                 protected virtual VKBufferMgr,
                 protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKCmdLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

        public:
            VKCmd (void) {
                m_VKCmdLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKCmd (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void setViewPorts (uint32_t deviceInfoId,
                               uint32_t firstViewPort,
                               std::vector <VkViewport>& viewPorts,
                               VkCommandBuffer commandBuffer) {
                
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                /* We've told Vulkan which operations to execute in the graphics pipeline and specified viewport and 
                 * scissor state for the pipeline to be dynamic. So we need to set them in the command buffer before 
                 * issuing our draw command
                */
                VkViewport defaultViewPort;
                defaultViewPort.x        = 0.0f;
                defaultViewPort.y        = 0.0f;
                defaultViewPort.width    = static_cast <float> (deviceInfo->params.swapChainExtent.width);
                defaultViewPort.height   = static_cast <float> (deviceInfo->params.swapChainExtent.height);
                defaultViewPort.minDepth = 0.0f;
                defaultViewPort.maxDepth = 1.0f;
                /* Add default view port to list of custom view ports (if any)
                */
                viewPorts.push_back (defaultViewPort);

                vkCmdSetViewport (commandBuffer, 
                                  firstViewPort, 
                                  static_cast <uint32_t> (viewPorts.size()), 
                                  viewPorts.data());
            }

            void setScissors (uint32_t deviceInfoId,
                              uint32_t firstScissor,
                              std::vector <VkRect2D>& scissors,
                              VkCommandBuffer commandBuffer) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);

                VkRect2D defaultScissor;
                defaultScissor.offset = {0, 0};
                defaultScissor.extent = deviceInfo->params.swapChainExtent;

                scissors.push_back (defaultScissor);
                vkCmdSetScissor (commandBuffer, 
                                 firstScissor, 
                                 static_cast <uint32_t> (scissors.size()), 
                                 scissors.data());
            }

            void copyBufferToBuffer (uint32_t srcBufferInfoId,
                                     uint32_t dstBufferInfoId,
                                     e_bufferType srcBufferType, 
                                     e_bufferType dstBufferType,
                                     VkDeviceSize srcOffset,
                                     VkDeviceSize dstOffset,
                                     VkCommandBuffer commandBuffer) {

                auto srcBufferInfo = getBufferInfo (srcBufferInfoId, srcBufferType);
                auto dstBufferInfo = getBufferInfo (dstBufferInfoId, dstBufferType);
                /* Contents of buffers are transferred using the vkCmdCopyBuffer command. It takes the source and 
                 * destination buffers as arguments, and an array of regions to copy. The regions are defined in 
                 * VkBufferCopy structs and consist of a source buffer offset, destination buffer offset and size
                */
                VkBufferCopy copyRegion;
                copyRegion.srcOffset = srcOffset; 
                copyRegion.dstOffset = dstOffset;
                copyRegion.size      = srcBufferInfo->meta.size;
                vkCmdCopyBuffer (commandBuffer, 
                                 srcBufferInfo->resource.buffer, 
                                 dstBufferInfo->resource.buffer, 
                                 1, 
                                 &copyRegion);
            }

            void copyBufferToImage (uint32_t srcBufferInfoId,
                                    uint32_t dstImageInfoId,
                                    e_bufferType srcBufferType, 
                                    e_imageType dstImageType, 
                                    VkDeviceSize srcOffset,
                                    VkImageLayout dstImageLayout,
                                    VkCommandBuffer commandBuffer) {

                auto dstImageInfo  = getImageInfo  (dstImageInfoId,  dstImageType);
                auto srcBufferInfo = getBufferInfo (srcBufferInfoId, srcBufferType);

                VkImageMemoryBarrier barrier;
                VkPipelineStageFlags sourceStage, destinationStage;
                transitionImageLayout (dstImageInfo->resource.image,
                                       dstImageInfo->params.initialLayout,
                                       dstImageLayout,
                                       0,
                                       dstImageInfo->meta.mipLevels,
                                       dstImageInfo->params.aspect,
                                       &barrier,
                                       sourceStage,
                                       destinationStage);
                /* All types of pipeline barriers are submitted using the same function. The first parameter after the 
                 * command buffer specifies in which pipeline stage the operations occur that should happen before the 
                 * barrier. The second parameter specifies the pipeline stage in which operations will wait on the 
                 * barrier
                 * 
                 * Note that, the pipeline stages that you are allowed to specify before and after the barrier depend on 
                 * how you use the resource before and after the barrier. For example, if you're going to read from a 
                 * uniform after the barrier, you would specify a destination stage of VK_ACCESS_UNIFORM_READ_BIT and 
                 * the earliest shader that will read from the uniform as pipeline stage, for example 
                 * VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT as the destination access mask
                 * 
                 * The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT. The latter turns the barrier into a 
                 * per-region condition. That means that the implementation is allowed to already begin reading from the 
                 * parts of a resource that were written so far, for example
                 * 
                 * The last three pairs of parameters reference arrays of pipeline barriers of the three available types,
                 * memory barriers, buffer memory barriers, and image memory barriers
                */
                vkCmdPipelineBarrier (commandBuffer,
                                      sourceStage,
                                      destinationStage,
                                      0,
                                      0, VK_NULL_HANDLE,
                                      0, VK_NULL_HANDLE,
                                      1, &barrier);
                /* Copy buffer containing pixel data to image, just like with buffer copies, you need to specify 
                 * which part of the buffer is going to be copied to which part of the image. This happens through 
                 * VkBufferImageCopy structs
                 * 
                 * The bufferOffset specifies the byte offset in the buffer at which the pixel values start. The 
                 * bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For 
                 * example, you could have some padding bytes between rows of the image. Specifying 0 for both indicates 
                 * that the pixels are simply tightly packed. The imageSubresource, imageOffset and imageExtent fields 
                 * indicate to which part of the image we want to copy the pixels
                */
                VkBufferImageCopy copyRegion;
                copyRegion.bufferOffset      = srcOffset;
                copyRegion.bufferRowLength   = 0;
                copyRegion.bufferImageHeight = 0;

                copyRegion.imageSubresource.aspectMask     = dstImageInfo->params.aspect;
                copyRegion.imageSubresource.mipLevel       = 0;
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount     = 1;

                copyRegion.imageOffset = {0, 0, 0};
                copyRegion.imageExtent = {
                                            dstImageInfo->meta.width,
                                            dstImageInfo->meta.height,
                                            1
                                         };
                /* Buffer to image copy operations are enqueued using the vkCmdCopyBufferToImage function, the fourth 
                 * parameter indicates which layout the image is currently using. I'm assuming here that the image has 
                 * already been transitioned to the layout that is optimal for copying pixels to
                 * 
                 * We're only copying one chunk of pixels to the whole image, but it's possible to specify an array of 
                 * VkBufferImageCopy to perform many different copies from this buffer to the image in one operation
                */
                vkCmdCopyBufferToImage (commandBuffer,
                                        srcBufferInfo->resource.buffer,
                                        dstImageInfo->resource.image,
                                        dstImageLayout,
                                        1,
                                        &copyRegion);                
            }

            /* Mipmaps are precalculated, downscaled versions of an image. Each new image is half the width and height of 
             * the previous one. Mipmaps are used as a form of Level of Detail or LOD. Objects that are far away from the 
             * camera will sample their textures from the smaller mip images. Using smaller images increases the rendering
             * speed and avoids artifacts such as Moiré patterns
             * 
             * Note that, if you are using a dedicated transfer queue, vkCmdBlitImage must be submitted to a queue with 
             * graphics capability
            */
            void blitImageToMipMaps (uint32_t imageInfoId, 
                                     e_imageType imageType,
                                     VkCommandBuffer commandBuffer) {

                auto imageInfo = getImageInfo (imageInfoId, imageType);
                /* Generating mip maps
                 * The input texture image is generated with multiple mip levels, but the staging buffer can only be used 
                 * to fill mip level 0. The other levels are still undefined. To fill these levels we need to generate the 
                 * data from the single level that we have. We will use the vkCmdBlitImage command to perform copying, 
                 * scaling, and filtering operations. We will call this multiple times to blit data to each level of our 
                 * texture image. Note that, vkCmdBlitImage is considered a transfer operation, so we must inform Vulkan 
                 * that we intend to use the texture image as both the source and destination of a transfer by specifying
                 * in usage flags when creating the texture image
                 * 
                 * vkCmdBlitImage depends on the layout of the image it operates on. We could transition the entire image 
                 * to VK_IMAGE_LAYOUT_GENERAL, but this will most likely be slow. For optimal performance, the source 
                 * image should be in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL and the destination image should be in 
                 * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                 * 
                 * Vulkan allows us to transition each mip level of an image independently. Each blit will only deal with 
                 * two mip levels at a time, so we can transition each level into the optimal layout between blit commands
                 * 
                 * Mip level 0
                 * Transition layout VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                 * Copy buffer to image
                 * 
                 * Mip level 1 to m_mipLevels - 1
                 * Transition layout VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                 * 
                 * Loop {
                 *          Mip level: 0
                 *          Transition layout 
                 *          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                 *          Blit: mip level 1
                 *          Transition layout 
                 *          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                 * 
                 *          Mip level: 1
                 *          Transition layout 
                 *          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                 *          Blit: mip level 2
                 *          Transition layout 
                 *          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                 *          .
                 *          .
                 *          .
                 * }
                */
                VkImageMemoryBarrier barrier;
                VkPipelineStageFlags sourceStage, destinationStage;

                int32_t mipWidth  = static_cast <int32_t> (imageInfo->meta.width);
                int32_t mipHeight = static_cast <int32_t> (imageInfo->meta.height);
                for (uint32_t i = 1; i < imageInfo->meta.mipLevels; i++) {
                    uint32_t srcMipLevel = i - 1;
                    uint32_t dstMipLevel = i;
                    /* First, we transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. This transition will wait 
                     * for level i - 1 to be filled, either from the previous blit command, or from vkCmdCopyBufferToImage.
                     * The current blit command will wait on this transition
                    */
                    transitionImageLayout (imageInfo->resource.image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           srcMipLevel,
                                           1,
                                           imageInfo->params.aspect,
                                           &barrier,
                                           sourceStage,
                                           destinationStage);

                    vkCmdPipelineBarrier (commandBuffer,
                                          sourceStage,
                                          destinationStage,
                                          0,
                                          0, VK_NULL_HANDLE,
                                          0, VK_NULL_HANDLE,
                                          1, &barrier);  

                    /* Next, we specify the regions that will be used in the blit operation. The source mip level is i - 1 
                     * and the destination mip level is i. The two elements of the srcOffsets array determine the 3D region 
                     * that data will be blitted from. dstOffsets determines the region that data will be blitted to. The 
                     * X and Y dimensions of the dstOffsets[1] are divided by two since each mip level is half the size of 
                     * the previous level. The Z dimension of srcOffsets[1] and dstOffsets[1] must be 1, since a 2D image 
                     * has a depth of 1
                    */
                    VkImageBlit blit;
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.srcOffsets[1] = {
                                            mipWidth, 
                                            mipHeight, 
                                            1
                                         };

                    blit.srcSubresource.aspectMask     = imageInfo->params.aspect;
                    blit.srcSubresource.mipLevel       = srcMipLevel;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount     = 1;

                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[1] = {
                                            mipWidth  > 1 ?  mipWidth/2: 1, 
                                            mipHeight > 1 ? mipHeight/2: 1, 
                                            1 
                                         };

                    blit.dstSubresource.aspectMask     = imageInfo->params.aspect;
                    blit.dstSubresource.mipLevel       = dstMipLevel;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount     = 1;

                    /* Note that, the source image is used for both the srcImage and dstImage parameter. This is because 
                     * we're blitting between different levels of the same image. The source mip level was just 
                     * transitioned to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL and the destination level is still in 
                     * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                     * 
                     * The last parameter allows us to specify a VkFilter to use in the blit. We use the VK_FILTER_LINEAR 
                     * to enable interpolation
                    */
                    vkCmdBlitImage (commandBuffer, 
                                    imageInfo->resource.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    imageInfo->resource.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    1, &blit,
                                    VK_FILTER_LINEAR);

                    /* To be able to start sampling from the texture image in the shader, we need one last transition to
                     * prepare it for shader access. Note that, this transition waits on the current blit command to 
                     * finish
                    */
                    transitionImageLayout (imageInfo->resource.image,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           srcMipLevel,
                                           1,
                                           imageInfo->params.aspect,
                                           &barrier,
                                           sourceStage,
                                           destinationStage);

                    vkCmdPipelineBarrier (commandBuffer,
                                          sourceStage,
                                          destinationStage,
                                          0,
                                          0, VK_NULL_HANDLE,
                                          0, VK_NULL_HANDLE,
                                          1, &barrier);   

                    /* At the end of the loop, we divide the current mip dimensions by two. We check each dimension before
                     * the division to ensure that dimension never becomes 0. This handles cases where the image is not 
                     * square, since one of the mip dimensions would reach 1 before the other dimension. When this happens,
                     * that dimension should remain 1 for all remaining levels
                    */
                    if (mipWidth > 1)  mipWidth  /= 2;
                    if (mipHeight > 1) mipHeight /= 2;                                                    
                }
                /* Transition the last mip level, since the last level was never blitted from
                */
                transitionImageLayout (imageInfo->resource.image,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       imageInfo->meta.mipLevels - 1,
                                       1,
                                       imageInfo->params.aspect,
                                       &barrier,
                                       sourceStage,
                                       destinationStage);

                vkCmdPipelineBarrier (commandBuffer,
                                      sourceStage,
                                      destinationStage,
                                      0,
                                      0, VK_NULL_HANDLE,
                                      0, VK_NULL_HANDLE,
                                      1, &barrier);                 
            }

            void beginRenderPass (uint32_t deviceInfoId,
                                  uint32_t renderPassInfoId,
                                  uint32_t swapChainImageId,
                                  const std::vector <VkClearValue>& clearValues,
                                  VkCommandBuffer commandBuffer) {
                
                auto deviceInfo     = getDeviceInfo     (deviceInfoId);
                auto renderPassInfo = getRenderPassInfo (renderPassInfoId);
                /* Drawing starts by beginning the render pass with vkCmdBeginRenderPass. The render pass is configured 
                 * using some parameters in a VkRenderPassBeginInfo struct
                */
                VkRenderPassBeginInfo beginInfo;
                beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                beginInfo.pNext = VK_NULL_HANDLE;
                /* The first parameters are the render pass itself and the attachments to bind. We created a framebuffer 
                 * for each swap chain image where it is specified as a color attachment. Thus we need to bind the 
                 * framebuffer for the swap chain image we want to draw to. Using the image index parameter which was 
                 * passed in, we can pick the right framebuffer for the current swap chain image
                */
                beginInfo.renderPass  = renderPassInfo->resource.renderPass;
                beginInfo.framebuffer = renderPassInfo->resource.frameBuffers[swapChainImageId];
                /* The next two parameters define the size of the render area. The render area defines where shader loads 
                 * and stores will take place. The pixels outside this region will have undefined values. It should match 
                 * the size of the attachments for best performance
                */
                beginInfo.renderArea.offset = {0, 0};
                beginInfo.renderArea.extent = deviceInfo->params.swapChainExtent;
                beginInfo.clearValueCount   = static_cast <uint32_t> (clearValues.size());
                beginInfo.pClearValues      = clearValues.data();

                /* The render pass can now begin. All of the functions that record commands can be recognized by their 
                 * vkCmd prefix. They all return void, so there will be no error handling until we've finished recording
                 * 
                 * The final parameter controls how the drawing commands within the render pass will be provided
                 * VK_SUBPASS_CONTENTS_INLINE
                 * The render pass commands will be embedded in the primary command buffer itself and no secondary command
                 * buffers will be executed
                 * 
                 * VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
                 * The render pass commands will be executed from secondary command buffers
                */
                vkCmdBeginRenderPass (commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
            }

            void endRenderPass (VkCommandBuffer commandBuffer) {
                vkCmdEndRenderPass (commandBuffer);
            }

            void bindPipeline (uint32_t pipelineInfoId,
                               VkPipelineBindPoint bindPoint,
                               VkCommandBuffer commandBuffer) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                vkCmdBindPipeline (commandBuffer, 
                                   bindPoint, 
                                   pipelineInfo->resource.pipeline);
            }

            void updatePushConstants (uint32_t pipelineInfoId,
                                      VkShaderStageFlags stageFlags, 
                                      uint32_t offset, uint32_t size, const void* data,
                                      VkCommandBuffer commandBuffer) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                vkCmdPushConstants (commandBuffer,
                                    pipelineInfo->resource.layout,
                                    stageFlags, 
                                    offset, size, data);
            }

            void bindVertexBuffers (const std::vector <uint32_t>& bufferInfoIds,
                                    uint32_t firstBinding,
                                    const std::vector <VkDeviceSize>& offsets,
                                    VkCommandBuffer commandBuffer) {
                
                std::vector <VkBuffer> vertexBuffers;
                /* The vkCmdBindVertexBuffers function is used to bind vertex buffers to bindings. The first two 
                 * parameters, besides the command buffer, specify the offset and number of bindings we're going to 
                 * specify vertex buffers for. The last two parameters specify the array of vertex buffers to bind and 
                 * the byte offsets to start reading vertex data from
                */
                for (auto const& infoId: bufferInfoIds) {
                    auto bufferInfo = getBufferInfo (infoId, VERTEX_BUFFER);
                    vertexBuffers.push_back (bufferInfo->resource.buffer);
                }

                vkCmdBindVertexBuffers (commandBuffer, 
                                        firstBinding, 
                                        static_cast <uint32_t> (vertexBuffers.size()), 
                                        vertexBuffers.data(), 
                                        offsets.data());
            }

            void bindIndexBuffer (uint32_t bufferInfoId,
                                  VkDeviceSize offset,
                                  VkIndexType indexType,
                                  VkCommandBuffer commandBuffer) {
                
                auto bufferInfo = getBufferInfo (bufferInfoId, INDEX_BUFFER);
                /* The vkCmdBindIndexBuffer binds the index buffer, just like we did for the vertex buffer. The 
                 * difference is that you can only have a single index buffer. It's unfortunately not possible to use 
                 * different indices for each vertex attribute, so we do still have to completely duplicate vertex data 
                 * even if just one attribute varies
                */
                vkCmdBindIndexBuffer (commandBuffer, 
                                      bufferInfo->resource.buffer, 
                                      offset, 
                                      indexType);
            }

            void bindDescriptorSets (uint32_t pipelineInfoId,
                                     VkPipelineBindPoint bindPoint,
                                     uint32_t firstSet,
                                     const std::vector <VkDescriptorSet>& descriptorSets,
                                     const std::vector <uint32_t>& dynamicOffsets,
                                     VkCommandBuffer commandBuffer) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore we 
                 * need to specify if we want to bind descriptor sets to the graphics or compute pipeline. The next 
                 * parameter is the pipeline layout that the descriptors are based on
                 * 
                 * The next three parameters specify the index of the first descriptor set, the number of sets to bind, 
                 * and the array of sets to bind
                 * 
                 * The last two parameters specify an array of offsets that are used for dynamic descriptors
                */
                vkCmdBindDescriptorSets (commandBuffer, 
                                         bindPoint, 
                                         pipelineInfo->resource.layout, 
                                         firstSet, 
                                         static_cast <uint32_t> (descriptorSets.size()), 
                                         descriptorSets.data(), 
                                         static_cast <uint32_t> (dynamicOffsets.size()), 
                                         dynamicOffsets.data());
            }

            void draw (uint32_t vertexCount, 
                       uint32_t instanceCount, 
                       uint32_t firstVertex, 
                       uint32_t firstInstance,
                       VkCommandBuffer commandBuffer) {

                vkCmdDraw (commandBuffer, 
                           vertexCount, 
                           instanceCount, 
                           firstVertex, 
                           firstInstance);
            }

            void drawIndexed (uint32_t indicesCount,
                              uint32_t instanceCount, 
                              uint32_t firstIndex, 
                              int32_t vertexOffset, 
                              uint32_t firstInstance,
                              VkCommandBuffer commandBuffer) {
                
                /* InstanceCount: Used for instanced rendering, use 1 if you're not doing that
                 * firstIndex:    Specifies an offset into the index buffer, using a value of 1 would cause the graphics 
                 *                card to start reading at the second index
                 * vertexOffset:  Specifies the value added to the vertex index before indexing into the vertex buffer
                 * firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex
                */
                vkCmdDrawIndexed (commandBuffer, 
                                  indicesCount, 
                                  instanceCount, 
                                  firstIndex, 
                                  vertexOffset, 
                                  firstInstance);
            }
    };
}   // namespace Core
#endif  // VK_CMD_H