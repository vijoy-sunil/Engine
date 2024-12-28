#ifndef UI_VIEW_H
#define UI_VIEW_H

#include "../../Core/Model/VKModelMatrix.h"
#include "../Wrapper/UIPrimitive.h"
#include "../Wrapper/UITree.h"
#include "../../SandBox/Controller/ENCamera.h"
#include "../../SandBox/ENLogHelper.h"

namespace Gui {
    class UIView: protected virtual Core::VKModelMatrix,
                  protected virtual UIPrimitive,
                  protected virtual UITree,
                  protected virtual SandBox::ENCamera {
        private:
            struct UIViewInfo {
                struct CameraFocus {
                    uint32_t modelInfoId;
                    uint32_t modelInstanceId;
                } cameraFocus;

                uint32_t activeCameraInfoId;
            } m_uiViewInfo;

            uint32_t m_cameraAnchorInfoId;
            std::vector <std::string> m_cameraInfoIdLabels;
            std::vector <std::string> m_cameraTypeLabels;

            Log::Record* m_UIViewLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIView (void) {
                m_UIViewLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIView (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyContent (uint32_t cameraAnchorInfoId) {
                /* Note that, we need to save the camera anchor info id in order to enable changing the active camera
                 * irrespective of the node selected
                */
                m_cameraAnchorInfoId = cameraAnchorInfoId;
                auto anchorInfo      = getModelInfo (m_cameraAnchorInfoId);
                /* Ready camera info id labels
                */
                for (uint32_t i = 0; i < anchorInfo->meta.instancesCount; i++) {
                    std::string label = "Info id [" + std::to_string (i) + "]";
                    m_cameraInfoIdLabels.push_back (label);
                }
                /* Ready camera type labels
                */
                uint32_t cameraTypeCount = 10;
                for (uint32_t i = 0; i < cameraTypeCount; i++)
                    m_cameraTypeLabels.push_back (getCameraTypeString (static_cast <SandBox::e_cameraType> (i)));
            }

            void createContent (uint32_t nodeInfoId,
                                bool& showMetricsOverlay) {
                auto nodeInfo = getNodeInfo (nodeInfoId);

                /* Default data */
                uint32_t selectedCameraInfoIdLabelIdx = m_uiViewInfo.activeCameraInfoId;
                uint32_t selectedCameraTypeLabelIdx   = static_cast <uint32_t> (getCameraType());

                float fovDeg                          = 0.0f;
                float nearPlane                       = 0.0f;
                float farPlane                        = 0.0f;

                bool setFocus                         = false;
                bool hideRender                       = false;
                bool fieldDisable                     = false;
                bool writePending                     = false;

                if ((nodeInfo->meta.type & CAMERA_NODE) && (nodeInfo->meta.type & INSTANCE_NODE)) {
                    /* Read data */
                    auto cameraInfo  = getCameraInfo (nodeInfo->meta.coreInfoId);
                    fovDeg           = cameraInfo->meta.fovDeg;
                    nearPlane        = cameraInfo->meta.nearPlane;
                    farPlane         = cameraInfo->meta.farPlane;

                    if ((nodeInfo->state.locked) || (!isCameraPropertyWritable()))
                        fieldDisable = true;
                }
                else
                    fieldDisable     = true;

                /* Show data */
                if (createCombo ("##cameraActive",
                                 "Camera active",
                                 "##postLabelCameraActive",
                                 m_cameraInfoIdLabels,
                                 false,
                                 g_styleSettings.size.inputFieldLarge,
                                 selectedCameraInfoIdLabelIdx)) {
                    /* Data write */
                    auto anchorInfo            = getModelInfo (m_cameraAnchorInfoId);
                    uint32_t anchorInstanceId  = selectedCameraInfoIdLabelIdx;
                    auto position              = anchorInfo->meta.transformDatas[anchorInstanceId].position;
                    auto rotateAngleDeg        = anchorInfo->meta.transformDatas[anchorInstanceId].rotateAngleDeg;
                    /* Match the selected camera's pose with the anchor instance's pose. Note that, usually when switching
                     * to the drone camera types, we use the previous type's values for fov, etc. as the initial values.
                     * However, since we are setting the initial camera type to drone lock, we will need to manually set
                     * them as shown below
                    */
                    auto cameraInfo            = getCameraInfo (anchorInstanceId);
                    cameraInfo->meta.position  = position;

                    float yawDeg               = -rotateAngleDeg.y;
                    float pitchDeg             = -rotateAngleDeg.x;
                    cameraInfo->meta.direction = getDirectionVector (yawDeg, pitchDeg);
                    cameraInfo->meta.fovDeg    = 80.0f;
                    /* Note that, upon chainging the active camera, we are setting the camera type to drone lock type
                     * which inherently doesn't set the boolean to update the camera matrices. Hence, why we need to
                     * explicitly set them
                    */
                    cameraInfo->meta.updateViewMatrix       = true;
                    cameraInfo->meta.updateProjectionMatrix = true;
                    /* Update active camera info id
                    */
                    m_uiViewInfo.activeCameraInfoId         = anchorInstanceId;
                    setCameraActive (anchorInstanceId, SandBox::DRONE_LOCK);
                }

                /* Show data */
                if (createCombo ("##cameraType",
                                 "Camera type",
                                 "##postLabelCameraType",
                                 m_cameraTypeLabels,
                                 false,
                                 g_styleSettings.size.inputFieldLarge,
                                 selectedCameraTypeLabelIdx)) {
                    /* Data write */
                    setCameraType (static_cast <SandBox::e_cameraType> (selectedCameraTypeLabelIdx));
                }

                /* Show data */
                createSeparatorText      ("Projection");
                if (createFloatTextField ("##fov",
                                          "FOV",
                                          "deg",
                                          g_styleSettings.precision,
                                          fieldDisable,
                                          g_styleSettings.size.inputFieldSmall,
                                          fovDeg)   ||

                    createFloatTextField ("##nearPlane",
                                          "Near plane",
                                          "m",
                                          g_styleSettings.precision,
                                          fieldDisable,
                                          g_styleSettings.size.inputFieldSmall,
                                          nearPlane)    ||

                    createFloatTextField ("##farPlane",
                                          "Far plane",
                                          "m",
                                          g_styleSettings.precision,
                                          fieldDisable,
                                          g_styleSettings.size.inputFieldSmall,
                                          farPlane))
                    writePending = true;

                /* Data write */
                if (!fieldDisable && writePending) {
                    auto cameraInfo                         = getCameraInfo (nodeInfo->meta.coreInfoId);
                    cameraInfo->meta.fovDeg                 = fovDeg;
                    cameraInfo->meta.nearPlane              = nearPlane;
                    cameraInfo->meta.farPlane               = farPlane;
                    cameraInfo->meta.updateProjectionMatrix = true;
                    setModelTransformRemoved (false);
                }

                /* Reset data */
                fieldDisable = false;
                /* Note that, we want to prevent the camera from setting focus on itself, hence why the 'if' condition
                */
                if (!(nodeInfo->meta.type & CAMERA_NODE) && (nodeInfo->meta.type & INSTANCE_NODE)) {
                    /* Read data */
                    auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                    uint32_t modelInfoId     = parentNodeInfo->meta.coreInfoId;
                    uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                    if ((m_uiViewInfo.cameraFocus.modelInfoId     == modelInfoId) &&
                        (m_uiViewInfo.cameraFocus.modelInstanceId == modelInstanceId))
                        setFocus             = true;

                    if (!isCameraFocusWritable())
                        fieldDisable         = true;
                }
                else
                    fieldDisable             = true;

                /* Show data */
                createSeparatorText  ("Misc");
                createCheckBoxButton ("##setFocus",
                                      "Set focus",
                                      "##postLabelSetFocus",
                                      fieldDisable,
                                      setFocus);
                /* Write data */
                if (!fieldDisable) {
                    auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                    uint32_t modelInfoId     = parentNodeInfo->meta.coreInfoId;
                    uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                    if (setFocus) {
                        m_uiViewInfo.cameraFocus.modelInfoId     = modelInfoId;
                        m_uiViewInfo.cameraFocus.modelInstanceId = modelInstanceId;
                    }
                }

                /* Reset data */
                fieldDisable = false;
                if (nodeInfo->meta.type & INSTANCE_NODE) {
                    /* Read data */
                    auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                    auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                    uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                    if (modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier == 0.0f)
                        hideRender           = true;
                    else
                        hideRender           = false;
                }
                else
                    fieldDisable             = true;

                /* Show data */
                const char* preLabel = (nodeInfo->meta.type & ANCHOR_NODE) ? "Hide anchor": "Hide model";
                createCheckBoxButton ("##hideRender",
                                      preLabel,
                                      "##postLabelHideRender",
                                      fieldDisable,
                                      hideRender);
                /* Write data */
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

                /* Show data */
                createSeparatorText  ("Overlay");
                createCheckBoxButton ("##metrics",
                                      "Metrics",
                                      "##postLabelMetrics",
                                      false,
                                      showMetricsOverlay);
            }

            UIViewInfo* getUIViewInfo (void) {
                return &m_uiViewInfo;
            }
    };
}   // namespace Gui
#endif  // UI_VIEW_H