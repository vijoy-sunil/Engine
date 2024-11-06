#ifndef EN_UI_H
#define EN_UI_H

#include "../../Core/Model/VKModelMgr.h"
#include "../../Core/RenderPass/VKAttachment.h"
#include "../../Core/RenderPass/VKSubPass.h"
#include "../../Core/RenderPass/VKFrameBuffer.h"
#include "../../Core/Cmd/VKCmd.h"
#include "../../Core/Scene/VKTextureSampler.h"
#include "../../Core/Scene/VKDescriptor.h"
#include "../../Gui/UIImpl.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENUI: protected virtual Core::VKModelMgr,
                protected virtual Core::VKAttachment,
                protected virtual Core::VKSubPass,
                protected virtual Core::VKFrameBuffer,
                protected virtual Core::VKCmd,
                protected virtual Core::VKTextureSampler,
                protected virtual Core::VKDescriptor,
                protected Gui::UIImpl {
        private:
            Log::Record* m_ENUILog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            ENUI (void) {
                m_ENUILog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO, Log::TO_FILE_IMMEDIATE);
            }

            ~ENUI (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void initExtension (uint32_t deviceInfoId,
                                uint32_t uiRenderPassInfoId,
                                uint32_t uiSceneInfoId,
                                uint32_t sceneInfoId) {

                auto deviceInfo  = getDeviceInfo (deviceInfoId);
                auto sceneInfo   = getSceneInfo  (sceneInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY RENDER PASS INFO                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyRenderPassInfo (uiRenderPassInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS ATTACHMENTS                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* The loadOp should be VK_ATTACHMENT_LOAD_OP_LOAD because we want our gui to be drawn over our
                 * main rendering. This tells Vulkan you don’t want to clear the content of the frame buffer but you
                 * want to draw over it instead. Since we’re going to draw some stuff, we also want initialLayout
                 * to be set to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for optimal performance. And because we want
                 * to present this image, finalLayout is set to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR. This will automatically
                 * transition our attachment to the right layout for presentation
                 *
                 * Note that, once a render pass instance has concluded, all attachments cease to be attachments. They're
                 * just regular images from that point forward. The contents of the image are governed by the render
                 * pass's storage operation. But once the storage operation is done, the image has the data produced by
                 * the storage operation. So there is no such thing as an attachment "from a previous render pass". There
                 * is merely an image and its data. How that image got its data is irrelevant to how you're going to use
                 * it now. So if an image has some data, and you use it as an attachment, and you use a load operation
                 * of load, the data in that attachment will have the image data from the image before becoming an
                 * attachment regardless of how the data got there
                */
                createAttachment (sceneInfo->id.swapChainImageInfoBase,
                                  uiRenderPassInfoId,
                                  Core::SWAP_CHAIN_IMAGE,
                                  0,
                                  VK_ATTACHMENT_LOAD_OP_LOAD,                   VK_ATTACHMENT_STORE_OP_STORE,
                                  VK_ATTACHMENT_LOAD_OP_DONT_CARE,              VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS                                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto inputAttachmentRefs   = std::vector <VkAttachmentReference> {
                };
                auto colorAttachmentRefs   = std::vector {
                    getAttachmentReference (0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                };
                auto resolveAttachmentRefs = std::vector <VkAttachmentReference> {
                };
                createSubPass (uiRenderPassInfoId,
                               inputAttachmentRefs,
                               colorAttachmentRefs,
                               nullptr,
                               resolveAttachmentRefs);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG SUB PASS DEPENDENCIES                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* srcSubpass must be VK_SUBPASS_EXTERNAL to create a dependency outside the current render pass. We
                 * can refer to this sub pass in dstSubpass by its index 0. Now we need to state what we’re waiting for.
                 * Before drawing our ui, we want our geometry to be already rendered. That means we want the pixels to
                 * be already written to the frame buffer
                 *
                 * Fortunately, there is a stage called VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT for that and we
                 * can set our srcStageMask to it. We can also set our dstStageMask to this same value because our gui
                 * will also be drawn to the same target. We’re basically waiting for pixels to be written before we can
                 * write pixels ourselves
                 *
                 * As for the access masks, srcAccessMask is set to VK_ACCESS_NONE and dstAccessMask is set to
                 * VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, we are waiting for writes to be done with before we can
                 * write ourselves
                */
                createDependency (uiRenderPassInfoId,
                                  0,
                                  VK_SUBPASS_EXTERNAL, 0,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                  VK_ACCESS_NONE,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG RENDER PASS                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                createRenderPass (deviceInfoId, uiRenderPassInfoId);
                LOG_INFO (m_ENUILog) << "[OK] Render pass "
                                     << "[" << uiRenderPassInfoId << "]"
                                     << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG FRAME BUFFERS                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->params.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, Core::SWAP_CHAIN_IMAGE);

                    auto attachments = std::vector {
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (deviceInfoId, uiRenderPassInfoId, attachments);
                    LOG_INFO (m_ENUILog) << "[OK] Frame buffer "
                                         << "[" << uiRenderPassInfoId << "]"
                                         << " "
                                         << "[" << deviceInfoId << "]"
                                         << std::endl;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG TEXTURE SAMPLER                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                createTextureSampler (deviceInfoId,
                                      uiSceneInfoId,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_TRUE,
                                      VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                      0.0f,
                                      0.0f,
                                      13.0f);
                LOG_INFO (m_ENUILog) << "[OK] Texture sampler "
                                     << "[" << uiSceneInfoId << "]"
                                     << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG DESCRIPTOR POOL                                                                         |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Imgui requires a single combined image sampler descriptor and a descriptor set per font image. In
                 * addition to this, we need a descriptor set per texture for use in imgui. Note that, the pool should
                 * be created with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, and must contain a pool size large
                 * enough to hold the required number of descriptors
                 *
                 * VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT specifies that descriptor sets can return their
                 * individual allocations to the pool. Otherwise, descriptor sets allocated from the pool must not be
                 * individually freed back to the pool
                */
                auto poolSizes = std::vector {
                    getPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 1 + static_cast <uint32_t> (getTextureImagePool().size())),
                };
                createDescriptorPool (deviceInfoId,
                                      uiSceneInfoId,
                                      poolSizes,
                                      1 + static_cast <uint32_t> (getTextureImagePool().size()),
                                      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
                LOG_INFO (m_ENUILog) << "[OK] Descriptor pool "
                                     << "[" << uiSceneInfoId << "]"
                                     << std::endl;
            }

            void drawExtension (uint32_t deviceInfoId,
                                uint32_t uiRenderPassInfoId,
                                uint32_t sceneInfoId,
                                uint32_t currentFrameInFlight,
                                uint32_t swapChainImageId,
                                float frameDelta) {

                auto sceneInfo   = getSceneInfo (sceneInfoId);
                auto clearValues = std::vector {
                    /* Attachment 0
                    */
                    VkClearValue {
                        /* Black with 100% opacity
                        */
                        {{0.0f, 0.0f, 0.0f, 1.0f}}
                    }
                };

                createUIFrame   (frameDelta);
                beginRenderPass (deviceInfoId,
                                 uiRenderPassInfoId,
                                 swapChainImageId,
                                 clearValues,
                                 sceneInfo->resource.commandBuffers[currentFrameInFlight]);
                drawUIFrame     (sceneInfo->resource.commandBuffers[currentFrameInFlight]);
                endRenderPass   (sceneInfo->resource.commandBuffers[currentFrameInFlight]);
            }

            void recreateSwapChainDeps (uint32_t deviceInfoId,
                                        uint32_t uiRenderPassInfoId,
                                        uint32_t sceneInfoId) {

                auto deviceInfo = getDeviceInfo (deviceInfoId);
                auto sceneInfo  = getSceneInfo  (sceneInfoId);

                vkDeviceWaitIdle (deviceInfo->resource.logDevice);
                /* |------------------------------------------------------------------------------------------------|
                 * | DESTROY FRAME BUFFERS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                VKFrameBuffer::cleanUp (deviceInfoId, uiRenderPassInfoId);
                LOG_INFO (m_ENUILog) << "[DELETE] Frame buffers "
                                     << "[" << uiRenderPassInfoId << "]"
                                     << " "
                                     << "[" << deviceInfoId << "]"
                                     << std::endl;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG FRAME BUFFERS                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (uint32_t i = 0; i < deviceInfo->params.swapChainSize; i++) {
                    uint32_t swapChainImageInfoId = sceneInfo->id.swapChainImageInfoBase + i;
                    auto swapChainImageInfo       = getImageInfo (swapChainImageInfoId, Core::SWAP_CHAIN_IMAGE);

                    auto attachments = std::vector {
                        swapChainImageInfo->resource.imageView
                    };
                    createFrameBuffer (deviceInfoId, uiRenderPassInfoId, attachments);
                    LOG_INFO (m_ENUILog) << "[OK] Frame buffer "
                                         << "[" << uiRenderPassInfoId << "]"
                                         << " "
                                         << "[" << deviceInfoId << "]"
                                         << std::endl;
                }
                /* Note that, upon recreating the swap chain, the minimum amount of swap chain image views might have
                 * changed. We need to notify imgui about this change
                */
                ImGui_ImplVulkan_SetMinImageCount (deviceInfo->params.minSwapChainImageCount);
            }
    };
}   // namespace SandBox
#endif  // EN_UI_H