#ifndef UI_TEXTURE_H
#define UI_TEXTURE_H

#include <imgui_impl_vulkan.h>
#include "../../Core/Image/VKImageMgr.h"
#include "../../Core/Scene/VKSceneMgr.h"
#include "../Wrapper/UIPrimitive.h"
#include "../Wrapper/UITree.h"

namespace Gui {
    class UITexture: protected virtual Core::VKImageMgr,
                     protected virtual Core::VKSceneMgr,
                     protected virtual UIPrimitive,
                     protected virtual UITree {
        private:
            struct UITextureInfo {
                struct Meta {
                    Core::e_textureType type;
                    uint32_t selectedLayerIdx;
                    std::string label;
                    std::vector <std::string> fileNames;
                    std::vector <std::string> layerIdxLabels;
                } meta;

                struct Resource {
                    std::vector <VkDescriptorSet> descriptorSets;
                } resource;
            };
            std::unordered_map <uint32_t, UITextureInfo> m_uiTextureInfoPool;

            uint32_t m_selectedTextureImageInfoIdLabelIdx;
            std::vector <std::string> m_textureImageInfoIdLabels;

            Log::Record* m_UITextureLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UITexture (void) {
                m_UITextureLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UITexture (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyContent (uint32_t sceneInfoId,
                               const std::unordered_map <Core::e_textureType,
                                     std::unordered_map <uint32_t, std::vector <std::string>>>& textureImagePools) {

                m_selectedTextureImageInfoIdLabelIdx = 0;
                auto sceneInfo = getSceneInfo (sceneInfoId);

                for (auto const& [type, pool]: textureImagePools) {
                    for (auto const& [infoId, paths]: pool) {

                        auto imageInfo      = getImageInfo (infoId, Core::TEXTURE_IMAGE);
                        uint32_t layerCount = imageInfo->meta.layerCount;
                        std::string label   = "Info id [" + std::to_string (infoId) + "]";

                        for (uint32_t layerIdx = 0; layerIdx < layerCount; layerIdx++) {
                            /* Strip path to just the file name
                            */
                            size_t stripStart    = paths[layerIdx].find_last_of ("\\/") + 1;
                            std::string fileName = paths[layerIdx].substr (stripStart, paths[layerIdx].length()
                                                                         - stripStart);

                            auto imageView       = layerCount == 1 ? imageInfo->resource.imageView:
                                                                     imageInfo->resource.aliasImageViews[layerIdx];
                            auto descriptorSet   = ImGui_ImplVulkan_AddTexture (sceneInfo->resource.textureSampler,
                                                                                imageView,
                                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                            m_uiTextureInfoPool[infoId].meta.type             = type;
                            m_uiTextureInfoPool[infoId].meta.selectedLayerIdx = 0;
                            m_uiTextureInfoPool[infoId].meta.label            = label;
                            m_uiTextureInfoPool[infoId].meta.fileNames.         push_back (fileName);
                            m_uiTextureInfoPool[infoId].meta.layerIdxLabels.    push_back (std::to_string (layerIdx));
                            m_uiTextureInfoPool[infoId].resource.descriptorSets.push_back (descriptorSet);
                        }
                    }
                }
                /* Note that, the ordering of data in the map must correspond to the ones in the vector. Hence, why we
                 * populate the vector after the map is completely populated
                */
                for (auto const& [key, val]: m_uiTextureInfoPool)
                    m_textureImageInfoIdLabels.push_back (val.meta.label);
            }

            void createContent (uint32_t nodeInfoId) {
                auto nodeInfo = getNodeInfo (nodeInfoId);

                /* Default data */
                bool fieldDisable = false;

                if ((nodeInfo->meta.type & MODEL_NODE) && (nodeInfo->meta.type & TEXTURE_NODE)) {
                    /* Read data */
                    uint32_t infoId                      = nodeInfo->meta.coreInfoId;
                    /* Convert texture image info id to label, and we use the label to find the offset to the labels
                     * vector. This provides us a common index to access both the map and the vector
                    */
                    std::string label                    = "Info id [" + std::to_string (infoId) + "]";
                    m_selectedTextureImageInfoIdLabelIdx = std::find (m_textureImageInfoIdLabels.begin(),
                                                                      m_textureImageInfoIdLabels.end(), label) -
                                                                      m_textureImageInfoIdLabels.begin();
                    fieldDisable                         = true;
                }

                /* Show data */
                createCombo ("##textureId",
                             "Id",
                             "##postLabelTextureId",
                             m_textureImageInfoIdLabels,
                             fieldDisable,
                             g_styleSettings.size.inputFieldLarge,
                             m_selectedTextureImageInfoIdLabelIdx);

                /* Note that, we have already ensured that there is 1:1 correspondence between the map and the vector.
                 * This makes it possible to offset into the map using an index to the vector
                */
                auto iter = std::next (m_uiTextureInfoPool.begin(), m_selectedTextureImageInfoIdLabelIdx);

                createCombo ("##layer",
                             "Layer",
                             "##postLabelLayer",
                             iter->second.meta.layerIdxLabels,
                             false,
                             g_styleSettings.size.inputFieldLarge,
                             iter->second.meta.selectedLayerIdx);

                createSeparatorText ("Preview");
                createImagePreview  (iter->second.resource.descriptorSets[iter->second.meta.selectedLayerIdx],
                                     g_styleSettings.size.image,
                                     g_styleSettings.color.border);

                auto imageInfo       = getImageInfo (iter->first, Core::TEXTURE_IMAGE);
                const char* type     = Core::getTextureTypeString (iter->second.meta.type);
                std::string dims     = std::to_string (imageInfo->meta.width)  + "px"
                                       + " x " +
                                       std::to_string (imageInfo->meta.height) + "px";
                std::string fileName = iter->second.meta.fileNames[iter->second.meta.selectedLayerIdx];

                createSeparatorText ("More");
                ImGui::Text         ("%s", type);
                ImGui::Text         ("%s", dims.c_str());
                ImGui::Text         ("%s", fileName.c_str());
            }

            void cleanUp (void) {
                for (auto const& [key, val]: m_uiTextureInfoPool) {
                    for (auto const& descriptorSet: val.resource.descriptorSets)
                        ImGui_ImplVulkan_RemoveTexture (descriptorSet);
                }
                m_uiTextureInfoPool.clear();
            }
    };
}   // namespace Gui
#endif  // UI_TEXTURE_H