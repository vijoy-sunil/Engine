#ifndef VK_PUSH_CONSTANT_RANGE_H
#define VK_PUSH_CONSTANT_RANGE_H

#include "VKPipelineMgr.h"

using namespace Collections;

namespace Renderer {
    class VKPushConstantRange: protected virtual VKPipelineMgr {
        private:
            static Log::Record* m_VKPushConstantRangeLog;
            const uint32_t m_instanceId = g_collectionsId++;
            
        public:
            VKPushConstantRange (void) {
                m_VKPushConstantRangeLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
            }

            ~VKPushConstantRange (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Shaders in Vulkan usually access information stored in memory through a descriptor resource. Push constants
             * aren’t descriptors though; they live outside of that system. Instead of having a piece of user-allocated
             * memory storage, push constant storage is ephemeral. When you bind a program pipeline, you are effectively
             * creating a few bytes of push constant storage memory. You can upload CPU data to this memory via 
             * vkCmdPushConstants. Rendering or dispatch commands issued after this function can read from this memory 
             * through push constant uniform values. No synchronization is needed, as vkCmdPushConstants effectively 
             * executes immediately (within the command buffer)
             * 
             * Note that, push constants are written in ranges. A important reason for that, is that you can have 
             * different push constants, at different ranges, in different stages. For example, you can reserve 64 bytes
             * (1 glm::mat4) size on the vertex shader, and then start the frag shader push constant from offset 64. This
             * way you would have different push constants on different stages
            */
            void createPushConstantRange (uint32_t pipelineInfoId,  
                                          VkShaderStageFlags stageFlags,
                                          uint32_t offset,
                                          uint32_t size) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);

                VkPushConstantRange range{};
                range.stageFlags = stageFlags;
                range.offset     = offset;
                range.size       = size;
                pipelineInfo->resource.pushConstantRanges.push_back (range);
            }
    };

    Log::Record* VKPushConstantRange::m_VKPushConstantRangeLog;
}   // namespace Renderer
#endif  // VK_PUSH_CONSTANT_RANGE_H