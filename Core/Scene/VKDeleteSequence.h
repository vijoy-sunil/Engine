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
            Log::Record* m_VKDeleteSequenceLog;
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
            void runSequence (const std::vector <uint32_t>& modelInfoIds, 
                              uint32_t renderPassInfoId, 
                              uint32_t pipelineInfoId,
                              uint32_t cameraInfoId,
                              uint32_t sceneInfoId,
                              uint32_t deviceInfoId) {

                auto modelInfoBase = getModelInfo  (*modelInfoIds.begin());
                auto sceneInfo     = getSceneInfo  (sceneInfoId);
                auto deviceInfo    = getDeviceInfo (deviceInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - FENCE AND SEMAPHORES                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t renderDoneSemaphoreInfoId = sceneInfo->id.renderDoneSemaphoreInfoBase + i; 
                    cleanUpSemaphore (renderDoneSemaphoreInfoId, deviceInfoId, SEM_RENDER_DONE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << renderDoneSemaphoreInfoId << "]"
                                                     << std::endl;
                }

                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t imageAvailableSemaphoreInfoId = sceneInfo->id.imageAvailableSemaphoreInfoBase + i;
                    cleanUpSemaphore (imageAvailableSemaphoreInfoId, deviceInfoId, SEM_IMAGE_AVAILABLE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops semaphore " 
                                                     << "[" << imageAvailableSemaphoreInfoId << "]"
                                                     << std::endl;
                }

                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t inFlightFenceInfoId = sceneInfo->id.inFlightFenceInfoBase + i;
                    cleanUpFence (inFlightFenceInfoId, deviceInfoId, FEN_IN_FLIGHT);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops fence " 
                                                     << "[" << inFlightFenceInfoId << "]"
                                                     << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DRAW OPS - COMMAND POOL                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKCmdBuffer::cleanUp (deviceInfoId, sceneInfo->resource.commandPool);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Draw ops command pool "
                                                 << "[" << sceneInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DESCRIPTOR POOL                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDescriptor::cleanUp (sceneInfoId, deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Descriptor pool " 
                                                 << "[" << sceneInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE SAMPLER                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKTextureSampler::cleanUp (sceneInfoId, deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture sampler " 
                                                 << "[" << sceneInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY PIPELINE                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */                                                 
                VKPipelineMgr::cleanUp (pipelineInfoId, deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Pipeline " 
                                                 << "[" << pipelineInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY FRAME BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKFrameBuffer::cleanUp (renderPassInfoId, deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Frame buffers " 
                                                 << "[" << renderPassInfoId << "]"
                                                 << " "
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY RENDER PASS                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKRenderPassMgr::cleanUp (renderPassInfoId, deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Render pass " 
                                                 << "[" << renderPassInfoId << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY STORAGE BUFFERS                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < g_maxFramesInFlight; i++) {
                    uint32_t storageBufferInfoId = sceneInfo->id.storageBufferInfoBase + i; 
                    VKBufferMgr::cleanUp (storageBufferInfoId, deviceInfoId, STORAGE_BUFFER);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Storage buffer " 
                                                     << "[" << storageBufferInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY INDEX BUFFER                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that, we are only deleting the index buffer belonging to base id model, since the rest of the
                 * statically loaded models' index buffers are owned by the base id model
                */
                VKBufferMgr::cleanUp (modelInfoBase->id.indexBufferInfo, deviceInfoId, INDEX_BUFFER);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Index buffer " 
                                                 << "[" << modelInfoBase->id.indexBufferInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY VERTEX BUFFERS                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& infoId: modelInfoBase->id.vertexBufferInfos) {
                    VKBufferMgr::cleanUp (infoId, deviceInfoId, VERTEX_BUFFER);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Vertex buffer " 
                                                     << "[" << infoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY MULTI SAMPLE RESOURCES                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (sceneInfo->id.multiSampleImageInfo, deviceInfoId, MULTISAMPLE_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Multi sample resources " 
                                                 << "[" << sceneInfo->id.multiSampleImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEPTH RESOURCES                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKImageMgr::cleanUp (sceneInfo->id.depthImageInfo, deviceInfoId, DEPTH_IMAGE);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Depth resources " 
                                                 << "[" << sceneInfo->id.depthImageInfo << "]"
                                                 << std::endl; 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY TEXTURE RESOURCES - DIFFUSE TEXTURE                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    VKImageMgr::cleanUp (infoId, deviceInfoId, TEXTURE_IMAGE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Texture resources " 
                                                     << "[" << infoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN RESOURCES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->meta.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    VKImageMgr::cleanUp (swapChainImageInfoId, deviceInfoId, SWAPCHAIN_IMAGE);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Swap chain resources " 
                                                     << "[" << swapChainImageInfoId << "]"
                                                     << " "
                                                     << "[" << deviceInfoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SWAP CHAIN                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDeviceMgr::cleanUpSwapChain (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Swap chain "
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY LOG DEVICE                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKLogDevice::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Log device " 
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl;                
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY SURFACE                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKSurface::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Surface " 
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl;   
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEBUG MESSENGER                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKValidation::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Debug messenger "
                                                 << "[" << deviceInfoId << "]" 
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY INSTANCE                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKInstance::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Instance "
                                                 << "[" << deviceInfoId << "]" 
                                                 << std::endl;                 
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY WINDOW                                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKWindow::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Window " 
                                                 << "[" << deviceInfoId << "]"
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
                for (auto const& infoId: modelInfoIds) {
                    VKModelMgr::cleanUp (infoId);
                    LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Model info " 
                                                     << "[" << infoId << "]"
                                                     << std::endl; 
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY DEVICE INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKDeviceMgr::cleanUp (deviceInfoId);
                LOG_INFO (m_VKDeleteSequenceLog) << "[DELETE] Device info " 
                                                 << "[" << deviceInfoId << "]"
                                                 << std::endl; 
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
}   // namespace Core
#endif  // VK_DELETE_SEQUENCE_H