#ifndef VK_RASTERIZATION_H
#define VK_RASTERIZATION_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Core {
    class VKRasterization: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKRasterizationLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKRasterization (void) {
                m_VKRasterizationLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKRasterization (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createRasterizationState (uint32_t pipelineInfoId, 
                                           VkPolygonMode polygonMode,
                                           float lineWidth,
                                           VkCullModeFlags cullMode,
                                           VkFrontFace frontFace) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it 
                 * into fragments to be colored by the fragment shader. It also performs depth testing, face culling and 
                 * the scissor test, and it can be configured to output fragments that fill entire polygons or just the 
                 * edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo 
                 * structure
                 * 
                 * depth testing
                 * When an object is projected on the screen, the depth (z-value) of a generated fragment in the projected 
                 * screen image is compared to the value already stored in the buffer (depth test), and replaces it if 
                 * the new value is closer
                 * 
                 * face culling
                 * If we imagine any closed shape, each of its faces has two sides. Each side would either face the user 
                 * or show its back to the user. What if we could only render the faces that are facing the viewer? This 
                 * is exactly what face culling does
                */
                VkPipelineRasterizationStateCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                createInfo.pNext = VK_NULL_HANDLE;
                createInfo.flags = 0;
                /* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are 
                 * clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps 
                 * (technique that generates fast approximate shadows)
                */
                createInfo.depthClampEnable = VK_FALSE;
                /* If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
                 * This basically disables any output to the framebuffer
                */
                createInfo.rasterizerDiscardEnable = VK_FALSE;
                /* The polygonMode determines how fragments are generated for geometry
                 * VK_POLYGON_MODE_FILL
                 * fill the area of the polygon with fragments
                 * 
                 * VK_POLYGON_MODE_LINE
                 * polygon edges are drawn as lines
                 * 
                 * VK_POLYGON_MODE_POINT
                 * polygon vertices are drawn as points
                */
                createInfo.polygonMode = polygonMode;
                /* The lineWidth describes the thickness of lines in terms of number of fragments
                */
                createInfo.lineWidth = lineWidth;
                /* The cullMode variable determines the type of face culling to use. You can disable culling, cull the 
                 * front faces, cull the back faces or both. The frontFace variable specifies the vertex order for faces 
                 * to be considered front-facing and can be clockwise or counterclockwise
                */
                createInfo.cullMode  = cullMode;
                createInfo.frontFace = frontFace;
                /* The rasterizer can alter the depth values by adding a constant value or biasing them based on a 
                 * fragment's slope. This is sometimes used for shadow mapping
                */
                createInfo.depthBiasEnable         = VK_FALSE;
                createInfo.depthBiasConstantFactor = 0.0f; 
                createInfo.depthBiasClamp          = 0.0f; 
                createInfo.depthBiasSlopeFactor    = 0.0f;  

                pipelineInfo->state.rasterization  = createInfo;
            }
    };
}   // namespace Core
#endif  // VK_RASTERIZATION_H