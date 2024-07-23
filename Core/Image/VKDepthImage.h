#ifndef VK_DEPTH_IMAGE_H
#define VK_DEPTH_IMAGE_H

#include "VKImageMgr.h"

using namespace Collections;

namespace Renderer {
    /* Without a depth buffer, fragments of a geometry that should have been drawn over the fragments of another geometry
     * based on the z position attribute will not be rasterized properly as it would solely depend on the ordering in the 
     * index array. There are two ways to solve this
     * 
     * (1) Sort all of the draw calls by depth from back to front
     * (2) Use depth testing with a depth buffer
     * 
     * The first approach is commonly used for drawing transparent objects, because order-independent transparency is a 
     * difficult challenge to solve. However, the problem of ordering fragments by depth is much more commonly solved 
     * using a depth buffer
     * 
     * A depth buffer is an additional attachment that stores the depth for every position, just like the color attachment 
     * stores the color of every position. Every time the rasterizer produces a fragment, the depth test will check if 
     * the new fragment is closer than the previous one. If it isn't, then the new fragment is discarded. A fragment that 
     * passes the depth test writes its own depth to the depth buffer
     * 
     * In short, the depth buffer will be read from to perform depth tests to see if a fragment is visible, and will be 
     * written to when a new fragment is drawn. The reading happens in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT 
     * stage and the writing in the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
     * 
     * The pipeline stages looks roughly like this as the draw call passes through 
     *                              t|...|ef|fs|lf|co|b
     * t  ... TOP_OF_PIPE
     * ef ... EARLY_FRAGMENT_TESTS
     * fs ... FRAGMENT_SHADER
     * lf ... LATE_FRAGMENT_TESTS
     * co ... COLOR_ATTACHMENT_OUTPUT
     * b  ... BOTTOM_OF_PIPE
     * 
    */
    class VKDepthImage: protected virtual VKImageMgr {
        private:
            static Log::Record* m_VKDepthImageLog;
            const uint32_t m_instanceId = g_collectionsId++; 

        public:
            VKDepthImage (void) {
                m_VKDepthImageLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKDepthImage (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Creating a depth image is fairly straightforward. It should have the same resolution as the color 
             * attachment, defined by the swap chain extent, an image usage appropriate for a depth attachment, optimal 
             * tiling and device local memory
            */
            void createDepthResources (uint32_t imageInfoId, uint32_t resourceId) {
                auto deviceInfo = getDeviceInfo();
                /* What is the right format for a depth image?
                 * Unlike the texture image, we don't necessarily need a specific format, because we won't be directly 
                 * accessing the texels from the program. It just needs to have a reasonable accuracy, at least 24 bits
                 * is common in real-world applications. There are several formats that fit this requirement
                 * 
                 * (1) VK_FORMAT_D32_SFLOAT: 32-bit float for depth
                 * (2) VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
                 * (3) VK_FORMAT_D24_UNORM_S8_UINT: 24-bit float for depth and 8 bit stencil component
                 * 
                 * The stencil component is used for stencil tests, which is an additional test that can be combined with
                 * depth testing
                 * 
                 * We could simply go for the VK_FORMAT_D32_SFLOAT format, because support for it is extremely common, but
                 * it's nice to add some extra flexibility to our application where possible (pick format function
                 * offers such flexibility). Note that, the support of a format also depends on the tiling mode and usage, 
                 * so we must include them in the function as well
                */
                auto formatCandidates = std::vector {
                    VK_FORMAT_D32_SFLOAT, 
                    VK_FORMAT_D32_SFLOAT_S8_UINT, 
                    VK_FORMAT_D24_UNORM_S8_UINT
                };
                VkFormat format = getSupportedFormat (formatCandidates,
                                                      VK_IMAGE_TILING_OPTIMAL,
                                                      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

                auto imageShareQueueFamilyIndices = std::vector {
                    deviceInfo->unique[resourceId].indices.graphicsFamily.value()
                };
                createImageResources (imageInfoId, 
                                      DEPTH_IMAGE,
                                      deviceInfo->unique[resourceId].swapChain.extent.width,
                                      deviceInfo->unique[resourceId].swapChain.extent.height,
                                      1,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      format,
                                      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                      deviceInfo->params.sampleCount,
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
                                      imageShareQueueFamilyIndices,
                                      VK_IMAGE_ASPECT_DEPTH_BIT);
                /* Note that, we don't need to explicitly transition the layout of the image to a depth attachment because
                 * the render pass will take care of this
                */
            }
    };

    Log::Record* VKDepthImage::m_VKDepthImageLog;
}   // namespace Renderer
#endif  // VK_DEPTH_IMAGE_H