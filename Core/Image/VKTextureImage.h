#ifndef VK_TEXTURE_IMAGE_H
#define VK_TEXTURE_IMAGE_H
/* We'll be using the stb_image library from the stb collection for loading images. Note that, the header only defines the 
 * prototypes of the functions by default. We need to include the header with the STB_IMAGE_IMPLEMENTATION definition to 
 * include the function bodies, otherwise we'll get linking errors
*/
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "VKImageMgr.h"
#include "../Buffer/VKBufferMgr.h"

using namespace Collections;

namespace Core {
    class VKTextureImage: protected virtual VKImageMgr,
                          protected virtual VKBufferMgr {
        private:
            Log::Record* m_VKTextureImageLog;
            const uint32_t m_instanceId = g_collectionsId++; 

        public:
            VKTextureImage (void) {
                m_VKTextureImageLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKTextureImage (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* We've already worked with image objects before, but those were automatically created by the swap chain 
             * extension. This time we'll have to create one by ourselves. Creating an image and filling it with data is 
             * similar to vertex buffer creation. We'll start by creating a 'staging resource' and filling it with pixel 
             * data and then we copy this to the final image object that we'll use for rendering
             * 
             * Although it is possible to create a 'staging image' for this purpose, Vulkan also allows you to copy 
             * pixels from a VkBuffer to an image and the API for this is actually faster on some hardware. We'll first 
             * create this buffer and fill it with pixel values, and then we'll create an image to copy the pixels to. 
             * Creating an image is not very different from creating buffers. It involves querying the memory 
             * requirements, allocating device memory and binding it
             * 
             * However, there is something extra that we'll have to take care of when working with images. Images can have 
             * different layouts that affect how the pixels are organized in memory. Due to the way graphics hardware 
             * works, simply storing the pixels row by row may not lead to the best performance, for example. When 
             * performing any operation on images, you must make sure that they have the layout that is optimal for use 
             * in that operation
             * 
             * Some of these image flags are
             * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
             * Optimal for presentation
             * 
             * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
             * Optimal as attachment for writing colors from the fragment shader
             * 
             * VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
             * Optimal as source in a transfer operation, like vkCmdCopyImageToBuffer
             * 
             * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             * Optimal as destination in a transfer operation, like vkCmdCopyBufferToImage
             * 
             * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             * Optimal for sampling from a shader
            */
            void createTextureResources (uint32_t imageInfoId, 
                                         uint32_t deviceInfoId, 
                                         const char* imageFilePath) {
                                            
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                int width, height, channels;
                /* The stbi_load function takes the file path and number of channels to load as arguments. The 
                 * STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn't have one, 
                 * which is nice for consistency with other textures (if any). The middle three parameters are outputs 
                 * for the width, height and actual number of channels in the image
                 * 
                 * The pointer that is returned is the first element in an array of pixel values
                */
                stbi_uc* pixels = stbi_load (imageFilePath, 
                                             &width, 
                                             &height, 
                                             &channels, 
                                             STBI_rgb_alpha);
                /* Calculate the number of levels in the mip chain. The max function selects the largest dimension. The 
                 * log2 function calculates how many times that dimension can be divided by 2. The floor function handles 
                 * cases where the largest dimension is not a power of 2. 1 is added so that the original image has a mip 
                 * level
                */
                uint32_t mipLevels = static_cast <uint32_t> (std::floor 
                                                            (std::log2 
                                                            (std::max (width, height)))) + 1;

                if (!pixels) {
                    LOG_ERROR (m_VKTextureImageLog) << "Failed to load texture image " 
                                                    << "[" << imageInfoId << "]"
                                                    << " "
                                                    << "[" << imageFilePath << "]"
                                                    << std::endl;
                    throw std::runtime_error ("Failed to load texture image");
                }

                /* The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgb_alpha for a total of 
                 * texWidth * texHeight * 4 values
                */
                VkDeviceSize size = static_cast <VkDeviceSize> (width * height * 4);
                auto stagingBufferShareQueueFamilyIndices = std::vector {
                    deviceInfo->meta.transferFamilyIndex.value()
                };
                /* Create staging buffer, the buffer should be in host visible memory so that we can map it and it 
                 * should be usable as a transfer source so that we can copy it to an image later on
                */
                createBuffer (imageInfoId, 
                              deviceInfoId,
                              STAGING_BUFFER_TEX,
                              size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              stagingBufferShareQueueFamilyIndices);

                /* Now, we can directly copy the pixel values that we got from the image loading library to the buffer
                */
                auto bufferInfo = getBufferInfo (imageInfoId, STAGING_BUFFER_TEX);
                vkMapMemory (deviceInfo->resource.logDevice, 
                             bufferInfo->resource.bufferMemory, 
                             0, 
                             size, 
                             0, 
                             &bufferInfo->meta.bufferMapped);
                memcpy (bufferInfo->meta.bufferMapped, pixels, static_cast <size_t> (size));
                vkUnmapMemory (deviceInfo->resource.logDevice, bufferInfo->resource.bufferMemory);

                /* Clean up the original pixel array
                */
                stbi_image_free (pixels);

                /* Although we could set up the shader to access the pixel values in the buffer, it's better to use image 
                 * objects in Vulkan for this purpose. Image objects will make it easier and faster to retrieve colors 
                 * by allowing us to use 2D coordinates
                 * 
                 * format
                 * Vulkan supports many possible image formats, but we should use the same format for the texels as the 
                 * pixels in the buffer, otherwise the copy operation will fail
                 * 
                 * tiling
                 * If you want to be able to directly access texels in the memory of the image, then you must use 
                 * VK_IMAGE_TILING_LINEAR. We will be using a staging buffer instead of a staging image, so this won't be 
                 * necessary. We will be using VK_IMAGE_TILING_OPTIMAL for efficient access from the shader
                 *
                 * usage 
                 * The usage field has the same semantics as the one during buffer creation. The image is going to be 
                 * used as destination for the buffer copy, so it should be set up as a transfer destination. We also 
                 * want to be able to access the image from the shader to color our mesh, so the usage should include 
                 * VK_IMAGE_USAGE_SAMPLED_BIT
                */

                /* We will be using vkCmdBlitImage to generate all the mip levels, which is quiet convenient, but 
                 * unfortunately it is not guaranteed to be supported on all platforms. It requires the image format 
                 * we use to support linear filtering. There are two alternatives in this case. You could implement a 
                 * function that searches common texture image formats for one that does support linear blitting, or you 
                 * could implement the mipmap generation in software with a library like stb_image_resize. Each mip level 
                 * can then be loaded into the image in the same way that you loaded the original image.
                 * 
                 * It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anyway. 
                 * Usually they are pregenerated and stored in the texture file alongside the base level to improve 
                 * loading speed
                */
                auto formatCandidates = std::vector {
                    VK_FORMAT_R8G8B8A8_SRGB
                };
                auto format = getSupportedFormat (deviceInfoId,
                                                  formatCandidates,
                                                  VK_IMAGE_TILING_OPTIMAL,
                                                  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

                auto imageShareQueueFamilyIndices = std::vector {
                    deviceInfo->meta.graphicsFamilyIndex.value(),
                    deviceInfo->meta.transferFamilyIndex.value()
                };
                createImageResources (imageInfoId, 
                                      deviceInfoId,
                                      TEXTURE_IMAGE,
                                      static_cast <uint32_t> (width),
                                      static_cast <uint32_t> (height),
                                      mipLevels,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      format,
                                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                      VK_IMAGE_USAGE_SAMPLED_BIT,
                                      VK_SAMPLE_COUNT_1_BIT,
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      imageShareQueueFamilyIndices,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
            }
    };
}   // namespace Core
#endif  // VK_TEXTURE_IMAGE_H