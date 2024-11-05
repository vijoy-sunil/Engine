#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <imgui_impl_vulkan.h>
#include <IconFontCppHeaders/IconsFontAwesome6.h>
#include "../Core/Model/VKModelMatrix.h"
#include "../Core/Image/VKImageMgr.h"
#include "../Core/Scene/VKSceneMgr.h"
#include "Wrapper/UIPrimitive.h"
#include "Wrapper/UITree.h"
#include "Wrapper/UIOverlay.h"
#include "Wrapper/UIPlot.h"
#include "../SandBox/Controller/ENCamera.h"
#include "../SandBox/ENLogHelper.h"

namespace Gui {
    class UIWindow: protected virtual Core::VKModelMatrix,
                    protected virtual Core::VKImageMgr,
                    protected virtual Core::VKSceneMgr,
                    protected UIPrimitive,
                    protected UITree,
                    protected UIOverlay,
                    protected UIPlot,
                    protected SandBox::ENCamera {
        private:
            struct UIImageInfo {
                struct Meta {
                    std::string label;
                    std::string fileName;
                } meta;

                struct Resource {
                    VkDescriptorSet descriptorSet;
                } resource;
            };
            std::unordered_map <uint32_t, UIImageInfo> m_uiImageInfoPool;

            std::vector <uint32_t>    m_rootNodeInfoIds;
            std::vector <std::string> m_cameraTypeLabels;
            std::vector <std::string> m_diffuseTextureImageInfoIdLabels;

            uint32_t m_selectedNodeInfoId;
            uint32_t m_selectedPropertyLabelIdx;

            Log::Record* m_UIWindowLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            bool isCameraPropertyWritable (void) {
                auto cameraType = getCameraType();
                /* Allow data write to camera properties when camera type is set to either drone lock or drone follow
                */
                if (cameraType != SandBox::DRONE_LOCK  && cameraType != SandBox::DRONE_FOLLOW)
                    return false;
                else
                    return true;
            }

            void deleteUIImageInfo (uint32_t uiImageInfoId) {
                if (m_uiImageInfoPool.find (uiImageInfoId) != m_uiImageInfoPool.end()) {
                    ImGui_ImplVulkan_RemoveTexture (m_uiImageInfoPool[uiImageInfoId].resource.descriptorSet);
                    m_uiImageInfoPool.erase        (uiImageInfoId);
                    return;
                }

                LOG_ERROR (m_UIWindowLog) << "Failed to delete ui image info "
                                          << "[" << uiImageInfoId << "]"
                                          << std::endl;
                throw std::runtime_error ("Failed to delete ui image info");
            }

        public:
            UIWindow (void) {
#if ENABLE_SAMPLE_MODELS_IMPORT
                m_selectedNodeInfoId       = g_defaultStateSettings.treeNode.worldCollectionSample;
#else
                m_selectedNodeInfoId       = g_defaultStateSettings.treeNode.worldCollection;
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
                m_selectedPropertyLabelIdx = g_defaultStateSettings.button.propertyEditor;

                m_UIWindowLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~UIWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyUIWindow (const std::vector <uint32_t>& modelInfoIds,
                                const std::vector <uint32_t>& cameraInfoIds,
                                uint32_t uiSceneInfoId,
                                uint32_t frameDeltaPlotDataInfoId,
                                uint32_t fpsPlotDataInfoId) {

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

                    level0NodeInfoIds.clear();
                    for (auto const& infoId: modelInfoIds) {
                        auto modelInfo = getModelInfo (infoId);

                        level1NodeInfoIds.clear();
                        for (size_t i = 0; i < modelInfo->meta.instances.size(); i++) {

                            level2NodeInfoIds.clear();
                            for (auto const& texId: modelInfo->id.diffuseTextureImageInfos) {
                                level2NodeInfoIds.push_back (currentNodeInfoId);
                                /* Get texture id from look up table for every instance
                                */
                                const uint32_t numColumns = 4;
                                uint32_t rowIdx           = texId / numColumns;
                                uint32_t colIdx           = texId % numColumns;
                                /* Construct label
                                */
                                std::string label         = " Diffuse texture [" + std::to_string (static_cast <uint32_t>
                                                            (modelInfo->meta.instances[i].texIdLUT[rowIdx][colIdx])) +
                                                            "]";
                                /* Since this is a leaf node, the child info id vector be empty
                                */
                                auto leafChildInfoIds     = std::vector <uint32_t> {};

                                readyNodeInfo (currentNodeInfoId,
                                               ICON_FA_FILE_IMAGE + label,
                                               MODEL_TEXTURE_NODE,
                                               UNDEFINED_ACTION,
                                               leafChildInfoIds,
                                               texId,
                                               true,
                                               treeNodeFlags);

                                currentNodeInfoId++;
                            }

                            level1NodeInfoIds.push_back (currentNodeInfoId);
                            std::string label = " Instance [" + std::to_string (i) + "]";
                            readyNodeInfo (currentNodeInfoId,
                                           ICON_FA_DATABASE + label,
                                           MODEL_INSTANCE_NODE,
                                           UNDEFINED_ACTION,
                                           level2NodeInfoIds,
                                           static_cast <uint32_t> (i),
                                           false,
                                           treeNodeFlags);
                            /* Update parent node info id for all children
                            */
                            for (auto const& infoId: level2NodeInfoIds) {
                                auto nodeInfo               = getNodeInfo (infoId);
                                nodeInfo->meta.parentInfoId = currentNodeInfoId;
                            }
                            currentNodeInfoId++;
                        }

                        level0NodeInfoIds.push_back (currentNodeInfoId);
                        auto label = SandBox::getModelTypeString (static_cast <SandBox::e_modelType> (infoId));
                        readyNodeInfo (currentNodeInfoId,
                                       label,
                                       MODEL_TYPE_NODE,
                                       UNDEFINED_ACTION,
                                       level1NodeInfoIds,
                                       infoId,
                                       false,
                                       treeNodeFlags);

                        for (auto const& infoId: level1NodeInfoIds) {
                            auto nodeInfo               = getNodeInfo (infoId);
                            nodeInfo->meta.parentInfoId = currentNodeInfoId;
                        }
                        currentNodeInfoId++;
                    }

                    /* Save root node id
                    */
                    m_rootNodeInfoIds.push_back (currentNodeInfoId);
                    std::string label = " Model";
                    readyNodeInfo (currentNodeInfoId,
                                   ICON_FA_CUBE + label,
                                   MODEL_ROOT_NODE,
                                   UNDEFINED_ACTION,
                                   level0NodeInfoIds,
                                   UINT32_MAX,
                                   false,
                                   treeNodeFlags);

                    for (auto const& infoId: level0NodeInfoIds) {
                        auto nodeInfo               = getNodeInfo (infoId);
                        nodeInfo->meta.parentInfoId = currentNodeInfoId;
                    }
                    currentNodeInfoId++;
                }
                /* |------------------------------------------------------------------------------------------------|
                 * | READY TREE NODES - CAMERA                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                {
                    std::vector <uint32_t> level0NodeInfoIds;

                    level0NodeInfoIds.clear();
                    for (auto const& infoId: cameraInfoIds) {

                        level0NodeInfoIds.push_back (currentNodeInfoId);
                        std::string label     = " Info id [" + std::to_string (infoId) + "]";
                        auto leafChildInfoIds = std::vector <uint32_t> {};
                        readyNodeInfo (currentNodeInfoId,
                                       ICON_FA_FILE + label,
                                       CAMERA_INFO_ID_NODE,
                                       UNDEFINED_ACTION,
                                       leafChildInfoIds,
                                       infoId,
                                       true,
                                       treeNodeFlags);
                        currentNodeInfoId++;
                    }

                    /* Save root node id
                    */
                    m_rootNodeInfoIds.push_back (currentNodeInfoId);
                    std::string label = " Camera";
                    readyNodeInfo (currentNodeInfoId,
                                   ICON_FA_CAMERA + label,
                                   CAMERA_ROOT_NODE,
                                   UNDEFINED_ACTION,
                                   level0NodeInfoIds,
                                   UINT32_MAX,
                                   false,
                                   treeNodeFlags);

                    for (auto const& infoId: level0NodeInfoIds) {
                        auto nodeInfo               = getNodeInfo (infoId);
                        nodeInfo->meta.parentInfoId = currentNodeInfoId;
                    }
                    currentNodeInfoId++;
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
                 * | READY CAMERA TYPE LABELS                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                uint32_t cameraTypeCount = 10;
                for (uint32_t i = 0; i < cameraTypeCount; i++)
                    m_cameraTypeLabels.push_back (getCameraTypeString (static_cast <SandBox::e_cameraType> (i)));
                /* |------------------------------------------------------------------------------------------------|
                 * | READY UI IMAGE INFO POOL                                                                       |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto sceneInfo = getSceneInfo (uiSceneInfoId);
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    auto imageInfo       = getImageInfo (infoId, Core::TEXTURE_IMAGE);
                    auto descriptorSet   = ImGui_ImplVulkan_AddTexture (sceneInfo->resource.textureSampler,
                                                                        imageInfo->resource.imageView,
                                                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    std::string label    = "Info id [" + std::to_string (infoId) + "]";
                    /* Strip path to just the file name
                    */
                    size_t stripStart    = path.find_last_of ("\\/") + 1;
                    std::string fileName = path.substr (stripStart, path.length() - stripStart);

                    m_uiImageInfoPool[infoId].meta     = {label, fileName};
                    m_uiImageInfoPool[infoId].resource = {descriptorSet};
                }
                /* Note that, the ordering of data in the map must correspond to the ones in the vector. Hence, why we
                 * populate the vector after the map is completely populated
                */
                for (auto const& [key, val]: m_uiImageInfoPool)
                    m_diffuseTextureImageInfoIdLabels.push_back (val.meta.label);
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
                        ICON_FA_ANCHOR,         /* Transform    */
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
                 * | RIGHT PANEL - TRANSFORM                                                                        |
                 * |------------------------------------------------------------------------------------------------|
                */
                    if (m_selectedPropertyLabelIdx == TRANSFORM) {

                        if (nodeInfo->meta.type <= MODEL_TEXTURE_NODE) {

                            glm::vec3 position           = {0.0f, 0.0f, 0.0f};
                            glm::vec3 rotateAxis         = {0.0f, 0.0f, 0.0f};
                            glm::vec3 scale              = {0.0f, 0.0f, 0.0f};
                            float rotateAngleDeg         = 0.0f;
                            bool fieldDisable            = false;
                            bool writePending            = false;

                            if (nodeInfo->meta.type != MODEL_INSTANCE_NODE)
                                fieldDisable             = true;
                            else {
                                auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                                uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;
                                position                 = modelInfo->meta.instanceDatas[modelInstanceId].position;
                                rotateAxis               = modelInfo->meta.instanceDatas[modelInstanceId].rotateAxis;
                                scale                    = modelInfo->meta.instanceDatas[modelInstanceId].scale;
                                rotateAngleDeg           = modelInfo->meta.instanceDatas[modelInstanceId].rotateAngleDeg;
                            }

                            ImGui::Text              ("%s",            "Position");
                            if (createFloatTextField ("##positionX",   "X",     "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.x)     ||
                                createFloatTextField ("##positionY",   "Y",     "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.y)     ||
                                createFloatTextField ("##positionZ",   "Z",     "m",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, position.z))
                                writePending = true;

                            ImGui::Text              ("%s",            "Rotate axis");
                            if (createFloatTextField ("##rotateAxisX", "X",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAxis.x)   ||
                                createFloatTextField ("##rotateAxisY", "Y",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAxis.y)   ||
                                createFloatTextField ("##rotateAxisZ", "Z",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAxis.z)   ||
                                createFloatTextField ("##rotateAngle", "Angle", "deg",      g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, rotateAngleDeg))
                                writePending = true;

                            ImGui::Text              ("%s",            "Scale");
                            if (createFloatTextField ("##scaleX",      "X",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.x)        ||
                                createFloatTextField ("##scaleY",      "Y",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.y)        ||
                                createFloatTextField ("##scaleZ",      "Z",     "u",        g_styleSettings.precision,
                                                      fieldDisable,
                                                      g_styleSettings.size.inputFieldSmall, scale.z))
                                writePending = true;

                            {   /* Data write */
                                if (!fieldDisable && writePending) {
                                    auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                                    auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                                    uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                                    modelInfo->meta.instanceDatas[modelInstanceId].position       = position;
                                    modelInfo->meta.instanceDatas[modelInstanceId].rotateAxis     = rotateAxis;
                                    modelInfo->meta.instanceDatas[modelInstanceId].scale          = scale;
                                    modelInfo->meta.instanceDatas[modelInstanceId].rotateAngleDeg = rotateAngleDeg;
                                    createModelMatrix (parentNodeInfo->meta.coreInfoId, modelInstanceId);
                                }
                            }
                        }

                        else if (nodeInfo->meta.type <= CAMERA_INFO_ID_NODE) {

                            glm::vec3 position  = {0.0f, 0.0f, 0.0f};
                            glm::vec3 direction = {0.0f, 0.0f, 0.0f};
                            glm::vec3 upVector  = {0.0f, 0.0f, 0.0f};
                            bool fieldDisable   = false;
                            bool writePending   = false;

                            if (nodeInfo->meta.type != CAMERA_INFO_ID_NODE)
                                fieldDisable    = true;
                            else {
                                auto cameraInfo = getCameraInfo (nodeInfo->meta.coreInfoId);
                                position        = cameraInfo->meta.position;
                                direction       = cameraInfo->meta.direction;
                                upVector        = cameraInfo->meta.upVector;

                                if (!isCameraPropertyWritable())
                                    fieldDisable = true;
                            }

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

                        float fovDeg                        = 0.0f;
                        float nearPlane                     = 0.0f;
                        float farPlane                      = 0.0f;
                        uint32_t selectedCameraTypeLabelIdx = static_cast <uint32_t> (getCameraType());
                        bool fieldDisable                   = false;
                        bool writePending                   = false;

                        if (nodeInfo->meta.type != CAMERA_INFO_ID_NODE)
                            fieldDisable    = true;
                        else {
                            auto cameraInfo = getCameraInfo (nodeInfo->meta.coreInfoId);
                            fovDeg          = cameraInfo->meta.fovDeg;
                            nearPlane       = cameraInfo->meta.nearPlane;
                            farPlane        = cameraInfo->meta.farPlane;

                            if (!isCameraPropertyWritable())
                                fieldDisable = true;
                        }
                        /* To prevent showing post label, prefix it with '##'
                        */
                        createCombo          ("##cameraType",
                                              "Camera type",
                                              "##postLabel",
                                              m_cameraTypeLabels,
                                              false,
                                              g_styleSettings.size.inputFieldLarge,
                                              selectedCameraTypeLabelIdx);
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

                        createCheckBoxButton ("##overlay", "Metrics", "##postLabel", false, showMetricsOverlay);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - TEXTURE                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == TEXTURE) {
                        static uint32_t selectedDiffuseLabelIdx = 0;
                        bool fieldDisable                       = false;

                        if (nodeInfo->meta.type == MODEL_TEXTURE_NODE) {
                            uint32_t infoId         = nodeInfo->meta.coreInfoId;
                            /* Convert texture info id to label, and we use the label to find the offset to the labels
                             * vector. This provides us a common index to access both the map and the vector
                            */
                            std::string label       = "Info id [" + std::to_string (infoId) + "]";
                            selectedDiffuseLabelIdx = std::find (m_diffuseTextureImageInfoIdLabels.begin(),
                                                                 m_diffuseTextureImageInfoIdLabels.end(),
                                                                 label) - m_diffuseTextureImageInfoIdLabels.begin();
                            fieldDisable            = true;
                        }

                        createCombo ("##diffuse",
                                     "Diffuse",
                                     "##postLabel",
                                     m_diffuseTextureImageInfoIdLabels,
                                     fieldDisable,
                                     g_styleSettings.size.inputFieldLarge,
                                     selectedDiffuseLabelIdx);

                        /* Note that, we have already ensured that there is 1:1 correspondence between the map and
                         * the vector. This makes it possible to offset into the map using an index to the vector
                        */
                        auto iter = std::next (m_uiImageInfoPool.begin(), selectedDiffuseLabelIdx);
                        createImagePreview    (iter->second.resource.descriptorSet,
                                               g_styleSettings.size.image,
                                               g_styleSettings.color.border);
                        /* Image details
                        */
                        auto imageInfo        = getImageInfo   (iter->first, Core::TEXTURE_IMAGE);
                        std::string dims      = std::to_string (imageInfo->meta.width) +
                                                "x" +
                                                std::to_string (imageInfo->meta.height);
                        std::string sizeBytes = std::to_string (imageInfo->allocation.size) + " bytes";
                        std::string fileName  = iter->second.meta.fileName;

                        ImGui::Text ("%s", dims.c_str());
                        ImGui::Text ("%s", sizeBytes.c_str());
                        ImGui::Text ("%s", fileName.c_str());
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - LIGHT                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == LIGHT) {
                        /* [ X ] Pending implementation */
                        createCheckBoxButton ("##shadow", "Shadow", "##postLabel", true, showShadow);
                    }
                /* |------------------------------------------------------------------------------------------------|
                 * | RIGHT PANEL - PHYSICS                                                                          |
                 * |------------------------------------------------------------------------------------------------|
                */
                    else if (m_selectedPropertyLabelIdx == PHYSICS) {
                        /* [ X ] Pending implementation */
                        createCheckBoxButton ("##boundingBox", "Bounding box", "##postLabel", true, showBoundingBox);
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

            void cleanUp (const std::vector <uint32_t>& plotDataInfoIds) {
                for (auto const& [path, infoId]: getTextureImagePool())
                    deleteUIImageInfo (infoId);

                UITree::cleanUp (UINT32_MAX);

                for (auto const& infoId: plotDataInfoIds)
                    UIPlot::cleanUp (infoId);
                ImPlot::DestroyContext();
            }
    };
}   // namespace Gui
#endif  // UI_WINDOW_H