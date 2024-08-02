#ifndef VK_MULTI_SAMPLE_IMAGE_H
#define VK_MULTI_SAMPLE_IMAGE_H

#include "VKImageMgr.h"

using namespace Collections;

namespace Renderer {
    class VKMultiSampleImage: protected virtual VKImageMgr {
        private:
            static Log::Record* m_VKMultiSampleImageLog;
            const uint32_t m_instanceId = g_collectionsId++; 

        public:
            VKMultiSampleImage (void) {
                m_VKMultiSampleImageLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKMultiSampleImage (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* With mipmapping, we are able to load multiple levels of detail for textures which fixes artifacts when 
             * rendering objects far away from the viewer. However, upon closer inspection you will notice jagged saw-like 
             * patterns along the edges of drawn geometric shapes. This undesired effect is called "aliasing" and it's a 
             * result of a limited numbers of pixels that are available for rendering
             * 
             * Since there are no displays out there with unlimited resolution, it will be always visible to some extent. 
             * There's a number of ways to fix this and on one of the more popular ones is Multisample anti-aliasing 
             * (MSAA)
             * 
             * In ordinary rendering, the pixel color is determined based on a single sample point which in most cases is 
             * the center of the target pixel on screen. If part of the drawn line passes through a certain pixel but 
             * doesn't cover the sample point, that pixel will be left blank, leading to the jagged "staircase" effect.
             * What MSAA does is it uses multiple sample points per pixel (hence the name) to determine its final color. 
             * As one might expect, more samples lead to better results, however it is also more computationally 
             * expensive
            */
            void createMultiSampleResources (uint32_t imageInfoId, uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                auto imageShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value()
                };
                /* Note that, we're also using only one mip level, since this is enforced by the Vulkan specification in 
                 * case of images with more than one sample per pixel. Also, this color buffer doesn't need mipmaps since 
                 * it's not going to be used as a texture
                 * 
                 * Lazy allocation in Vulkan
                 * Consider an example of deferred rendering. You need g-buffers. But you're going to fill them up during 
                 * the g-buffer pass, and you'll consume them during the lighting pass(es). After that point, you won't be
                 * using their contents again. For many renderers, this doesn't really matter. But with a tile-based 
                 * renderer, it can. Why? Because if a tile is big enough to store all of the g-buffer data all at once, 
                 * then the implementation doesn't actually need to write the g-buffer data out to memory. It can just 
                 * leave everything in tile memory, do the lighting pass(es) within the tile (you read them as input 
                 * attachments), and then forget they exist
                 * 
                 * But Vulkan requires that images have memory bound to them before they can be used. Lazy memory exists 
                 * so that you can fulfill that requirement while letting the implementation know that you aren't really 
                 * going to use this memory. Or more to the point, actual memory will only be allocated if you do 
                 * something that requires it
                 * 
                 * Depth buffers and depth/stencil buffers could be lazily allocated too, so long as you don't need to 
                 * access them like regular images. Note that, it's not about a way to make stenciling or depth testing 
                 * optional. It's about making their backing storage ephemeral, memory that can live within a TBR's tile 
                 * and nowhere else. You're still doing the operations; it's just not taking up actual memory
                 * 
                 * VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT flag specifies that implementations may support using memory 
                 * allocations with the VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT to back an image with this usage. This 
                 * bit can be set for any image that can be used to create a VkImageView suitable for use as a color, 
                 * resolve, depth/stencil, or input attachment. Note that, memory types must not have both 
                 * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT and VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set
                */
                createImageResources (imageInfoId, 
                                      MULTISAMPLE_IMAGE,
                                      deviceInfo->unique[resourceId].swapChain.extent.width,
                                      deviceInfo->unique[resourceId].swapChain.extent.height,
                                      1,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      deviceInfo->unique[resourceId].swapChain.format,
                                      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | 
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                                      deviceInfo->params.maxSampleCount,
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
                                      imageShareQueueFamilyIndices,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
            }
    };

    Log::Record* VKMultiSampleImage::m_VKMultiSampleImageLog;
}   // namespace Renderer
#endif  // VK_MULTI_SAMPLE_IMAGE_H