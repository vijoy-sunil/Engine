#ifndef VK_DELETE_SEQUENCE_H
#define VK_DELETE_SEQUENCE_H

#include "../Device/VKWindow.h"
#include "../Device/VKInstance.h"
#include "../Device/VKSurface.h"
#include "../Device/VKLogDevice.h"
#include "../Model/VKModelMgr.h"
#include "../Image/VKImageMgr.h"
#include "../Buffer/VKBufferMgr.h"
#include "../RenderPass/VKFrameBuffer.h"
#include "../Cmd/VKCmdBuffer.h"
#include "VKCameraMgr.h"
#include "VKTextureSampler.h"
#include "VKDescriptor.h"
#include "VKSyncObject.h"

using namespace Collections;

namespace Core {
    class VKDeleteSequence: protected virtual VKWindow,
                            protected virtual VKInstance,
                            protected virtual VKSurface,
                            protected virtual VKLogDevice,
                            protected virtual VKModelMgr,
                            protected virtual VKImageMgr,
                            protected virtual VKBufferMgr,
                            protected virtual VKFrameBuffer,
                            protected virtual VKCmdBuffer,
                            protected virtual VKCameraMgr,
                            protected virtual VKTextureSampler,
                            protected virtual VKDescriptor,
                            protected virtual VKSyncObject {
        private:
            static Log::Record* m_VKDeleteSequenceLog;
            const uint32_t m_instanceId = g_collectionsId++;

        public:
            VKDeleteSequence (void) {
                m_VKDeleteSequenceLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~VKDeleteSequence (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void runSequence (uint32_t modelInfoIdBase, 
                              uint32_t renderPassInfoId, 
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId,
                              uint32_t resourceId,
                              uint32_t sceneInfoId) {

                auto modelInfoBase = getModelInfo (modelInfoIdBase);
                auto sceneInfo     = getSceneInfo (sceneInfoId);
                auto deviceInfo    = getDeviceInfo();
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - FENCE AND SEMAPHORES                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t renderDoneSemaphoreInfoId = sceneInfo->id.renderDoneSemaphoreInfoBase + i; 
                    cleanUpSemaphore (renderDoneSemaphoreInfoId, SEM_RENDER_DONE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << renderDoneSemaphoreInfoId << "]"
                                                     << std::endl;
                }

                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t imageAvailableSemaphoreInfoId = sceneInfo->id.imageAvailableSemaphoreInfoBase + i;
                    cleanUpSemaphore (imageAvailableSemaphoreInfoId, SEM_IMAGE_AVAILABLE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << imageAvailableSemaphoreInfoId << "]"
                                                     << std::endl;
                }

                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t inFlightFenceInfoId = sceneInfo->id.inFlightFenceInfoBase + i;
                    cleanUpFence (inFlightFenceInfoId, FEN_IN_FLIGHT);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops fence " 
                                                     << "[" << inFlightFenceInfoId << "]"
                                                     << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - COMMAND POOL                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (sceneInfo->resource.commandPool);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops command pool"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DESCRIPTOR POOL                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDescriptor::cleanUp (sceneInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Descriptor pool " 
                                                 << "[" << sceneInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE SAMPLER                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKTextureSampler::cleanUp (sceneInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture sampler " 
                                                 << "[" << sceneInfoId << "]"
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
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t uniformBufferInfoId = sceneInfo->id.uniformBufferInfoBase + i; 
                    VKBufferMgr::cleanUp (uniformBufferInfoId, UNIFORM_BUFFER);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Uniform buffer " 
                                                     << "[" << uniformBufferInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY INDEX BUFFER                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that, we are only deleting the index buffer belonging to base id model, since the rest of the
                 * statically loaded models' index buffers are owned by the base id model
                */
                VKBufferMgr::cleanUp (modelInfoBase->id.indexBufferInfo, INDEX_BUFFER);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Index buffer " 
                                                 << "[" << modelInfoBase->id.indexBufferInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY VERTEX BUFFER                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKBufferMgr::cleanUp (modelInfoBase->id.vertexBufferInfo, VERTEX_BUFFER);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Vertex buffer " 
                                                 << "[" << modelInfoBase->id.vertexBufferInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MULTI SAMPLE RESOURCES                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (sceneInfo->id.multiSampleImageInfo, MULTISAMPLE_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Multi sample resources " 
                                                 << "[" << sceneInfo->id.multiSampleImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEPTH RESOURCES                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (sceneInfo->id.depthImageInfo, DEPTH_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Depth resources " 
                                                 << "[" << sceneInfo->id.depthImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE RESOURCES - DIFFUSE TEXTURE                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    VKImageMgr::cleanUp (infoId, TEXTURE_IMAGE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture resources " 
                                                     << "[" << infoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN RESOURCES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->unique[resourceId].swapChain.size; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
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
                 * | DESTROY SCENE INFO                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKSceneMgr::cleanUp (sceneInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Scene info " 
                                                 << "[" << sceneInfoId << "]"
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
                for (size_t i = 0; i < g_pathSettings.models.size(); i++) {
                    uint32_t modelInfoId = modelInfoIdBase + static_cast <uint32_t> (i);
                    
                    VKModelMgr::cleanUp (modelInfoId);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Model info " 
                                                     << "[" << modelInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpModelInfoPool();
                dumpImageInfoPool();
                dumpBufferInfoPool();   
                dumpRenderPassInfoPool();  
                dumpPipelineInfoPool();
                dumpCameraInfoPool();
                dumpFenceInfoPool();
                dumpSemaphoreInfoPool();
                dumpSceneInfoPool();
                dumpDeviceInfoPool();
            }
    };

    Log::Record* VKDeleteSequence::m_VKDeleteSequenceLog;
}   // namespace Core
#endif  // VK_DELETE_SEQUENCE_H