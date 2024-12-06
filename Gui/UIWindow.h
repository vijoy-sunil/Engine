#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <imgui_impl_vulkan.h>
#include <IconFontCppHeaders/IconsFontAwesome6.h>
#include "../Core/Model/VKInstanceData.h"
#include "../Core/Image/VKImageMgr.h"
#include "../Core/Scene/VKSceneMgr.h"
#include "Wrapper/UIPrimitive.h"
#include "Wrapper/UITree.h"
#include "Wrapper/UIPlot.h"
#include "../SandBox/Controller/ENCamera.h"
#include "../SandBox/ENLogHelper.h"

namespace Gui {
    inline e_nodeType operator | (e_nodeType typeA, e_nodeType typeB) {
        return static_cast <e_nodeType> (static_cast <int> (typeA) | static_cast <int> (typeB));
    }

    class UIWindow: protected virtual Core::VKInstanceData,
                    protected virtual Core::VKImageMgr,
                    protected virtual Core::VKSceneMgr,
                    protected UIPrimitive,
                    protected UITree,
                    protected UIPlot,
                    protected SandBox::ENCamera {
        private:
            struct UIBridgeInfo {
                struct CameraFocus {
                    uint32_t modelInfoId;
                    uint32_t modelInstanceId;
                } cameraFocus;

                uint32_t activeCameraInfoId;
            } m_uiBridgeInfo;

            struct UIImageInfo {
                struct Meta {
                    uint32_t selectedLayerIdx;
                    std::string label;
                    std::vector <std::string> fileNames;
                    std::vector <std::string> layerIdxLabels;
                } meta;

                struct Resource {
                    std::vector <VkDescriptorSet> descriptorSets;
                } resource;
            };
            std::unordered_map <uint32_t, UIImageInfo> m_uiImageInfoPool;

            struct UILightInfo {
                struct Meta {
                    uint32_t anchorInstanceId;
                    std::string label;
                } meta;
            };
            std::unordered_map <SandBox::e_anchorType, std::vector <UILightInfo>> m_uiLightInfoPool;

            std::vector <uint32_t> m_rootNodeInfoIds;
            std::vector <uint32_t> m_lockedNodeInfoIds;

            std::vector <std::string> m_diffuseTextureImageInfoIdLabels;
            std::vector <std::string> m_directionalLightInfoIdLabels;
            std::vector <std::string> m_pointLightInfoIdLabels;
            std::vector <std::string> m_spotLightInfoIdLabels;
            std::vector <std::string> m_cameraInfoIdLabels;
            std::vector <std::string> m_cameraTypeLabels;

            uint32_t m_cameraAnchorInfoId;
            uint32_t m_selectedNodeInfoId;
            uint32_t m_selectedPropertyLabelIdx;

            Log::Record* m_UIWindowLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void readyTreeLevel (uint32_t& currentNodeInfoId,
                                 std::vector <uint32_t>& currentLevelNodeInfoIds,
                                 const std::string& label,
                                 e_nodeType type,
                                 const std::vector <uint32_t>& childNodeInfoIds,
                                 uint32_t coreInfoId,
                                 ImGuiTreeNodeFlags treeNodeFlags) {

                currentLevelNodeInfoIds.push_back (currentNodeInfoId);
                readyNodeInfo   (currentNodeInfoId,
                                 label,
                                 type,
                                 childNodeInfoIds,
                                 coreInfoId,
                                 childNodeInfoIds.empty(),
                                 treeNodeFlags);

                /* Update parent node info id for all children
                 */
                for (auto const& infoId: childNodeInfoIds) {
                    auto nodeInfo               = getNodeInfo (infoId);
                    nodeInfo->meta.parentInfoId = currentNodeInfoId;
                }
                currentNodeInfoId++;
            }

            bool isCameraPropertyWritable (void) {
                auto nodeInfo   = getNodeInfo (m_selectedNodeInfoId);
                auto cameraType = getCameraType();

                /* Prevent data write to camera properties when
                 * (1) Node is in 'locked' state, or
                 * (2) Camera type is not set to either drone lock or drone follow
                */
                if (nodeInfo->state.locked ||
                   (cameraType != SandBox::DRONE_LOCK  && cameraType != SandBox::DRONE_FOLLOW))
                    return false;
                else
                    return true;
            }

            ImVec4 getAnchorColor (uint32_t anchorInfoId, uint32_t anchorInstanceId) {
                ImVec4 color;
                color.x = static_cast <float> (decodeTexIdLUTPacket (anchorInfoId, anchorInstanceId, 0))  / UINT8_MAX;
                color.y = static_cast <float> (decodeTexIdLUTPacket (anchorInfoId, anchorInstanceId, 4))  / UINT8_MAX;
                color.z = static_cast <float> (decodeTexIdLUTPacket (anchorInfoId, anchorInstanceId, 8))  / UINT8_MAX;
                color.w = static_cast <float> (decodeTexIdLUTPacket (anchorInfoId, anchorInstanceId, 12)) / UINT8_MAX;

                return color;
            }

            void setAnchorColor (uint32_t anchorInfoId, uint32_t anchorInstanceId, ImVec4 color) {
                color.x *= UINT8_MAX;
                color.y *= UINT8_MAX;
                color.z *= UINT8_MAX;
                color.w *= UINT8_MAX;

                updateTexIdLUT (anchorInfoId, anchorInstanceId, 0,  color.x);
                updateTexIdLUT (anchorInfoId, anchorInstanceId, 4,  color.y);
                updateTexIdLUT (anchorInfoId, anchorInstanceId, 8,  color.z);
                updateTexIdLUT (anchorInfoId, anchorInstanceId, 12, color.w);
            }

        public:
            UIWindow (void) {
#if ENABLE_SAMPLE_MODELS_IMPORT
                m_lockedNodeInfoIds        = g_defaultStateSettings.treeNode.lockedNodesSample;
                m_selectedNodeInfoId       = g_defaultStateSettings.treeNode.worldCollectionSample;
#else
                m_lockedNodeInfoIds        = g_defaultStateSettings.treeNode.lockedNodes;
                m_selectedNodeInfoId       = g_defaultStateSettings.treeNode.worldCollection;
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                m_selectedPropertyLabelIdx = g_defaultStateSettings.button.propertyEditor;

                m_UIWindowLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyUIWindow (const std::vector <uint32_t>& modelInfoIds,
                                uint32_t cameraAnchorInfoId,
                                const std::vector <uint32_t>& lightAnchorInfoIds,
                                uint32_t uiSceneInfoId,
                                uint32_t frameDeltaPlotDataInfoId,
                                uint32_t fpsPlotDataInfoId,
                                const std::unordered_map <uint32_t, std::vector <std::string>>& textureImagePool) {

               auto treeNodeFlags          = ImGuiTreeNodeFlags_OpenOnArrow         |
                                             ImGuiTreeNodeFlags_OpenOnDoubleClick   |
                                             ImGuiTreeNodeFlags_FramePadding        |
                                             ImGuiTreeNodeFlags_SpanFullWidth;
                uint32_t currentNodeInfoId = 0;
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
                /* |------------------------------------------------------------------------------------------------|
                 * | READY TREE NODES - MODEL                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                {
                    std::vector <uint32_t> level0NodeInfoIds;
                    std::vector <uint32_t> level1NodeInfoIds;
                    std::vector <uint32_t> level2NodeInfoIds;

                    for (auto const& infoId: modelInfoIds) {
                        auto modelInfo = getModelInfo (infoId);

                        level1NodeInfoIds.clear();
                        for (size_t i = 0; i < modelInfo->meta.instances.size(); i++) {

                            level2NodeInfoIds.clear();
                            for (auto const& texId: modelInfo->id.diffuseTextureImageInfos) {

                                /* Get texture image info id from look up table for every instance and construct label
                                */
                                uint32_t newTexId     = decodeTexIdLUTPacket (infoId, i, texId);
                                std::string label     = " Diffuse texture [" + std::to_string (newTexId) + "]";
                                auto leafChildInfoIds = std::vector <uint32_t> {};

                                readyTreeLevel (currentNodeInfoId,
                                                level2NodeInfoIds,
                                                ICON_FA_FILE_IMAGE + label,
                                                MODEL_NODE | TEXTURE_NODE,
                                                leafChildInfoIds,
                                                newTexId,
                                                treeNodeFlags);
                            }

                            std::string label = " Instance [" + std::to_string (i) + "]";
                            readyTreeLevel     (currentNodeInfoId,
                                                level1NodeInfoIds,
                                                ICON_FA_DATABASE + label,
                                                MODEL_NODE | INSTANCE_NODE,
                                                level2NodeInfoIds,
                                                static_cast <uint32_t> (i),
                                                treeNodeFlags);
                        }

                        auto label = SandBox::getModelTypeString (static_cast <SandBox::e_modelType> (infoId));
                        readyTreeLevel         (currentNodeInfoId,
                                                level0NodeInfoIds,
                                                label,
                                                MODEL_NODE | TYPE_NODE,
                                                level1NodeInfoIds,
                                                infoId,
                                                treeNodeFlags);
                    }

                    std::string label = " Model";
                    readyTreeLevel             (currentNodeInfoId,
                                                m_rootNodeInfoIds,
                                                ICON_FA_CUBE + label,
                                                MODEL_NODE | ROOT_NODE,
                                                level0NodeInfoIds,
                                                UINT32_MAX,
                                                treeNodeFlags);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY TREE NODES - CAMERA                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                {
                    std::vector <uint32_t> level0NodeInfoIds;
                    std::vector <uint32_t> level1NodeInfoIds;

                    {   /* Note that, there is only one camera anchor (unlike light anchors), and its instances will
                         * represent all the available cameras
                        */
                        auto anchorInfo = getModelInfo (cameraAnchorInfoId);
                        for (size_t i = 0; i < anchorInfo->meta.instances.size(); i++) {

                            std::string label     = " Instance ["      + std::to_string (i) + "]" +
                                                    ":" + "Info id ["  + std::to_string (i) + "]";
                            auto leafChildInfoIds = std::vector <uint32_t> {};

                            readyTreeLevel (currentNodeInfoId,
                                            level1NodeInfoIds,
                                            ICON_FA_ANCHOR + label,
                                            ANCHOR_NODE | INSTANCE_NODE | CAMERA_NODE | INFO_ID_NODE,
                                            leafChildInfoIds,
                                            static_cast <uint32_t> (i),
                                            treeNodeFlags);
                        }

                        auto label = SandBox::getAnchorTypeString (static_cast <SandBox::e_anchorType>
                                     (cameraAnchorInfoId));
                        readyTreeLevel     (currentNodeInfoId,
                                            level0NodeInfoIds,
                                            label,
                                            ANCHOR_NODE | TYPE_NODE | CAMERA_NODE,
                                            level1NodeInfoIds,
                                            cameraAnchorInfoId,
                                            treeNodeFlags);
                    }

                    std::string label = " Camera";
                    readyTreeLevel         (currentNodeInfoId,
                                            m_rootNodeInfoIds,
                                            ICON_FA_CAMERA + label,
                                            CAMERA_NODE | ROOT_NODE,
                                            level0NodeInfoIds,
                                            UINT32_MAX,
                                            treeNodeFlags);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY TREE NODES - LIGHT                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                {
                    std::vector <uint32_t> level0NodeInfoIds;
                    std::vector <uint32_t> level1NodeInfoIds;

                    for (auto const& infoId: lightAnchorInfoIds) {
                        auto anchorInfo = getModelInfo (infoId);

                        level1NodeInfoIds.clear();
                        for (size_t i = 0; i < anchorInfo->meta.instances.size(); i++) {

                            std::string label     = " Instance ["      + std::to_string (i) + "]" +
                                                    ":" + "Info id ["  + std::to_string (i) + "]";
                            auto leafChildInfoIds = std::vector <uint32_t> {};

                            readyTreeLevel (currentNodeInfoId,
                                            level1NodeInfoIds,
                                            ICON_FA_ANCHOR + label,
                                            ANCHOR_NODE | INSTANCE_NODE | LIGHT_NODE | INFO_ID_NODE,
                                            leafChildInfoIds,
                                            static_cast <uint32_t> (i),
                                            treeNodeFlags);
                        }

                        auto label = SandBox::getAnchorTypeString (static_cast <SandBox::e_anchorType> (infoId));
                        readyTreeLevel     (currentNodeInfoId,
                                            level0NodeInfoIds,
                                            label,
                                            ANCHOR_NODE | TYPE_NODE | LIGHT_NODE,
                                            level1NodeInfoIds,
                                            infoId,
                                            treeNodeFlags);
                    }

                    std::string label = " Light";
                    readyTreeLevel         (currentNodeInfoId,
                                            m_rootNodeInfoIds,
                                            ICON_FA_SUN + label,
                                            LIGHT_NODE | ROOT_NODE,
                                            level0NodeInfoIds,
                                            UINT32_MAX,
                                            treeNodeFlags);
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SELECTED NODE                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Open parents of selected node. Note that, parent info id of root nodes are set to UINT32_MAX
                */
                auto nodeInfo = getNodeInfo (m_selectedNodeInfoId);
                if (nodeInfo->meta.parentInfoId != UINT32_MAX)
                    openRootToNode (nodeInfo->meta.parentInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY LOCKED NODES                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& infoId: m_lockedNodeInfoIds) {
                    auto nodeInfo          = getNodeInfo (infoId);
                    nodeInfo->state.locked = true;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY UI IMAGE INFO POOL                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto sceneInfo = getSceneInfo (uiSceneInfoId);
                for (auto const& [infoId, paths]: textureImagePool) {
                    auto imageInfo      = getImageInfo (infoId, Core::TEXTURE_IMAGE);

                    uint32_t layerCount = imageInfo->meta.layerCount;
                    std::string label   = "Info id [" + std::to_string (infoId) + "]";

                    for (uint32_t layerIdx = 0; layerIdx < layerCount; layerIdx++) {
                        /* Strip path to just the file name
                        */
                        size_t stripStart    = paths[layerIdx].find_last_of ("\\/") + 1;
                        std::string fileName = paths[layerIdx].substr (stripStart, paths[layerIdx].length() - stripStart);

                        auto imageView       = layerCount == 1 ? imageInfo->resource.imageView:
                                                                 imageInfo->resource.aliasImageViews[layerIdx];
                        auto descriptorSet   = ImGui_ImplVulkan_AddTexture (sceneInfo->resource.textureSampler,
                                                                            imageView,
                                                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                        m_uiImageInfoPool[infoId].meta.selectedLayerIdx = 0;
                        m_uiImageInfoPool[infoId].meta.label            = label;
                        m_uiImageInfoPool[infoId].meta.fileNames.         push_back (fileName);
                        m_uiImageInfoPool[infoId].meta.layerIdxLabels.    push_back (std::to_string (layerIdx));
                        m_uiImageInfoPool[infoId].resource.descriptorSets.push_back (descriptorSet);
                    }
                }
                /* Note that, the ordering of data in the map must correspond to the ones in the vector. Hence, why we
                 * populate the vector after the map is completely populated
                */
                for (auto const& [key, val]: m_uiImageInfoPool)
                    m_diffuseTextureImageInfoIdLabels.push_back (val.meta.label);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY UI LIGHT INFO POOL                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                for (auto const& infoId: lightAnchorInfoIds) {
                    auto anchorInfo = getModelInfo (infoId);

                    for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {
                        std::string label = "Info id [" + std::to_string (i) + "]";
                        m_uiLightInfoPool[static_cast <SandBox::e_anchorType> (infoId)].push_back ({{i, label}});
                    }
                }

                for (auto const& info: m_uiLightInfoPool[SandBox::ANCHOR_DIRECTIONAL_LIGHT])
                    m_directionalLightInfoIdLabels.push_back (info.meta.label);
                for (auto const& info: m_uiLightInfoPool[SandBox::ANCHOR_POINT_LIGHT])
                    m_pointLightInfoIdLabels.push_back       (info.meta.label);
                for (auto const& info: m_uiLightInfoPool[SandBox::ANCHOR_SPOT_LIGHT])
                    m_spotLightInfoIdLabels.push_back        (info.meta.label);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA INFO ID LABELS                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto anchorInfo = getModelInfo (cameraAnchorInfoId);
                for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {
                    std::string label = "Info id [" + std::to_string (i) + "]";
                    m_cameraInfoIdLabels.push_back (label);
                }
                /* Note that, we need to save the camera anchor info id in order to enable changing the active camera
                 * irrespective of the node selected
                */
                m_cameraAnchorInfoId = cameraAnchorInfoId;
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CAMERA TYPE LABELS                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t cameraTypeCount = 10;
                for (uint32_t i = 0; i < cameraTypeCount; i++)
                    m_cameraTypeLabels.push_back (getCameraTypeString (static_cast <SandBox::e_cameraType> (i)));
                /* |------------------------------------------------------------------------------------------------|
                 * | READY PLOT DATA INFO                                                                           |
                 * |------------------------------------------------------------------------------------------------|
                */
                ImPlot::CreateContext();

                auto& style       = ImPlot::GetStyle();
                style.PlotPadding = g_plotSettings.padding;
                /* Note that, x axis limits are ignored if we are plotting againt time variable, which means the x axis
                 * limits will based on elapsed time and history
                */
                readyPlotDataInfo (frameDeltaPlotDataInfoId,
                                   "Frame delta",
                                   g_plotSettings.history,
                                   0.0f, 0.0f,
                                   0.0f, 0.05f,
                                   true,
                                   g_plotSettings.bufferCapacity,
                                   ImPlotFlags_CanvasOnly,
                                   ImPlotAxisFlags_NoDecorations,
                                   ImPlotLineFlags_Shaded);

                readyPlotDataInfo (fpsPlotDataInfoId,
                                   "FPS",
                                   g_plotSettings.history,
                                   0.0f, 0.0f,
                                   0.0f, 240.0f,
                                   true,
                                   g_plotSettings.bufferCapacity,
                                   ImPlotFlags_CanvasOnly,
                                   ImPlotAxisFlags_NoDecorations,
                                   ImPlotLineFlags_Shaded);
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpNodeInfoPool();
                dumpPlotDataInfoPool();
            }

            void createWorldCollection (bool& showWindow) {
                ImGui::Begin (ICON_FA_DIAGRAM_PROJECT " World Collection", &showWindow, 0);
                /* Right click to open pop up menu
                */
                ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, g_styleSettings.spacing.list);
                if (ImGui::BeginPopupContextWindow()) {
                    if (ImGui::MenuItem ("Expand all",   nullptr, false))  openAllNodes();
                    if (ImGui::MenuItem ("Collapse all", nullptr, false))  closeAllNodes();

                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();

                /* Create tree
                */
                ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2 (0.0f, 0.0f));
                for (auto const& infoId: m_rootNodeInfoIds)
                    createTree (infoId, m_selectedNodeInfoId);
                ImGui::PopStyleVar();

                ImGui::End();
            }

            void createPropertyEditor (bool& showWindow,
                                       bool& showMetricsOverlay,
                                       bool& showBoundingBox,
                                       bool& showShadow) {

                /* Bridge data from other windows
                */
                auto nodeInfo = getNodeInfo (m_selectedNodeInfoId);

                ImGui::Begin (ICON_FA_PEN " Property Editor", &showWindow, 0);
                /* |------------------------------------------------------------------------------------------------|
                 * | LEFT PANEL - BEGIN                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                if (ImGui::BeginChild ("##leftPanel",
                                       ImVec2 (g_styleSettings.size.verticalTabButton.x, 0.0f),
                                       0,
                                       ImGuiWindowFlags_NoBackground)) {

                    auto icons = std::vector <const char*> {
                        ICON_FA_SCISSORS,       /* Transform    */
                        ICON_FA_EYE,            /* View         */
                        ICON_FA_PALETTE,        /* Texture      */
                        ICON_FA_LIGHTBULB,      /* Light        */
                        ICON_FA_PAPER_PLANE,    /* Physics      */
                        ICON_FA_PLUG,           /* Debug        */
                    };

                    auto labels = std::vector <const char*> {
                        "Transform",
                        "View",
                        "Texture",
                        "Light",
                        "Physics",
                        "Debug"
                    };

                    createVerticalTabs (icons,
                                        labels,
                                        g_styleSettings.size.verticalTabButton,
                                        g_styleSettings.color.tabActive,
                                        g_styleSettings.color.tabInactive,
                                        m_selectedPropertyLabelIdx);
                }
                ImGui::EndChild();
                /* |------------------------------------------------------------------------------------------------|
                 * | LEFT PANEL - END                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                ImGui::SameLine();
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - BEGIN                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                ImGui::PushStyleVar   (ImGuiStyleVar_WindowPadding, g_styleSettings.padding.child);
                ImGui::PushStyleVar   (ImGuiStyleVar_FrameRounding, g_styleSettings.rounding.inputField);
                if (ImGui::BeginChild ("##rightPanel",
                                       ImVec2 (0.0f, 0.0f),
                                       ImGuiChildFlags_AlwaysUseWindowPadding,
                                       0)) {
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - TRANSFORM MODEL/LIGHT                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    if (m_selectedPropertyLabelIdx == TRANSFORM) {

                        if ((nodeInfo->meta.type & MODEL_NODE) ||
                            (nodeInfo->meta.type & LIGHT_NODE)) {

                            glm::vec3 position           = {0.0f, 0.0f, 0.0f};
                            glm::vec3 scale              = {0.0f, 0.0f, 0.0f};
                            glm::vec3 rotateAngleDeg     = {0.0f, 0.0f, 0.0f};
                            bool fieldDisable            = false;
                            bool writePending            = false;

                            if (nodeInfo->meta.type & INSTANCE_NODE) {
                                auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                                uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;
                                position                 = modelInfo->meta.transformDatas[modelInstanceId].position;
                                scale                    = modelInfo->meta.transformDatas[modelInstanceId].scale;
                                rotateAngleDeg           = modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg;

                                if (nodeInfo->state.locked)
                                    fieldDisable         = true;
                            }
                            else
                                fieldDisable             = true;

                            ImGui::Text              ("%s",             "Position");
                            if (createFloatTextField ("##positionX",    "X",    "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.x)         ||
                                createFloatTextField ("##positionY",    "Y",    "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.y)         ||
                                createFloatTextField ("##positionZ",    "Z",    "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.z))
                                writePending = true;

                            ImGui::Text              ("%s",             "Rotate angle");
                            if (createFloatTextField ("##rotateAngleX", "X",    "deg",      g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAngleDeg.x)   ||
                                createFloatTextField ("##rotateAngleY", "Y",    "deg",      g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAngleDeg.y)   ||
                                createFloatTextField ("##rotateAngleZ", "Z",    "deg",      g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAngleDeg.z))
                                writePending = true;

                            ImGui::Text              ("%s",             "Scale");
                            if (createFloatTextField ("##scaleX",       "X",    "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.x)            ||
                                createFloatTextField ("##scaleY",       "Y",    "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.y)            ||
                                createFloatTextField ("##scaleZ",       "Z",    "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.z))
                                writePending = true;

                            {   /* Data write */
                                if (!fieldDisable && writePending) {
                                    auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                    auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                                    uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                                    modelInfo->meta.transformDatas[modelInstanceId].position       = position;
                                    modelInfo->meta.transformDatas[modelInstanceId].scale          = scale;
                                    modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg = rotateAngleDeg;
                                    createModelMatrix (parentNodeInfo->meta.coreInfoId, modelInstanceId);
                                }
                            }
                        }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - TRANSFORM CAMERA                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                        else if (nodeInfo->meta.type & CAMERA_NODE) {

                            glm::vec3 position   = {0.0f, 0.0f, 0.0f};
                            glm::vec3 direction  = {0.0f, 0.0f, 0.0f};
                            glm::vec3 upVector   = {0.0f, 0.0f, 0.0f};
                            bool fieldDisable    = false;
                            bool writePending    = false;

                            if (nodeInfo->meta.type & INSTANCE_NODE) {
                                auto cameraInfo  = getCameraInfo (nodeInfo->meta.coreInfoId);
                                position         = cameraInfo->meta.position;
                                direction        = cameraInfo->meta.direction;
                                upVector         = cameraInfo->meta.upVector;

                                if (!isCameraPropertyWritable())
                                    fieldDisable = true;
                            }
                            else
                                fieldDisable     = true;

                            ImGui::Text              ("%s",           "Position");
                            if (createFloatTextField ("##positionX",  "X", "m",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.x)     ||
                                createFloatTextField ("##positionY",  "Y", "m",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.y)     ||
                                createFloatTextField ("##positionZ",  "Z", "m",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.z))
                                writePending = true;

                            ImGui::Text              ("%s",           "Direction");
                            if (createFloatTextField ("##directionX", "X", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, direction.x)    ||
                                createFloatTextField ("##directionY", "Y", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, direction.y)    ||
                                createFloatTextField ("##directionZ", "Z", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, direction.z))
                                writePending = true;

                            ImGui::Text              ("%s",           "Up vector");
                            if (createFloatTextField ("##upVectorX",  "X", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, upVector.x)     ||
                                createFloatTextField ("##upVectorY",  "Y", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, upVector.y)     ||
                                createFloatTextField ("##upVectorZ",  "Z", "u",             g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, upVector.z))
                                writePending = true;

                            {   /* Data write */
                                if (!fieldDisable && writePending) {
                                    auto cameraInfo                   = getCameraInfo (nodeInfo->meta.coreInfoId);
                                    cameraInfo->meta.position         = position;
                                    cameraInfo->meta.direction        = direction;
                                    cameraInfo->meta.upVector         = upVector;
                                    cameraInfo->meta.updateViewMatrix = true;
                                    setModelTransformRemoved (false);
                                }
                            }
                        }
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - VIEW                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == VIEW) {

                        uint32_t selectedCameraInfoIdLabelIdx = m_uiBridgeInfo.activeCameraInfoId;
                        uint32_t selectedCameraTypeLabelIdx   = static_cast <uint32_t> (getCameraType());
                        float fovDeg                          = 0.0f;
                        float nearPlane                       = 0.0f;
                        float farPlane                        = 0.0f;
                        bool setFocus                         = false;
                        bool hideRender                       = false;
                        bool fieldDisable                     = false;
                        bool writePending                     = false;

                        if ((nodeInfo->meta.type & CAMERA_NODE) &&
                            (nodeInfo->meta.type & INSTANCE_NODE)) {

                            auto cameraInfo  = getCameraInfo (nodeInfo->meta.coreInfoId);
                            fovDeg           = cameraInfo->meta.fovDeg;
                            nearPlane        = cameraInfo->meta.nearPlane;
                            farPlane         = cameraInfo->meta.farPlane;

                            if (!isCameraPropertyWritable())
                                fieldDisable = true;
                        }
                        else
                            fieldDisable     = true;
                        /* To prevent showing post label, prefix it with '##'
                        */
                        if (createCombo      ("##cameraActive",
                                              "Camera active",
                                              "##postLabelCameraActive",
                                              m_cameraInfoIdLabels,
                                              false,
                                              g_styleSettings.size.inputFieldLarge,
                                              selectedCameraInfoIdLabelIdx))
                        {   /* Data write */
                            auto anchorInfo            = getModelInfo (m_cameraAnchorInfoId);
                            uint32_t anchorInstanceId  = selectedCameraInfoIdLabelIdx;
                            auto position              = anchorInfo->meta.transformDatas[anchorInstanceId].position;
                            auto rotateAngleDeg        = anchorInfo->meta.transformDatas[anchorInstanceId].rotateAngleDeg;
                            /* Match the selected camera's pose with the anchor instance's pose. Note that, usually when
                             * switching to the drone camera types, we use the previous type's values for fov, etc. as
                             * the initial values. However, since we are setting the initial camera type to drone lock,
                             * we will need to manually set them as shown below
                            */
                            auto cameraInfo            = getCameraInfo (anchorInstanceId);
                            cameraInfo->meta.position  = position;

                            float yawDeg               = -rotateAngleDeg.y;
                            float pitchDeg             = -rotateAngleDeg.x;
                            cameraInfo->meta.direction = getDirectionVector (yawDeg, pitchDeg);
                            cameraInfo->meta.fovDeg    = 80.0f;
                            /* Note that, upon chainging the active camera, we are setting the camera type to drone lock
                             * type which inherently doesn't set the boolean to update the camera matrices. Hence, why
                             * we need to explicitly set them
                            */
                            cameraInfo->meta.updateViewMatrix       = true;
                            cameraInfo->meta.updateProjectionMatrix = true;
                            /* Update active camera info id
                            */
                            m_uiBridgeInfo.activeCameraInfoId       = anchorInstanceId;
                            setCameraActive (anchorInstanceId, SandBox::DRONE_LOCK);
                        }

                        if (createCombo      ("##cameraType",
                                              "Camera type",
                                              "##postLabelCameraType",
                                              m_cameraTypeLabels,
                                              false,
                                              g_styleSettings.size.inputFieldLarge,
                                              selectedCameraTypeLabelIdx))
                        {   /* Data write */
                            setCameraType    (static_cast <SandBox::e_cameraType> (selectedCameraTypeLabelIdx));
                        }

                        if (createFloatTextField ("##fov",       "FOV",        "deg",   g_styleSettings.precision,
                                                  fieldDisable,
                                                  g_styleSettings.size.inputFieldSmall, fovDeg)     ||
                            createFloatTextField ("##nearPlane", "Near plane", "m",     g_styleSettings.precision,
                                                  fieldDisable,
                                                  g_styleSettings.size.inputFieldSmall, nearPlane)  ||
                            createFloatTextField ("##farPlane",  "Far plane",  "m",     g_styleSettings.precision,
                                                  fieldDisable,
                                                  g_styleSettings.size.inputFieldSmall, farPlane))
                            writePending = true;

                        {   /* Data write */
                            if (!fieldDisable && writePending) {
                                auto cameraInfo                         = getCameraInfo (nodeInfo->meta.coreInfoId);
                                cameraInfo->meta.fovDeg                 = fovDeg;
                                cameraInfo->meta.nearPlane              = nearPlane;
                                cameraInfo->meta.farPlane               = farPlane;
                                cameraInfo->meta.updateProjectionMatrix = true;
                                setModelTransformRemoved (false);
                            }
                        }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - VIEW > SET FOCUS                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                        /* Note that, we want to prevent the camera from setting focus on itself, hence why the 'if'
                         * condition
                        */
                        if (!(nodeInfo->meta.type & CAMERA_NODE) &&
                             (nodeInfo->meta.type & INSTANCE_NODE)) {

                            auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                            uint32_t modelInfoId     = parentNodeInfo->meta.coreInfoId;
                            uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                            if ((m_uiBridgeInfo.cameraFocus.modelInfoId     == modelInfoId) &&
                                (m_uiBridgeInfo.cameraFocus.modelInstanceId == modelInstanceId))
                                setFocus             = true;
                            fieldDisable             = false;
                        }
                        else
                            fieldDisable             = true;

                        createCheckBoxButton ("##setFocus",
                                              "Set focus",
                                              "##postLabelSetFocus",
                                              fieldDisable,
                                              setFocus);
                        {   /* Data write */
                            if (!fieldDisable) {
                                auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                uint32_t modelInfoId     = parentNodeInfo->meta.coreInfoId;
                                uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                                if (setFocus) {
                                    m_uiBridgeInfo.cameraFocus.modelInfoId     = modelInfoId;
                                    m_uiBridgeInfo.cameraFocus.modelInstanceId = modelInstanceId;
                                }
                            }
                        }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - VIEW > HIDE MODEL/HIDE ANCHOR                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                        if (nodeInfo->meta.type & INSTANCE_NODE) {
                            auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                            auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                            uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                            if (modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier == 0.0f)
                                hideRender           = true;
                            else
                                hideRender           = false;
                            fieldDisable             = false;
                        }
                        else
                            fieldDisable             = true;

                        const char* preLabel = (nodeInfo->meta.type & ANCHOR_NODE) ? "Hide anchor": "Hide model";
                        createCheckBoxButton ("##hideRender",
                                              preLabel,
                                              "##postLabelHideRender",
                                              fieldDisable,
                                              hideRender);
                        {   /* Data write */
                            if (!fieldDisable) {
                                auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                                uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                                if (hideRender)
                                    modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier = 0.0f;
                                else
                                    modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier = 1.0f;
                                createModelMatrix (parentNodeInfo->meta.coreInfoId, modelInstanceId);
                            }
                        }

                        createCheckBoxButton ("##metrics",
                                              "Metrics",
                                              "##postLabelMetrics",
                                              false,
                                              showMetricsOverlay);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - TEXTURE                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == TEXTURE) {
                        static uint32_t selectedDiffuseLabelIdx = 0;
                        bool fieldDisable                       = false;

                        if (nodeInfo->meta.type & MODEL_NODE &&
                            nodeInfo->meta.type & TEXTURE_NODE) {

                            uint32_t infoId         = nodeInfo->meta.coreInfoId;
                            /* Convert texture image info id to label, and we use the label to find the offset to the
                             * labels vector. This provides us a common index to access both the map and the vector
                            */
                            std::string label       = "Info id [" + std::to_string (infoId) + "]";
                            selectedDiffuseLabelIdx = std::find (m_diffuseTextureImageInfoIdLabels.begin(),
                                                                 m_diffuseTextureImageInfoIdLabels.end(),
                                                                 label) - m_diffuseTextureImageInfoIdLabels.begin();
                            fieldDisable            = true;
                        }

                        createCombo ("##diffuse",
                                     "Diffuse",
                                     "##postLabelDiffuse",
                                     m_diffuseTextureImageInfoIdLabels,
                                     fieldDisable,
                                     g_styleSettings.size.inputFieldLarge,
                                     selectedDiffuseLabelIdx);

                        /* Note that, we have already ensured that there is 1:1 correspondence between the map and
                         * the vector. This makes it possible to offset into the map using an index to the vector
                        */
                        auto iter = std::next (m_uiImageInfoPool.begin(), selectedDiffuseLabelIdx);

                        createCombo ("##layer",
                                     "Layer",
                                     "##postLabelLayer",
                                     iter->second.meta.layerIdxLabels,
                                     false,
                                     g_styleSettings.size.inputFieldLarge,
                                     iter->second.meta.selectedLayerIdx);

                        createImagePreview (iter->second.resource.descriptorSets[iter->second.meta.selectedLayerIdx],
                                            g_styleSettings.size.image,
                                            g_styleSettings.color.border);
                        /* Image details
                        */
                        auto imageInfo       = getImageInfo (iter->first, Core::TEXTURE_IMAGE);
                        std::string dims     = std::to_string (imageInfo->meta.width)  + "px"
                                               + " x " +
                                               std::to_string (imageInfo->meta.height) + "px";
                        std::string fileName = iter->second.meta.fileNames[iter->second.meta.selectedLayerIdx];

                        ImGui::Text ("%s", dims.c_str());
                        ImGui::Text ("%s", fileName.c_str());
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - LIGHT                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == LIGHT) {
                        static ImVec4 ambientColor                  = {0.0f, 0.0f, 0.0f, 1.0f};

                        static uint32_t selectedDirectionalLabelIdx = 0;
                        static uint32_t selectedPointLabelIdx       = 0;
                        static uint32_t selectedSpotLabelIdx        = 0;

                        bool fieldDisableDirectional                = false;
                        bool fieldDisablePoint                      = false;
                        bool fieldDisableSpot                       = false;

                        if (nodeInfo->meta.type & LIGHT_NODE &&
                            nodeInfo->meta.type & INSTANCE_NODE) {

                            auto parentNodeInfo = getNodeInfo (nodeInfo->meta.parentInfoId);
                            auto anchorType     = static_cast <SandBox::e_anchorType> (parentNodeInfo->meta.coreInfoId);

                            if (anchorType == SandBox::ANCHOR_DIRECTIONAL_LIGHT) {

                                uint32_t infoId             = nodeInfo->meta.coreInfoId;
                                std::string label           = "Info id [" + std::to_string (infoId) + "]";
                                selectedDirectionalLabelIdx = std::find (m_directionalLightInfoIdLabels.begin(),
                                                                         m_directionalLightInfoIdLabels.end(), label) -
                                                                         m_directionalLightInfoIdLabels.begin();
                                fieldDisableDirectional     = true;
                            }

                            else if (anchorType == SandBox::ANCHOR_POINT_LIGHT)  {

                                uint32_t infoId             = nodeInfo->meta.coreInfoId;
                                std::string label           = "Info id [" + std::to_string (infoId) + "]";
                                selectedPointLabelIdx       = std::find (m_pointLightInfoIdLabels.begin(),
                                                                         m_pointLightInfoIdLabels.end(), label) -
                                                                         m_pointLightInfoIdLabels.begin();
                                fieldDisablePoint           = true;
                            }

                            else if (anchorType == SandBox::ANCHOR_SPOT_LIGHT)   {

                                uint32_t infoId             = nodeInfo->meta.coreInfoId;
                                std::string label           = "Info id [" + std::to_string (infoId) + "]";
                                selectedSpotLabelIdx        = std::find (m_spotLightInfoIdLabels.begin(),
                                                                         m_spotLightInfoIdLabels.end(), label) -
                                                                         m_spotLightInfoIdLabels.begin();
                                fieldDisableSpot            = true;
                            }
                        }

                        createColorButton ("##ambient",
                                           "Ambient",
                                           false,
                                           g_styleSettings.size.inputFieldLarge,
                                           ambientColor);
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - DIRECTIONAL LIGHT                                                                |
                 * |------------------------------------------------------------------------------------------------|
                */
                        if (!m_directionalLightInfoIdLabels.empty()) {
                            createCombo           ("##directional",
                                                   "Directional",
                                                   "##postLabelDirectional",
                                                   m_directionalLightInfoIdLabels,
                                                   fieldDisableDirectional,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   selectedDirectionalLabelIdx);
                            {   /* Data read/write */
                                auto anchorType           = SandBox::ANCHOR_DIRECTIONAL_LIGHT;
                                auto infos                = m_uiLightInfoPool[anchorType];
                                auto iter                 = std::next (infos.begin(), selectedDirectionalLabelIdx);
                                uint32_t anchorInstanceId = iter->meta.anchorInstanceId;
                                ImVec4 directionalColor   = getAnchorColor (anchorType, anchorInstanceId);

                                createColorButton ("##directional",
                                                   "Color",
                                                   false,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   directionalColor);

                                setAnchorColor    (anchorType, anchorInstanceId, directionalColor);
                            }
                        }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - POINT LIGHT                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                        if (!m_pointLightInfoIdLabels.empty()) {
                            createCombo           ("##point",
                                                   "Point",
                                                   "##postLabelPoint",
                                                   m_pointLightInfoIdLabels,
                                                   fieldDisablePoint,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   selectedPointLabelIdx);
                            {   /* Data read/write */
                                auto anchorType           = SandBox::ANCHOR_POINT_LIGHT;
                                auto infos                = m_uiLightInfoPool[anchorType];
                                auto iter                 = std::next (infos.begin(), selectedPointLabelIdx);
                                uint32_t anchorInstanceId = iter->meta.anchorInstanceId;
                                ImVec4 pointColor         = getAnchorColor (anchorType, anchorInstanceId);

                                createColorButton ("##point",
                                                   "Color",
                                                   false,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   pointColor);

                                setAnchorColor    (anchorType, anchorInstanceId, pointColor);
                            }
                        }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - SPOT LIGHT                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                        if (!m_spotLightInfoIdLabels.empty()) {
                            createCombo           ("##spot",
                                                   "Spot",
                                                   "##postLabelSpot",
                                                   m_spotLightInfoIdLabels,
                                                   fieldDisableSpot,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   selectedSpotLabelIdx);
                            {   /* Data read/write */
                                auto anchorType           = SandBox::ANCHOR_SPOT_LIGHT;
                                auto infos                = m_uiLightInfoPool[anchorType];
                                auto iter                 = std::next (infos.begin(), selectedSpotLabelIdx);
                                uint32_t anchorInstanceId = iter->meta.anchorInstanceId;
                                ImVec4 spotColor          = getAnchorColor (anchorType, anchorInstanceId);

                                createColorButton ("##spot",
                                                   "Color",
                                                   false,
                                                   g_styleSettings.size.inputFieldLarge,
                                                   spotColor);

                                setAnchorColor    (anchorType, anchorInstanceId, spotColor);
                            }
                        }

                        createCheckBoxButton ("##shadow",
                                              "Shadow",
                                              "##postLabelShadow",
                                              true,
                                              showShadow);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - PHYSICS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == PHYSICS) {
                        /* [ X ] Pending implementation */
                        createCheckBoxButton ("##boundingBox",
                                              "Bounding box",
                                              "##postLabelBoundingBox",
                                              true,
                                              showBoundingBox);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - DEBUG                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == DEBUG) {
                        /* [ X ] Pending implementation */
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleVar (2);
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - END                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                ImGui::End();
            }

            UIBridgeInfo* getUIBridgeInfo (void) {
                return &m_uiBridgeInfo;
            }

            void cleanUp (const std::vector <uint32_t>& plotDataInfoIds) {
                for (auto const& [key, val]: m_uiImageInfoPool) {
                    for (auto const& descriptorSet: val.resource.descriptorSets)
                        ImGui_ImplVulkan_RemoveTexture (descriptorSet);
                }
                m_uiImageInfoPool.clear();

                UITree::cleanUp (UINT32_MAX);

                for (auto const& infoId: plotDataInfoIds)
                    UIPlot::cleanUp (infoId);
                ImPlot::DestroyContext();
            }
    };
}   // namespace Gui
#endif  // UI_WINDOW_H