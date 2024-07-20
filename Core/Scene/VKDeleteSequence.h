#ifndef VK_DELETE_SEQUENCE_H
#define VK_DELETE_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Device/VKInstance.h"
#include "../Device/VKSurface.h"
#include "../Device/VKLogDevice.h"
#include "../Image/VKImageMgr.h"
#include "../Buffer/VKBufferMgr.h"
#include "../RenderPass/VKFrameBuffer.h"
#include "../Model/VKTextureSampler.h"
#include "../Model/VKDescriptor.h"
#include "../Cmds/VKCmdBuffer.h"
#include "VKCameraMgr.h"
#include "VKSyncObjects.h"
#include "VKDrawSequence.h"

using namespace Collections;

namespace Renderer {
    class VKDeleteSequence: protected virtual VKWindow,
                            protected virtual VKInstance,
                            protected virtual VKSurface,
                            protected virtual VKLogDevice,
                            protected virtual VKImageMgr,
                            protected virtual VKBufferMgr,
                            protected virtual VKFrameBuffer,
                            protected virtual VKTextureSampler,
                            protected virtual VKDescriptor,
                            protected virtual VKCmdBuffer,
                            protected virtual VKCameraMgr,
                            protected virtual VKSyncObjects,
                            protected virtual VKDrawSequence {
        private:
            static Log::Record* m_VKDeleteSequenceLog;
            const size_t m_instanceId = g_collectionsId++;

        public:
            VKDeleteSequence (void) {
                m_VKDeleteSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
            }

            ~VKDeleteSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void runSequence (uint32_t modelInfoId, 
                              uint32_t renderPassInfoId, 
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId,
                              uint32_t resourceId,
                              uint32_t handOffInfoId) {

                auto modelInfo  = getModelInfo (modelInfoId);
                auto deviceInfo = getDeviceInfo();
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - FENCE AND SEMAPHORES                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto handOffInfo = getHandOffInfo (handOffInfoId);
                for (auto const& infoId: handOffInfo->id.renderDoneSemaphoreInfos) {
                    cleanUpSemaphore (infoId, SEM_RENDER_DONE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << infoId << "]"
                                                     << std::endl;
                }

                for (auto const& infoId: handOffInfo->id.imageAvailableSemaphoreInfos) {
                    cleanUpSemaphore (infoId, SEM_IMAGE_AVAILABLE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << infoId << "]"
                                                     << std::endl;
                }

                for (auto const& infoId: handOffInfo->id.inFlightFenceInfos) {
                    cleanUpFence (infoId, FEN_IN_FLIGHT);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops fence " 
                                                     << "[" << infoId << "]"
                                                     << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - COMMAND POOL                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (handOffInfo->resource.commandPool);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops command pool"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DESCRIPTOR POOL                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDescriptor::cleanUp (modelInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Descriptor pool " 
                                                 << "[" << modelInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE SAMPLER                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKTextureSampler::cleanUp (modelInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture sampler " 
                                                 << "[" << modelInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY PIPELINE                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */                                                 
                VKPipelineMgr::cleanUp (pipelineInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Pipeline " 
                                                 << "[" << pipelineInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY FRAME BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKFrameBuffer::cleanUp (renderPassInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Frame buffers " 
                                                 << "[" << renderPassInfoId << "]"
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY RENDER PASS                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKRenderPassMgr::cleanUp (renderPassInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Render pass " 
                                                 << "[" << renderPassInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY UNIFORM BUFFERS                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (size_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t uniformBufferInfoId = modelInfo->id.uniformBufferInfoBase + static_cast <uint32_t> (i); 
                    VKBufferMgr::cleanUp (uniformBufferInfoId, UNIFORM_BUFFER);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Uniform buffer " 
                                                     << "[" << uniformBufferInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY INDEX BUFFER                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (modelInfo->id.indexBufferInfo, INDEX_BUFFER);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Index buffer " 
                                                 << "[" << modelInfo->id.indexBufferInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY VERTEX BUFFER                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (modelInfo->id.vertexBufferInfo, VERTEX_BUFFER);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Vertex buffer " 
                                                 << "[" << modelInfo->id.vertexBufferInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MULTI SAMPLE RESOURCES                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (modelInfo->id.multiSampleImageInfo, MULTISAMPLE_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Multi sample resources " 
                                                 << "[" << modelInfo->id.multiSampleImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEPTH RESOURCES                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (modelInfo->id.depthImageInfo, DEPTH_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Depth resources " 
                                                 << "[" << modelInfo->id.depthImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE RESOURCES - DIFFUSE TEXTURE                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (size_t i = 0; i < modelInfo->path.diffuseTextureImages.size(); i++) {
                    uint32_t textureImageInfoId = modelInfo->id.diffuseTextureImageInfoBase + static_cast <uint32_t> (i);
                    VKImageMgr::cleanUp (textureImageInfoId, TEXTURE_IMAGE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture resources " 
                                                     << "[" << textureImageInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN RESOURCES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->unique[resourceId].swapChain.size; i++) {
                    uint32_t swapChainImageInfoId = modelInfo->id.swapChainImageInfoBase + i;
                    VKImageMgr::cleanUp (swapChainImageInfoId, SWAPCHAIN_IMAGE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Swap chain resources " 
                                                     << "[" << swapChainImageInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDeviceMgr::cleanUp (resourceId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Swap chain "
                                                 << "[" << resourceId << "]"
                                                 << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY LOG DEVICE                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKLogDevice::cleanUp();
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Log device" 
                                                 << std::endl;                
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SURFACE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKSurface::cleanUp (resourceId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Surface " 
                                                 << "[" << resourceId << "]"
                                                 << std::endl;   
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEBUG MESSENGER                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKValidation::cleanUp();
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Debug messenger" 
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY INSTANCE                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKInstance::cleanUp();
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Instance" 
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY WINDOW                                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKWindow::cleanUp (resourceId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Window " 
                                                 << "[" << resourceId << "]"
                                                 << std::endl;   
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY HAND OFF INFO                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDrawSequence::cleanUp (handOffInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Hand off info " 
                                                 << "[" << handOffInfoId << "]"
                                                 << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY CAMERA INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCameraMgr::cleanUp (cameraInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Camera info " 
                                                 << "[" << cameraInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MODEL INFO                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKModelMgr::cleanUp (modelInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Model info " 
                                                 << "[" << modelInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpImageInfoPool();
                dumpBufferInfoPool();   
                dumpRenderPassInfoPool();  
                dumpPipelineInfoPool();
                dumpModelInfoPool();
                dumpCameraInfoPool();
                dumpFenceInfoPool();
                dumpSemaphoreInfoPool();
                dumpHandOffInfoPool();
            }
    };

    Log::Record* VKDeleteSequence::m_VKDeleteSequenceLog;
}   // namespace Renderer
#endif  // VK_DELETE_SEQUENCE_H