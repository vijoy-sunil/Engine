#ifndef VK_INPUT_ASSEMBLY_H
#define VK_INPUT_ASSEMBLY_H

#include "VKPipelineMgr.h"

namespace Core {
    class VKInputAssembly: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKInputAssemblyLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;
            
        public:
            VKInputAssembly (void) {
                m_VKInputAssemblyLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKInputAssembly (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createInputAssemblyState (uint32_t pipelineInfoId, 
                                           VkPrimitiveTopology topology,
                                           VkBool32 restartEnable) {
                
                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                /* The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be 
                 * drawn from the vertices and if primitive restart should be enabled
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_POINT_LIST
                 * points from vertices
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_LINE_LIST
                 * line from every 2 vertices without reuse
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_LINE_STRIP
                 * the end vertex of every line is used as start vertex for the next line
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                 * triangle from every 3 vertices without reuse
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                 * the second and third vertex of every triangle are used as first two vertices of the next triangle
                */
                VkPipelineInputAssemblyStateCreateInfo createInfo;
                createInfo.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                createInfo.pNext    = VK_NULL_HANDLE;
                createInfo.flags    = 0;
                createInfo.topology = topology;
                /* If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and 
                 * triangles in the _STRIP topology modes
                */
                createInfo.primitiveRestartEnable = restartEnable;
                pipelineInfo->state.inputAssembly = createInfo;
            } 
    };
}   // namespace Core
#endif  // VK_INPUT_ASSEMBLY_H