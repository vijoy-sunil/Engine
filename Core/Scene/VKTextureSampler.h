#ifndef VK_TEXTURE_SAMPLER_H
#define VK_TEXTURE_SAMPLER_H

#include "VKSceneMgr.h"
#include "../Device/VKDeviceMgr.h"

using namespace Collections;

namespace Renderer {
    class VKTextureSampler: protected virtual VKSceneMgr,
                            protected virtual VKDeviceMgr {
        private:
            static Log::Record* m_VKTextureSamplerLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKTextureSampler (void) {
                m_VKTextureSamplerLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKTextureSampler (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* It is possible for shaders to read texels directly from images, but that is not very common when they are 
             * used as textures. Textures are usually accessed through samplers, which will apply filtering and 
             * transformations to compute the final color that is retrieved. These filters are helpful to deal with 
             * problems like
             * 
             * (1) Oversampling
             * Consider a texture that is mapped to geometry with more fragments than texels. If you simply took the 
             * closest texel for the texture coordinate in each fragment, then you would get a result like miecraft
             * blocky texture. Whereras, if you combined the 4 closest texels through linear interpolation, then you 
             * would get a smoother result. A sampler object automatically applies this filtering for you when reading a 
             * color from the texture
             * 
             * (2) Undersampling
             * Undersampling is the opposite problem, where you have more texels than fragments. This will lead to 
             * artifacts when sampling high frequency patterns like a checkerboard texture at a sharp angle, for example. 
             * The solution to this is anisotropic filtering, which can also be applied automatically by a sampler
             * 
             * (3) Transformations
             * Aside from these filters, a sampler can also take care of transformations. It determines what happens when 
             * you try to read texels outside the image through its addressing mode
            */
            void createTextureSampler (uint32_t sceneInfoId, 
                                       VkFilter filter,
                                       VkSamplerAddressMode addressMode,
                                       VkBool32 anisotropyEnable,
                                       VkSamplerMipmapMode mipMapMode,
                                       float minLod, 
                                       float maxLod) {

                auto sceneInfo  = getSceneInfo (sceneInfoId); 
                auto deviceInfo = getDeviceInfo();
                /* Samplers are configured through a VkSamplerCreateInfo structure, which specifies all filters and 
                 * transformations that it should apply
                */
                VkSamplerCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified. 
                 * Magnification concerns the oversampling problem described above, and minification concerns 
                 * undersampling
                 * (https://www.gamedevelopment.blog/wp-content/uploads/2017/11/nearest-vs-linear-texture-filter.png)
                */
                createInfo.magFilter = filter;
                createInfo.minFilter = filter;
                /* The addressing mode can be specified per axis using the addressMode fields. Note that the axes are 
                 * called U, V and W instead of X, Y and Z. This is a convention for texture space coordinates
                 * 
                 * (1) VK_SAMPLER_ADDRESS_MODE_REPEAT
                 * Repeat the texture when going beyond the image dimensions
                 * 
                 * (2) VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
                 * Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions
                 * 
                 * (3) VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                 * Take the color of the edge closest to the coordinate beyond the image dimensions
                 * 
                 * (4) VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
                 * Like clamp to edge, but instead uses the edge opposite to the closest edge
                 * 
                 * (5) VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
                 * Return a solid color when sampling beyond the dimensions of the image
                 * 
                 * Note that, the repeat mode is probably the most common mode, because it can be used to tile textures 
                 * like floors and walls
                */
                createInfo.addressModeU = addressMode;
                createInfo.addressModeV = addressMode;
                createInfo.addressModeW = addressMode;
                /* The next two fields specify if anisotropic filtering should be used. There is no reason not to use this 
                 * unless performance is a concern. The maxAnisotropy field limits the amount of texel samples that can 
                 * be used to calculate the final color. A lower value results in better performance, but lower quality 
                 * results. To figure out which value we can use, we need to retrieve the properties of the physical 
                 * device
                */
                createInfo.anisotropyEnable = anisotropyEnable;
                /* Note that, VkPhysicalDeviceProperties structure contains a VkPhysicalDeviceLimits member named limits. 
                 * This struct in turn has a member called maxSamplerAnisotropy and this is the maximum value we can 
                 * specify for maxAnisotropy. If we want to go for maximum quality, we can simply use that value directly
                */
                anisotropyEnable == VK_TRUE ? createInfo.maxAnisotropy = deviceInfo->params.maxSamplerAnisotropy:
                                              createInfo.maxAnisotropy = 1.0f;

                /* The borderColor field specifies which color is returned when sampling beyond the image with clamp to 
                 * border addressing mode. It is possible to return black, white or transparent in either float or int 
                 * formats. You cannot specify an arbitrary color
                */
                createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; 
                /* The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels 
                 * in an image. If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) 
                 * and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the [0, 1) range on 
                 * all axes. Real-world applications almost always use normalized coordinates, because then it's possible 
                 * to use textures of varying resolutions with the exact same coordinates
                */  
                createInfo.unnormalizedCoordinates = VK_FALSE;
                /* If a comparison function is enabled, then texels will first be compared to a value, and the result of 
                 * that comparison is used in filtering operations. This is mainly used for percentage-closer filtering 
                 * on shadow maps
                */
                createInfo.compareEnable = VK_FALSE;
                createInfo.compareOp     = VK_COMPARE_OP_ALWAYS;  
                /* While the VkImage holds the mipmap data, VkSampler controls how that data is read while rendering. 
                 * Vulkan allows us to specify minLod, maxLod, mipLodBias, and mipmapMode ("Lod" means "Level of Detail"). 
                 * When a texture is sampled, the sampler selects a mip level according to the following pseudocode
                 * 
                 * lod = getLodLevelFromScreenSize();
                 * lod is smaller when the object is close, may be negative
                 *   
                 * lod = clamp (lod + mipLodBias, minLod, maxLod);
                 * 
                 * level is clamped to the number of mip levels in the texture image
                 * level = clamp (floor (lod), 0, texture.mipLevels - 1); 
                 * 
                 * if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
                 *      color = sample (level);
                 * else
                 *      color = blend (sample (level), sample (level + 1));
                 * 
                 * If mipmapMode is VK_SAMPLER_MIPMAP_MODE_NEAREST, lod selects the mip level to sample from. If the 
                 * mipmap mode is VK_SAMPLER_MIPMAP_MODE_LINEAR, lod is used to select two mip levels to be sampled. Those
                 * levels are sampled and the results are linearly blended
                 * 
                 * if (lod <= 0)
                 *      color = readTexture (uv, magFilter);
                 * else
                 *      color = readTexture (uv, minFilter);
                 * 
                 * lod is also used to select between magFilter and minFilter. If the object is close to the camera, 
                 * magFilter is used as the filter. If the object is further from the camera, minFilter is used
                 * 
                 * It can be seen that, mipLodBias lets us force Vulkan to use lower lod and level than it would normally 
                 * use
                */
                createInfo.mipmapMode = mipMapMode;
                createInfo.mipLodBias = 0.0f;
                createInfo.minLod     = minLod;
                createInfo.maxLod     = maxLod;

                /* Note the sampler does not reference a VkImage anywhere. The sampler is a distinct object that provides 
                 * an interface to extract colors from a texture. It can be applied to any image you want, whether it is 
                 * 1D, 2D or 3D. This is different from many older APIs, which combined texture images and filtering into 
                 * a single state
                */
                VkSampler textureSampler;
                VkResult result = vkCreateSampler (deviceInfo->shared.logDevice, 
                                                   &createInfo, 
                                                   VK_NULL_HANDLE, 
                                                   &textureSampler);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKTextureSamplerLog) << "Failed to create texture sampler "
                                                      << "[" << sceneInfoId << "]"
                                                      << " "
                                                      << "[" << string_VkResult (result) << "]"
                                                      << std::endl;
                    throw std::runtime_error ("Failed to create texture sampler");
                } 
                sceneInfo->resource.textureSampler = textureSampler;
            }

            void cleanUp (uint32_t sceneInfoId) {
                auto sceneInfo  = getSceneInfo (sceneInfoId);
                auto deviceInfo = getDeviceInfo();

                vkDestroySampler (deviceInfo->shared.logDevice, 
                                  sceneInfo->resource.textureSampler, 
                                  VK_NULL_HANDLE);
            }
    };

    Log::Record* VKTextureSampler::m_VKTextureSamplerLog;
}   // namespace Renderer
#endif  // VK_TEXTURE_SAMPLER_H