#ifndef UI_WORLD_COLLECTION_H
#define UI_WORLD_COLLECTION_H

#include <IconFontCppHeaders/IconsFontAwesome6.h>
#include "../../Core/Model/VKModelMgr.h"
#include "../../Core/Scene/VKLightMgr.h"
#include "../Wrapper/UITree.h"
#include "../../SandBox/ENLogHelper.h"

namespace Gui {
    inline e_nodeType operator | (e_nodeType typeA, e_nodeType typeB) {
        return static_cast <e_nodeType> (static_cast <int> (typeA) | static_cast <int> (typeB));
    }

    class UIWorldCollection: protected virtual Core::VKModelMgr,
                             protected virtual Core::VKLightMgr,
                             protected virtual UITree {
        private:
            uint32_t m_currentNodeInfoId;
            ImGuiTreeNodeFlags m_treeNodeFlags;

            Log::Record* m_UIWorldCollectionLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void readyTreeLevel (std::vector <uint32_t>& currentLevelNodeInfoIds,
                                 const std::string& label,
                                 e_nodeType type,
                                 const std::vector <uint32_t>& childNodeInfoIds,
                                 uint32_t coreInfoId) {

                currentLevelNodeInfoIds.push_back (m_currentNodeInfoId);
                readyNodeInfo   (m_currentNodeInfoId,
                                 label,
                                 type,
                                 childNodeInfoIds,
                                 coreInfoId,
                                 childNodeInfoIds.empty(),
                                 m_treeNodeFlags);

                /* Update parent node info id for all children
                 */
                for (auto const& infoId: childNodeInfoIds) {
                    auto nodeInfo               = getNodeInfo (infoId);
                    nodeInfo->meta.parentInfoId = m_currentNodeInfoId;
                }
                m_currentNodeInfoId++;
            }

        public:
            UIWorldCollection (void) {
                m_currentNodeInfoId    = 0;
                m_treeNodeFlags        = ImGuiTreeNodeFlags_OpenOnArrow         |
                                         ImGuiTreeNodeFlags_OpenOnDoubleClick   |
                                         ImGuiTreeNodeFlags_FramePadding        |
                                         ImGuiTreeNodeFlags_SpanFullWidth;
                m_UIWorldCollectionLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIWorldCollection (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Tree nodes layout
             *
             * |Root node A
             * |
             * |------------|Level 0 node
             * |            |
             * |            |-----------|Level 1 node
             * |            |           |
             * |            |           |-----------|Level 2 node
             * |            |           |
             * :            :           :
             * |
             * |Root node B
             * :
            */
            void readyModelCollectionContent (const std::vector <uint32_t>& modelInfoIds,
                                              std::vector <uint32_t>& rootNodeInfoIds) {

                std::vector <uint32_t> level0NodeInfoIds;
                std::vector <uint32_t> level1NodeInfoIds;
                std::vector <uint32_t> level2NodeInfoIds;

                for (auto const& infoId: modelInfoIds) {
                    auto modelInfo = getModelInfo (infoId);

                    level1NodeInfoIds.clear();
                    for (size_t i = 0; i < modelInfo->meta.instances.size(); i++) {

                        level2NodeInfoIds.clear();
                        for (auto const& texId: modelInfo->id.textureImageInfos[Core::DIFFUSE_TEXTURE]) {

                            /* Get texture image info id from look up table for every instance and construct label
                            */
                            uint32_t newTexId     = decodeTexIdLUTPacket (infoId, i, Core::DIFFUSE_TEXTURE, texId);
                            std::string label     = " Diffuse texture [" + std::to_string (newTexId) + "]";
                            auto leafChildInfoIds = std::vector <uint32_t> {};

                            readyTreeLevel (level2NodeInfoIds,
                                            ICON_FA_FIRE + label,
                                            MODEL_NODE | TEXTURE_NODE,
                                            leafChildInfoIds,
                                            newTexId);
                        }

                        for (auto const& texId: modelInfo->id.textureImageInfos[Core::SPECULAR_TEXTURE]) {
                            uint32_t newTexId     = decodeTexIdLUTPacket (infoId, i, Core::SPECULAR_TEXTURE, texId);
                            std::string label     = " Specular texture [" + std::to_string (newTexId) + "]";
                            auto leafChildInfoIds = std::vector <uint32_t> {};

                            readyTreeLevel (level2NodeInfoIds,
                                            ICON_FA_FIRE + label,
                                            MODEL_NODE | TEXTURE_NODE,
                                            leafChildInfoIds,
                                            newTexId);
                        }

                        for (auto const& texId: modelInfo->id.textureImageInfos[Core::EMISSION_TEXTURE]) {
                            uint32_t newTexId     = decodeTexIdLUTPacket (infoId, i, Core::EMISSION_TEXTURE, texId);
                            std::string label     = " Emission texture [" + std::to_string (newTexId) + "]";
                            auto leafChildInfoIds = std::vector <uint32_t> {};

                            readyTreeLevel (level2NodeInfoIds,
                                            ICON_FA_FIRE + label,
                                            MODEL_NODE | TEXTURE_NODE,
                                            leafChildInfoIds,
                                            newTexId);
                        }

                        std::string label = " Instance [" + std::to_string (i) + "]";
                        readyTreeLevel     (level1NodeInfoIds,
                                            ICON_FA_DATABASE + label,
                                            MODEL_NODE | INSTANCE_NODE,
                                            level2NodeInfoIds,
                                            static_cast <uint32_t> (i));
                    }

                    auto label = SandBox::getModelTypeString (static_cast <SandBox::e_modelType> (infoId));
                    readyTreeLevel         (level0NodeInfoIds,
                                            label,
                                            MODEL_NODE | TYPE_NODE,
                                            level1NodeInfoIds,
                                            infoId);
                }

                std::string label = " Model";
                readyTreeLevel             (rootNodeInfoIds,
                                            ICON_FA_CUBE + label,
                                            MODEL_NODE | ROOT_NODE,
                                            level0NodeInfoIds,
                                            UINT32_MAX);
            }

            void readyCameraCollectionContent (uint32_t cameraAnchorInfoId,
                                               std::vector <uint32_t>& rootNodeInfoIds) {

                std::vector <uint32_t> level0NodeInfoIds;
                std::vector <uint32_t> level1NodeInfoIds;
                /* Note that, there is only one camera anchor (unlike light anchors), and its instances will represent
                 * all the available cameras
                */
                auto anchorInfo = getModelInfo (cameraAnchorInfoId);
                for (size_t i = 0; i < anchorInfo->meta.instances.size(); i++) {

                    std::string label     = " Instance ["      + std::to_string (i) + "]" +
                                            ":" + "Info id ["  + std::to_string (i) + "]";
                    auto leafChildInfoIds = std::vector <uint32_t> {};

                    readyTreeLevel (level1NodeInfoIds,
                                    ICON_FA_ANCHOR + label,
                                    ANCHOR_NODE | INSTANCE_NODE | CAMERA_NODE | INFO_ID_NODE,
                                    leafChildInfoIds,
                                    static_cast <uint32_t> (i));
                }
                {
                auto label = SandBox::getAnchorTypeString (static_cast <SandBox::e_anchorType> (cameraAnchorInfoId));
                readyTreeLevel     (level0NodeInfoIds,
                                    label,
                                    ANCHOR_NODE | TYPE_NODE | CAMERA_NODE,
                                    level1NodeInfoIds,
                                    cameraAnchorInfoId);
                }
                {
                std::string label = " Camera";
                readyTreeLevel     (rootNodeInfoIds,
                                    ICON_FA_CAMERA + label,
                                    CAMERA_NODE | ROOT_NODE,
                                    level0NodeInfoIds,
                                    UINT32_MAX);
                }
            }

            void readyLightCollectionContent (const std::vector <uint32_t>& lightInfoIds,
                                              std::vector <uint32_t>& rootNodeInfoIds) {

                std::vector <uint32_t> level0NodeInfoIds;
                std::vector <uint32_t> level1NodeInfoIds;

                for (auto const& infoId: lightInfoIds) {
                    auto lightInfo = getLightInfo (infoId);

                    level1NodeInfoIds.clear();
                    for (size_t i = 0; i < lightInfo->meta.instances.size(); i++) {

                        std::string label     = " Instance ["  + std::to_string (i) + "]" + ":" + 
                                                 "Instance ["  + std::to_string (i) + "]";
                        auto leafChildInfoIds = std::vector <uint32_t> {};

                        readyTreeLevel (level1NodeInfoIds,
                                        ICON_FA_ANCHOR + label,
                                        ANCHOR_NODE | INSTANCE_NODE | LIGHT_NODE,
                                        leafChildInfoIds,
                                        static_cast <uint32_t> (i));
                        }

                    auto label = Core::getLightTypeString (static_cast <Core::e_lightType> (infoId));
                    readyTreeLevel     (level0NodeInfoIds,
                                        label,
                                        LIGHT_NODE | TYPE_NODE,
                                        level1NodeInfoIds,
                                        infoId);
                }

                std::string label = " Light";
                readyTreeLevel         (rootNodeInfoIds,
                                        ICON_FA_SUN + label,
                                        LIGHT_NODE | ROOT_NODE,
                                        level0NodeInfoIds,
                                        UINT32_MAX);
            }
    };
}   // namespace Gui
#endif  // UI_WORLD_COLLECTION