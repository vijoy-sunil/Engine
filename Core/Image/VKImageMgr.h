#ifndef VK_IMAGE_MGR_H
#define VK_IMAGE_MGR_H

#include "../Device/VKPhyDevice.h"

using namespace Collections;

namespace Core {
    class VKImageMgr: protected virtual VKPhyDevice {
        private:
            struct ImageInfo {
                struct Meta {
                    uint32_t id;
                    uint32_t width;
                    uint32_t height;
                    /* In Vulkan, each of the mip images is stored in different mip levels of a VkImage. Mip level 0 is 
                     * the original image, and the mip levels after level 0 are commonly referred to as the mip chain. 
                     * The number of mip levels is calculated using image dimensions
                    */
                    uint32_t mipLevels;
                } meta;
                
                struct Resource {
                    VkImage image;
                    VkDeviceMemory imageMemory;
                    VkImageView imageView;
                } resource;

                struct Parameters {
                    VkImageLayout initialLayout;
                    VkFormat format;
                    VkImageUsageFlags usage;
                    VkSampleCountFlagBits sampleCount;
                    VkImageTiling tiling;
                    VkMemoryPropertyFlags property;
                    VkSharingMode sharingMode;
                    VkImageAspectFlags aspect;
                } params;

                struct Allocation {
                    VkDeviceSize size;
                    uint32_t memoryTypeBits;
                    uint32_t memoryTypeIndex;
                } allocation;

                bool operator == (const ImageInfo& other) const {
                    return meta.id == other.meta.id;
                }  
            };
            std::map <e_imageType, std::vector <ImageInfo>> m_imageInfoPool;

            static Log::Record* m_VKImageMgrLog;
            const uint32_t m_instanceId = g_collectionsId++;

            /* Helper function that tells us if the format contains a stencil component
            */
            bool isStencilComponentSupported (VkFormat format) {
                return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
            }

            void deleteImageInfo (ImageInfo* imageInfo, e_imageType type) {
                if (m_imageInfoPool.find (type) != m_imageInfoPool.end()) {
                    auto& infos = m_imageInfoPool[type];

                    infos.erase (std::remove (infos.begin(), infos.end(), *imageInfo), infos.end());
                    m_imageInfoPool[type] = infos;
                    return;
                }

                LOG_ERROR (m_VKImageMgrLog) << "Failed to delete image info "
                                            << "[" << imageInfo->meta.id << "]"
                                            << " "
                                            << "[" << Utils::getImageTypeString (type) << "]"           
                                            << std::endl;
                throw std::runtime_error ("Failed to delete image info");              
            }

        public:
            VKImageMgr (void) {
                m_VKImageMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
                /* Create a type void image, since the image info struct is private, there may be cases where we need
                 * its type. Using the get function with an auto will help to resolve this
                */
                ImageInfo info{};
                m_imageInfoPool[VOID_IMAGE].push_back (info);
            }

            ~VKImageMgr (void) {
                LOG_CLOSE (m_instanceId);
            }
            
        protected:
            /* The below function takes a list of candidate formats in order from most desirable to least desirable, and 
             * checks which is the first one that supports desired tiling mode and format features
            */
            VkFormat getSupportedFormat (uint32_t deviceInfoId,
                                         const std::vector <VkFormat>& formatCandidates, 
                                         VkImageTiling tiling, 
                                         VkFormatFeatureFlags features) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);

                LOG_INFO (m_VKImageMgrLog) << "Required features"
                                           << std::endl;   
                auto flags = Utils::getSplitString (string_VkFormatFeatureFlags (features), "|");
                for (auto const& flag: flags)
                LOG_INFO (m_VKImageMgrLog) << "[" << flag << "]" 
                                           << std::endl;

                for (auto const& format: formatCandidates) {
                    VkFormatProperties properties;
                    vkGetPhysicalDeviceFormatProperties (deviceInfo->resource.phyDevice, format, &properties);

                    if (tiling == VK_IMAGE_TILING_LINEAR && 
                       (properties.linearTilingFeatures & features) == features)                   
                        return format;

                    if (tiling == VK_IMAGE_TILING_OPTIMAL && 
                       (properties.optimalTilingFeatures & features) == features)
                        return format;
                }
                
                LOG_ERROR (m_VKImageMgrLog) << "Failed to find supported format "
                                            << "[" << deviceInfoId << "]" 
                                            << std::endl;
                throw std::runtime_error ("Failed to find supported format");
            }

            /* To use any VkImage, including those in the swap chain, in the render pipeline we have to create a 
             * VkImageView object. An image view is quite literally a view into an image. It describes how to access the 
             * image and which part of the image to access
            */
            void createImageView (ImageInfo* imageInfo,
                                  uint32_t deviceInfoId,
                                  e_imageType type, 
                                  uint32_t baseMipLevel,
                                  VkImage image) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                for (auto const& info: m_imageInfoPool[type]) {
                    if (info.meta.id == imageInfo->meta.id) {
                        LOG_ERROR (m_VKImageMgrLog) << "Image info id already exists " 
                                                    << "[" << imageInfo->meta.id << "]"
                                                    << " "
                                                    << "[" << Utils::getImageTypeString (type) << "]"
                                                    << std::endl;
                        throw std::runtime_error ("Image info id already exists");
                    }
                }

                VkImageViewCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                createInfo.image = image;
                /* The viewType and format fields specify how the image data should be interpreted (ex: 1D/2D/3D textures)
                */
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format   = imageInfo->params.format;
                /* The components field allows you to swizzle (mix) the color channels around. ex: you can map all of the 
                 * channels to the red channel for a monochrome texture by setting all channels to VK_COMPONENT_SWIZZLE_R 
                */
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                createInfo.subresourceRange.aspectMask     = imageInfo->params.aspect;
                createInfo.subresourceRange.baseMipLevel   = baseMipLevel;
                createInfo.subresourceRange.levelCount     = imageInfo->meta.mipLevels;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount     = 1;

                VkImageView imageView;
                VkResult result = vkCreateImageView (deviceInfo->resource.logDevice, 
                                                     &createInfo, 
                                                     VK_NULL_HANDLE,
                                                     &imageView);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKImageMgrLog) << "Failed to create image view " 
                                                << "[" << imageInfo->meta.id << "]"
                                                << " "
                                                << "[" << Utils::getImageTypeString (type) << "]"
                                                << " "
                                                << "[" << string_VkResult (result) << "]"
                                                << std::endl;
                    throw std::runtime_error ("Failed to create image view");
                }

                imageInfo->resource.image     = image;
                imageInfo->resource.imageView = imageView;
                m_imageInfoPool[type].push_back (*imageInfo);
            }

            void createImageResources (uint32_t imageInfoId,
                                       uint32_t deviceInfoId,
                                       e_imageType type,
                                       uint32_t width, 
                                       uint32_t height,
                                       uint32_t mipLevels,
                                       VkImageLayout initialLayout,
                                       VkFormat format,
                                       VkImageUsageFlags usage,
                                       VkSampleCountFlagBits sampleCount,
                                       VkImageTiling tiling,
                                       VkMemoryPropertyFlags property,       
                                       const std::vector <uint32_t>& queueFamilyIndices,
                                       VkImageAspectFlags aspect) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                for (auto const& info: m_imageInfoPool[type]) {
                    if (info.meta.id == imageInfoId) {
                        LOG_ERROR (m_VKImageMgrLog) << "Image info id already exists " 
                                                    << "[" << imageInfoId << "]"
                                                    << " "
                                                    << "[" << Utils::getImageTypeString (type) << "]"
                                                    << std::endl;
                        throw std::runtime_error ("Image info id already exists");
                    }
                }

                VkImageCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                /* There are some optional flags for images that are related to sparse images. Sparse images are images 
                 * where only certain regions are actually backed by memory. If you were using a 3D texture for a voxel 
                 * terrain, for example, then you could use this to avoid allocating memory to store large volumes of 
                 * "air" values
                */
                createInfo.flags = 0; 
                /* The image type, specified in the imageType field, tells Vulkan with what kind of coordinate system the 
                 * texels in the image are going to be addressed. It is possible to create 1D, 2D and 3D images. One 
                 * dimensional images can be used to store an array of data or gradient, two dimensional images are 
                 * mainly used for textures, and three dimensional images can be used to store voxel volumes, for example
                 * 
                 * What is a texel?
                 * Pixels within an image object VkImage are known as texels. In a broader sense, when a pixel belongs to 
                 * an image used as a texture resource, it is called a 'texture pixel' or shortened to 'texel'
                */
                createInfo.imageType = VK_IMAGE_TYPE_2D;
                /* The extent field specifies the dimensions of the image, basically how many texels there are on each 
                 * axis. That's why depth must be 1 instead of 0
                */
                createInfo.extent.width  = width;
                createInfo.extent.height = height;
                createInfo.extent.depth  = 1;
                createInfo.mipLevels     = mipLevels;
                createInfo.arrayLayers   = 1;
                createInfo.format        = format;
                /* The tiling field can have one of two values
                 * (1) VK_IMAGE_TILING_LINEAR
                 * Texels are laid out in row-major order like our pixels array
                 * 
                 * (2) VK_IMAGE_TILING_OPTIMAL
                 * Texels are laid out in an implementation defined order for optimal access
                 * 
                 * Note that, unlike the layout of an image, the tiling mode cannot be changed at a later time
                */
                createInfo.tiling = tiling; 
                /* There are only two possible values for the initialLayout of an image
                 * (1) VK_IMAGE_LAYOUT_UNDEFINED
                 * Not usable by the GPU and the very first transition will discard the texels
                 * 
                 * (2) VK_IMAGE_LAYOUT_PREINITIALIZED
                 * Not usable by the GPU, but the first transition will preserve the texels
                 * 
                 * There are few situations where it is necessary for the texels to be preserved during the first 
                 * transition. One example, however, would be if you wanted to use an image as a staging image in 
                 * combination with the VK_IMAGE_TILING_LINEAR layout. In that case, you'd want to upload the texel data 
                 * to it and then transition the image to be a transfer source without losing the data. Whereas, if we
                 * are using a staging buffer, we're first going to transition the image to be a transfer destination and
                 * then copy texel data to it from a buffer object, so we don't need this property and can safely use 
                 * VK_IMAGE_LAYOUT_UNDEFINED instead
                */
                createInfo.initialLayout = initialLayout;   
                createInfo.usage         = usage;
                createInfo.samples       = sampleCount;

                if (isQueueFamiliesUnique (queueFamilyIndices)) {
                    createInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = static_cast <uint32_t> (queueFamilyIndices.size());
                    createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();                    
                }
                else {
                    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;
                    createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE; 
                }

                VkImage image;
                VkResult result = vkCreateImage (deviceInfo->resource.logDevice, &createInfo, VK_NULL_HANDLE, &image);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKImageMgrLog) << "Failed to create image " 
                                                << "[" << imageInfoId << "]"
                                                << " "
                                                << "[" << Utils::getImageTypeString (type) << "]"
                                                << " "
                                                << "[" << string_VkResult (result) << "]"
                                                << std::endl; 
                    throw std::runtime_error ("Failed to create image");
                }

                /* Allocating memory for an image works in exactly the same way as allocating memory for a buffer. Use 
                 * vkGetImageMemoryRequirements instead of vkGetBufferMemoryRequirements, and use vkBindImageMemory 
                 * instead of vkBindBufferMemory
                */
                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements (deviceInfo->resource.logDevice, image, &memRequirements);

                VkMemoryAllocateInfo allocInfo;
                allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.pNext           = VK_NULL_HANDLE;
                allocInfo.allocationSize  = memRequirements.size;
                allocInfo.memoryTypeIndex = getMemoryTypeIndex (deviceInfoId, memRequirements.memoryTypeBits, property);

                deviceInfo->meta.memoryAllocationCount++;

                VkDeviceMemory imageMemory;
                result = vkAllocateMemory (deviceInfo->resource.logDevice, &allocInfo, VK_NULL_HANDLE, &imageMemory);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKImageMgrLog) << "Failed to allocate image memory " 
                                                << "[" << imageInfoId << "]"
                                                << " "
                                                << "[" << Utils::getImageTypeString (type) << "]"
                                                << " "
                                                << "[" << string_VkResult (result) << "]"
                                                << std::endl;                     
                    throw std::runtime_error ("Failed to allocate image memory");
                }

                vkBindImageMemory (deviceInfo->resource.logDevice, image, imageMemory, 0);

                ImageInfo info;
                info.meta.id                    = imageInfoId;
                info.meta.width                 = width;
                info.meta.height                = height;
                info.meta.mipLevels             = mipLevels;
                info.resource.imageMemory       = imageMemory;
                info.params.initialLayout       = initialLayout;
                info.params.format              = format;
                info.params.usage               = usage;
                info.params.sampleCount         = sampleCount;
                info.params.tiling              = tiling;
                info.params.property            = property;
                info.params.sharingMode         = createInfo.sharingMode;
                info.params.aspect              = aspect;
                info.allocation.size            = allocInfo.allocationSize;
                info.allocation.memoryTypeBits  = memRequirements.memoryTypeBits;
                info.allocation.memoryTypeIndex = allocInfo.memoryTypeIndex;
                /* Create image view
                */
                createImageView (&info, 
                                 deviceInfoId,
                                 type, 
                                 0,
                                 image);
            }            

            /* One of the most common ways to perform layout transitions is using an image memory barrier. A pipeline 
             * barrier like this is generally used to synchronize access to resources, like ensuring that a write to a 
             * buffer completes before reading from it, but it can also be used to transition image layouts and transfer 
             * queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used. There is an equivalent buffer memory 
             * barrier to do this for buffers
            */
            void transitionImageLayout (VkImage image, 
                                        VkImageLayout oldLayout, 
                                        VkImageLayout newLayout,
                                        uint32_t baseMipLevel,
                                        uint32_t mipLevels,
                                        VkImageAspectFlags aspect,
                                        VkImageMemoryBarrier* barrier,
                                        VkPipelineStageFlags& sourceStage,
                                        VkPipelineStageFlags& destinationStage) {

                barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier->pNext = VK_NULL_HANDLE;
                /* The first two fields specify layout transition. It is possible to use VK_IMAGE_LAYOUT_UNDEFINED as 
                 * oldLayout if you don't care about the existing contents of the image
                */
                barrier->oldLayout = oldLayout;
                barrier->newLayout = newLayout;
                /* If you are using the barrier to transfer queue family ownership, then these two fields should be the 
                 * indices of the queue families. They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do 
                 * this (not the default value!)
                */
                barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                /* The image and subresourceRange specify the image that is affected and the specific part of the image
                */
                barrier->image = image;
                barrier->subresourceRange.aspectMask     = aspect; 
                barrier->subresourceRange.baseMipLevel   = baseMipLevel;
                barrier->subresourceRange.levelCount     = mipLevels;
                barrier->subresourceRange.baseArrayLayer = 0;
                barrier->subresourceRange.layerCount     = 1;
                /* Barriers are primarily used for synchronization purposes, so you must specify which types of 
                 * operations that involve the resource 'must happen before the barrier', and which operations that  
                 * involve the resource 'must wait on the barrier'. We need to do that despite already using a fence/
                 * vkQueueWaitIdle to manually synchronize
                */
                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
                    newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    /* Since the transfer writes don't have to wait on anything, you may specify an empty access mask and 
                     * the earliest possible pipeline stage VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT for the pre-barrier 
                     * operations
                    */
                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    barrier->srcAccessMask = VK_ACCESS_NONE;
                    /* Note that, even though at this point we may not have a pipeline bound, we still use the pipeline
                     * stage VK_PIPELINE_STAGE_TRANSFER_BIT. This is possible because VK_PIPELINE_STAGE_TRANSFER_BIT 
                     * pipeline stage is not a real stage within the graphics and compute pipelines, it is more of a 
                     * pseudo-stage where transfers happen
                    */
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrier->dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                }

                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
                         newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrier->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                }

                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                         newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrier->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrier->dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;                    
                } 

                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
                         newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrier->srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;                    
                }                 

                else {
                    LOG_ERROR (m_VKImageMgrLog) << "Unsupported layout transition " 
                                                << "[" << string_VkImageLayout (oldLayout) << "]"
                                                << "->"
                                                << "[" << string_VkImageLayout (newLayout) << "]"
                                                << std::endl;
                    throw std::runtime_error ("Unsupported layout transition");
                }
            }

            ImageInfo* getImageInfo (uint32_t imageInfoId, e_imageType type) {
                if (m_imageInfoPool.find (type) != m_imageInfoPool.end()) {
                    auto& infos = m_imageInfoPool[type];
                    for (auto& info: infos) {
                        if (info.meta.id == imageInfoId) return &info;
                    }
                }

                LOG_ERROR (m_VKImageMgrLog) << "Failed to find image info "
                                            << "[" << imageInfoId << "]"
                                            << " "
                                            << "[" << Utils::getImageTypeString (type) << "]"                                         
                                            << std::endl;
                throw std::runtime_error ("Failed to find image info");
            }

            void dumpImageInfoPool (void) {
                LOG_INFO (m_VKImageMgrLog) << "Dumping image info pool"
                                           << std::endl;

                for (auto const& [key, val]: m_imageInfoPool) {
                    LOG_INFO (m_VKImageMgrLog) << "Type " 
                                               << "[" << Utils::getImageTypeString (key) << "]"
                                               << std::endl;
                    
                    for (auto const& info: val) {
                        LOG_INFO (m_VKImageMgrLog) << "Id "
                                                   << "[" << info.meta.id << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Dims "
                                                   << "[" << info.meta.width    << ", "
                                                          << info.meta.height   << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Mip levels "
                                                   << "[" << info.meta.mipLevels << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Initial layout "
                                                   << "[" << string_VkImageLayout (info.params.initialLayout) << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Format "
                                                   << "[" << string_VkFormat (info.params.format) << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Usage"
                                                   << std::endl;
                        auto flags = Utils::getSplitString (string_VkImageUsageFlags (info.params.usage), "|");
                        for (auto const& flag: flags)
                        LOG_INFO (m_VKImageMgrLog) << "[" << flag << "]" 
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Sample count "
                                                   << "[" << string_VkSampleCountFlagBits (info.params.sampleCount) << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Tiling "
                                                   << "[" << string_VkImageTiling (info.params.tiling) << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Property"
                                                   << std::endl;
                        auto properties = Utils::getSplitString (string_VkMemoryPropertyFlags (info.params.property), "|");
                        for (auto const& property: properties)
                        LOG_INFO (m_VKImageMgrLog) << "[" << property << "]" 
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Sharing mode "
                                                   << "[" << string_VkSharingMode (info.params.sharingMode) << "]"
                                                   << std::endl;   

                        LOG_INFO (m_VKImageMgrLog) << "Aspect "
                                                   << "[" << string_VkImageAspectFlags (info.params.aspect) << "]"
                                                   << std::endl; 

                        LOG_INFO (m_VKImageMgrLog) << "Allocation size "
                                                   << "[" << info.allocation.size << "]"
                                                   << std::endl;     

                        LOG_INFO (m_VKImageMgrLog) << "Mempry type bits "
                                                   << "[" << info.allocation.memoryTypeBits << "]"
                                                   << std::endl;    

                        LOG_INFO (m_VKImageMgrLog) << "Mempry type index "
                                                   << "[" << info.allocation.memoryTypeIndex << "]"
                                                   << std::endl;                                                                                                                                                                              
                    }
                }
            }

            void cleanUp (uint32_t imageInfoId, uint32_t deviceInfoId, e_imageType type) {
                auto imageInfo  = getImageInfo  (imageInfoId, type);
                auto deviceInfo = getDeviceInfo (deviceInfoId); 
                /* If we are cleaning up swap chain resources, we are only going to delete the associated image view. The
                 * destroy swap chain method will take care of the rest
                */
                vkDestroyImageView (deviceInfo->resource.logDevice, imageInfo->resource.imageView,   VK_NULL_HANDLE);

                if (type != SWAPCHAIN_IMAGE) {
                vkDestroyImage     (deviceInfo->resource.logDevice, imageInfo->resource.image,       VK_NULL_HANDLE);
                vkFreeMemory       (deviceInfo->resource.logDevice, imageInfo->resource.imageMemory, VK_NULL_HANDLE);
                }
                deleteImageInfo    (imageInfo, type);
            }
    };

    Log::Record* VKImageMgr::m_VKImageMgrLog;
}   // namespace Core
#endif  // VK_IMAGE_MGR_H