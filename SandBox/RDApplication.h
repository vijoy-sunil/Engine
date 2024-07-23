#ifndef RD_APPLICATION_H
#define RD_APPLICATION_H

#include "../Core/Scene/VKInitSequence.h"
#include "../Core/Scene/VKDrawSequence.h"
#include "../Core/Scene/VKDeleteSequence.h"

namespace Renderer {
    class RDApplication: protected VKInitSequence,
                         protected virtual VKDrawSequence,
                         protected VKDeleteSequence {
        private:
            uint32_t m_deviceResourcesCount;

            uint32_t m_modelInfoId;
            uint32_t m_vertexBufferInfoId;
            uint32_t m_indexBufferInfoId;
            uint32_t m_uniformBufferInfoIdBase;
            uint32_t m_swapChainImageInfoIdBase;
            uint32_t m_diffuseTextureImageInfoIdBase;
            uint32_t m_depthImageInfoId;
            uint32_t m_multiSampleImageInfoId;

            uint32_t m_renderPassInfoId;
            uint32_t m_pipelineInfoId;
            uint32_t m_cameraInfoId;
            uint32_t m_resourceId;
            uint32_t m_handOffInfoId;

            bool m_refreshModelTransform;
            bool m_refreshCameraTransform;

        public:
            RDApplication (void) {
                m_deviceResourcesCount            = 1;
                m_modelInfoId                     = 0;
                /* Info id overview
                 * |--------------------|--------------------|
                 * | MODEL INFO ID      |   0                |
                 * |--------------------|--------------------|
                 * | STAGING_BUFFER     |   0, 1,    2, 3, 4 |
                 * |--------------------|   ^  ^     ^  ^  ^ |
                 * | VERTEX_BUFFER      |   0  :     :  :  : |
                 * |--------------------|      :     :  :  : |
                 * | INDEX_BUFFER       |   1 >:     :  :  : |
                 * |--------------------|            :  :  : |
                 * | UNIFORM_BUFFER     |   2, 3     :  :  : |
                 * |--------------------|            :  :  : |
                 *                                   :  :  : |
                 * |--------------------|            :  :  : |
                 * | SWAPCHAIN_IMAGE    |   0, 1, 2  :  :  : |
                 * |--------------------|            :  :  : |
                 * | TEXTURE_IMAGE      |   2, 3, 4 >: >: >: |
                 * |--------------------|                    |
                 * | DEPTH_IMAGE        |   0                |
                 * |--------------------|                    |
                 * | MULTISAMPLE_IMAGE  |   0                |
                 * |--------------------|--------------------|
                */
                m_vertexBufferInfoId            = 0;
                m_indexBufferInfoId             = 1;
                m_uniformBufferInfoIdBase       = 2;
                m_swapChainImageInfoIdBase      = 0;
                m_diffuseTextureImageInfoIdBase = 2;
                m_depthImageInfoId              = 0;
                m_multiSampleImageInfoId        = 0;

                m_renderPassInfoId         = 0;
                m_pipelineInfoId           = 0;
                m_cameraInfoId             = 0;
                m_resourceId               = 0;
                m_handOffInfoId            = 0;

                m_refreshModelTransform    = false;
                m_refreshCameraTransform   = false;
            }

            ~RDApplication (void) {
            }

            void createScene (void) {
                setDeviceResourceCount (m_deviceResourcesCount);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY MODEL INFO                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto infoIds = std::vector {
                                                m_vertexBufferInfoId, 
                                                m_indexBufferInfoId, 
                                                m_uniformBufferInfoIdBase,
                                                m_swapChainImageInfoIdBase,
                                                m_diffuseTextureImageInfoIdBase, 
                                                m_depthImageInfoId, 
                                                m_multiSampleImageInfoId
                                           };
                readyModelInfo (m_modelInfoId,
                                g_pathSettings.models[3],
                                g_pathSettings.mtlFileDir,
                                g_pathSettings.vertexShaderBinary,
                                g_pathSettings.fragmentShaderBinary,
                                infoIds);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY HAND OFF INFO                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyHandOffInfo (m_handOffInfoId);
                auto handOffInfo = getHandOffInfo (m_handOffInfoId);

                TransformInfo transformInfo{};
                transformInfo.model.translate      = {0.0f,  0.0f,  0.0f};
                transformInfo.model.rotateAxis     = {0.0f,  1.0f,  0.0f};
                transformInfo.model.scale          = {1.0f,  1.0f,  1.0f};
                transformInfo.model.rotateAngleDeg = 0.0f; 

                transformInfo.camera.position  = {0.0f,  0.0f, -4.0f};
                transformInfo.camera.center    = {0.0f,  0.0f,  0.0f};
                transformInfo.camera.upVector  = {0.0f, -1.0f,  0.0f};
                transformInfo.camera.fovDeg    = 45.0f;
                transformInfo.camera.nearPlane = 0.1f;
                transformInfo.camera.farPlane  = 10.0f;

                FragShaderVarsPC fragShaderVars{};
                fragShaderVars.texId = 0;

                handOffInfo->meta.transformInfo  = transformInfo;
                handOffInfo->meta.fragShaderVars = fragShaderVars;

                VKInitSequence::runSequence (m_modelInfoId, 
                                             m_renderPassInfoId,
                                             m_pipelineInfoId,
                                             m_cameraInfoId,
                                             m_resourceId,
                                             m_handOffInfoId);
            }

            void runScene (void) {
                auto deviceInfo  = getDeviceInfo();

#if ENABLE_IDLE_ROTATION || ENABLE_CYCLE_TEXTURES
                auto handOffInfo = getHandOffInfo (m_handOffInfoId);
#endif  // ENABLE_IDLE_ROTATION || ENABLE_CYCLE_TEXTURES

#if ENABLE_CYCLE_TEXTURES
                auto modelInfo   = getModelInfo   (m_modelInfoId);
                /* Array of textures that will be used to cycle through using push push constant
                 *                                          V               V               V               V      
                 * |----------------|---------------|---------------|---------------|---------------|---------------|
                 * |    default     |   model       |   tex 0       |   tex 1       |   tex 2       |   tex 3       |
                 * |    texture     |   textures    |               |               |               |               |
                 * |----------------|---------------|---------------|---------------|---------------|---------------|
                 *                                          ^
                 *                                          offset
                */
                uint32_t cycleTexturesOffset = static_cast <uint32_t> (modelInfo->path.diffuseTextureImages.size() - 
                                                                       g_pathSettings.cycleTextures.size());
                uint32_t framesUntilNextDefaultTexture = g_framesPerCycleTexture;
                handOffInfo->meta.fragShaderVars.texId = cycleTexturesOffset;
#endif  // ENABLE_CYCLE_TEXTURES

                /* |------------------------------------------------------------------------------------------------|
                 * | EVENT LOOP                                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                while (!glfwWindowShouldClose (deviceInfo->unique[m_resourceId].window)) {
                    glfwPollEvents();

#if ENABLE_IDLE_ROTATION
                    /* Calculate the time in seconds since rendering has started with floating point accuracy
                    */
                    static auto startTime = std::chrono::high_resolution_clock::now();
                    auto currentTime      = std::chrono::high_resolution_clock::now();
                    float time            = std::chrono::duration <float, std::chrono::seconds::period> 
                                            (currentTime - startTime).count();  
                    
                    handOffInfo->meta.transformInfo.model.rotateAngleDeg  = time * 20.0f;

                    m_refreshModelTransform  = true;
                    m_refreshCameraTransform = false;
#endif  // ENABLE_IDLE_ROTATION

                    VKDrawSequence::runSequence (m_modelInfoId, 
                                                 m_renderPassInfoId,
                                                 m_pipelineInfoId,
                                                 m_cameraInfoId,
                                                 m_resourceId,
                                                 m_handOffInfoId,
                                                 m_refreshModelTransform, m_refreshCameraTransform);
                    /* Reset flags
                    */
                    m_refreshModelTransform  = false;
                    m_refreshCameraTransform = false;       

#if ENABLE_CYCLE_TEXTURES
                    framesUntilNextDefaultTexture--;
                    if (framesUntilNextDefaultTexture == 0) {
                        framesUntilNextDefaultTexture    = g_framesPerCycleTexture;
                        FragShaderVarsPC fragShaderVars  = handOffInfo->meta.fragShaderVars;
                        fragShaderVars.texId             = (fragShaderVars.texId + 1) % static_cast <uint32_t> 
                                                           (modelInfo->path.diffuseTextureImages.size());
                        if (fragShaderVars.texId == 0)
                            fragShaderVars.texId         = cycleTexturesOffset;
                        handOffInfo->meta.fragShaderVars = fragShaderVars;
                    }
#endif  // ENABLE_CYCLE_TEXTURES             
                }
                /* Remember that all of the operations in the above render method are asynchronous. That means that when
                 * we exit the render loop, drawing and presentation operations may still be going on. Cleaning up
                 * resources while that is happening is a bad idea. To fix that problem, we should wait for the logical 
                 * device to finish operations before exiting mainLoop and destroying the window
                */
                vkDeviceWaitIdle (deviceInfo->shared.logDevice);
            }

            void deleteScene (void) {
                VKDeleteSequence::runSequence (m_modelInfoId, 
                                               m_renderPassInfoId,
                                               m_pipelineInfoId,
                                               m_cameraInfoId,
                                               m_resourceId,
                                               m_handOffInfoId);
            }
    };
}   // namespace Renderer
#endif  // RD_APPLICATION_H