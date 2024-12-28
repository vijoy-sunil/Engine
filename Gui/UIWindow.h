#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "Content/UIWorldCollection.h"
#include "Content/UITransform.h"
#include "Content/UIView.h"
#include "Content/UITexture.h"
#include "Content/UILight.h"
#include "Content/UIPhysics.h"
#include "Content/UIDebug.h"

namespace Gui {
    class UIWindow: protected UIWorldCollection,
                    protected UITransform,
                    protected UIView,
                    protected UITexture,
                    protected UILight,
                    protected UIPhysics,
                    protected UIDebug {
        private:
            std::vector <uint32_t>    m_rootNodeInfoIds;
            std::vector <const char*> m_propertyIcons;
            std::vector <const char*> m_propertyLabels;

            uint32_t m_selectedNodeInfoId;
            uint32_t m_selectedPropertyLabelIdx;

            Log::Record* m_UIWindowLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIWindow (void) {
                m_selectedPropertyLabelIdx = g_defaultStateSettings.button.propertyEditor;
                m_UIWindowLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyUIWindow (const std::vector <uint32_t>& modelInfoIds,
                                uint32_t cameraAnchorInfoId,
                                uint32_t uiSceneInfoId,
                                const std::vector <uint32_t>& lightInfoIds,
                                const std::unordered_map <Core::e_textureType,
                                      std::unordered_map <uint32_t, std::vector <std::string>>>& textureImagePools) {
                /* |------------------------------------------------------------------------------------------------|
                 * | READY CONTENTS                                                                                 |
                 * |------------------------------------------------------------------------------------------------|
                */
                UIWorldCollection::readyModelCollectionContent  (modelInfoIds,       m_rootNodeInfoIds);
                UIWorldCollection::readyCameraCollectionContent (cameraAnchorInfoId, m_rootNodeInfoIds);
                UIWorldCollection::readyLightCollectionContent  (lightInfoIds,       m_rootNodeInfoIds);

                UIView::readyContent    (cameraAnchorInfoId);
                UITexture::readyContent (uiSceneInfoId, textureImagePools);
                UILight::readyContent   (lightInfoIds);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY SELECTED NODE                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Open parents of selected node. Note that, parent info id of root nodes are set to UINT32_MAX
                */
                m_selectedNodeInfoId  = getNodeInfoId (CAMERA_NODE | INFO_ID_NODE, 0);
                auto selectedNodeInfo = getNodeInfo   (m_selectedNodeInfoId);

                if (selectedNodeInfo->meta.parentInfoId != UINT32_MAX)
                    openRootToNode (selectedNodeInfo->meta.parentInfoId);
                /* |------------------------------------------------------------------------------------------------|
                 * | READY LOCKED NODES                                                                             |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t lockedNodeParentInfoId = getNodeInfoId (MODEL_NODE | TYPE_NODE, SandBox::SKY_BOX);
                auto lockedNodeParentInfo       = getNodeInfo   (lockedNodeParentInfoId);
                /* Lock all child nodes
                */
                for (auto const& infoId: lockedNodeParentInfo->meta.childInfoIds) {
                    auto nodeInfo          = getNodeInfo (infoId);
                    nodeInfo->state.locked = true;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY PROPERTY ICONS, LABELS                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                m_propertyIcons = {
                    ICON_FA_SCISSORS,       /* Transform    */
                    ICON_FA_EYE,            /* View         */
                    ICON_FA_PALETTE,        /* Texture      */
                    ICON_FA_LIGHTBULB,      /* Light        */
                    ICON_FA_PAPER_PLANE,    /* Physics      */
                    ICON_FA_PLUG,           /* Debug        */
                };

                m_propertyLabels = {
                    "Transform",
                    "View",
                    "Texture",
                    "Light",
                    "Physics",
                    "Debug"
                };
                /* |------------------------------------------------------------------------------------------------|
                 * | DUMP METHODS                                                                                   |
                 * |------------------------------------------------------------------------------------------------|
                */
                dumpNodeInfoPool();
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

                ImGui::Begin (ICON_FA_PEN " Property Editor", &showWindow, 0);
                /* |------------------------------------------------------------------------------------------------|
                 * | LEFT PANEL                                                                                     |
                 * |------------------------------------------------------------------------------------------------|
                */
                if (ImGui::BeginChild ("##leftPanel",
                                       ImVec2 (g_styleSettings.size.verticalTabButton.x, 0.0f),
                                       0,
                                       ImGuiWindowFlags_NoBackground)) {

                    createVerticalTabs (m_propertyIcons,
                                        m_propertyLabels,
                                        g_styleSettings.size.verticalTabButton,
                                        g_styleSettings.color.tabActive,
                                        g_styleSettings.color.tabInactive,
                                        m_selectedPropertyLabelIdx);
                }
                ImGui::EndChild();
                ImGui::SameLine();
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL                                                                                    |
                 * |------------------------------------------------------------------------------------------------|
                */
                ImGui::PushStyleVar   (ImGuiStyleVar_WindowPadding, g_styleSettings.padding.child);
                ImGui::PushStyleVar   (ImGuiStyleVar_FrameRounding, g_styleSettings.rounding.inputField);
                if (ImGui::BeginChild ("##rightPanel",
                                       ImVec2 (0.0f, 0.0f),
                                       ImGuiChildFlags_AlwaysUseWindowPadding,
                                       0)) {

                    switch (m_selectedPropertyLabelIdx) {
                        case TRANSFORM: UITransform::createContent (m_selectedNodeInfoId);                      break;
                        case VIEW:      UIView::     createContent (m_selectedNodeInfoId, showMetricsOverlay);  break;
                        case TEXTURE:   UITexture::  createContent (m_selectedNodeInfoId);                      break;
                        case LIGHT:     UILight::    createContent (m_selectedNodeInfoId, showShadow);          break;
                        case PHYSICS:   UIPhysics::  createContent (showBoundingBox);                           break;
                        case DEBUG:     UIDebug::    createContent();                                           break;
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleVar (2);
                ImGui::End();
            }

            void cleanUp (void) {
                UITexture::cleanUp();
                UITree::cleanUp (UINT32_MAX);
            }
    };
}   // namespace Gui
#endif  // UI_WINDOW_H