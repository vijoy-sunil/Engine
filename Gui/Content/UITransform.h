#ifndef UI_TRANSFORM_H
#define UI_TRANSFORM_H

#include "../../Core/Model/VKModelMatrix.h"
#include "../../Core/Scene/VKLightMgr.h"
#include "../Wrapper/UIPrimitive.h"
#include "../Wrapper/UITree.h"
#include "../../SandBox/Controller/ENCamera.h"

namespace Gui {
    class UITransform: protected virtual Core::VKModelMatrix,
                       protected virtual Core::VKLightMgr,
                       protected virtual UIPrimitive,
                       protected virtual UITree,
                       protected virtual SandBox::ENCamera {
        private:
            Log::Record* m_UITransformLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UITransform (void) {
                m_UITransformLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UITransform (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createContent (uint32_t nodeInfoId) {
                auto nodeInfo = getNodeInfo (nodeInfoId);
                
                /* Transform model/light */
                if ((nodeInfo->meta.type & MODEL_NODE) || (nodeInfo->meta.type & LIGHT_NODE)) {
                    /* Default data */
                    glm::vec3 position           = glm::vec3 (0.0f);
                    glm::vec3 scale              = glm::vec3 (0.0f);
                    glm::vec3 rotateAngleDeg     = glm::vec3 (0.0f);
                    bool fieldDisable            = false;
                    bool writePending            = false;

                    if (nodeInfo->meta.type & INSTANCE_NODE) {
                        /* Read data */
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

                    /* Show data */
                    createSeparatorText      ("Position");
                    if (createFloatTextField ("##positionX",
                                              "X",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.x)   ||

                        createFloatTextField ("##positionY",
                                              "Y",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.y)   ||

                        createFloatTextField ("##positionZ",
                                              "Z",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.z))
                        writePending = true;

                    createSeparatorText      ("Rotate angle");
                    if (createFloatTextField ("##rotateAngleX",
                                              "X",
                                              "deg",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              rotateAngleDeg.x) ||

                        createFloatTextField ("##rotateAngleY", "Y",
                                              "deg",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              rotateAngleDeg.y) ||

                        createFloatTextField ("##rotateAngleZ",
                                              "Z",
                                              "deg",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              rotateAngleDeg.z))
                        writePending = true;

                    createSeparatorText      ("Scale");
                    if (createFloatTextField ("##scaleX",
                                              "X",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              scale.x)  ||

                        createFloatTextField ("##scaleY",
                                              "Y",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              scale.y)  ||

                        createFloatTextField ("##scaleZ",
                                              "Z",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              scale.z))
                        writePending = true;

                    /* Write data */
                    if (!fieldDisable && writePending) {
                        auto parentNodeInfo      = getNodeInfo  (nodeInfo->meta.parentInfoId);
                        auto modelInfo           = getModelInfo (parentNodeInfo->meta.coreInfoId);
                        uint32_t modelInstanceId = nodeInfo->meta.coreInfoId;

                        modelInfo->meta.transformDatas[modelInstanceId].position       = position;
                        modelInfo->meta.transformDatas[modelInstanceId].scale          = scale;
                        modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg = rotateAngleDeg;
                        createModelMatrix (parentNodeInfo->meta.coreInfoId, modelInstanceId);

                        /* Set light position and direction from its corresponding anchor instance. Note, that we need to
                         * check if the selected node is a light instance node before we copy the anchor instance pose
                        */
                        if (nodeInfo->meta.type & LIGHT_NODE) {
                            auto lightInfo           = getLightInfo (parentNodeInfo->meta.coreInfoId);
                            uint32_t lightInstanceId = modelInstanceId;
                            float yawDeg             = -rotateAngleDeg.y;
                            float pitchDeg           = -rotateAngleDeg.x;

                            lightInfo->meta.instances[lightInstanceId].position  = position;
                            lightInfo->meta.instances[lightInstanceId].direction = getDirectionVector
                                                                                   (yawDeg, pitchDeg);
                        }
                    }
                }
                /* Transform camera */
                else if (nodeInfo->meta.type & CAMERA_NODE) {
                    /* Default data */
                    glm::vec3 position   = glm::vec3 (0.0f);
                    glm::vec3 direction  = glm::vec3 (0.0f);
                    glm::vec3 upVector   = glm::vec3 (0.0f);
                    bool fieldDisable    = false;
                    bool writePending    = false;

                    if (nodeInfo->meta.type & INSTANCE_NODE) {
                        /* Read data */
                        auto cameraInfo  = getCameraInfo (nodeInfo->meta.coreInfoId);
                        position         = cameraInfo->meta.position;
                        direction        = cameraInfo->meta.direction;
                        upVector         = cameraInfo->meta.upVector;

                        if ((nodeInfo->state.locked) || (!isCameraPropertyWritable()))
                            fieldDisable = true;
                    }
                    else
                        fieldDisable     = true;

                    /* Show data */
                    createSeparatorText      ("Position");
                    if (createFloatTextField ("##positionX",
                                              "X",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.x)   ||

                        createFloatTextField ("##positionY",
                                              "Y",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.y)   ||

                        createFloatTextField ("##positionZ",
                                              "Z",
                                              "m",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              position.z))
                        writePending = true;

                    createSeparatorText      ("Direction");
                    if (createFloatTextField ("##directionX",
                                              "X",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              direction.x)  ||

                        createFloatTextField ("##directionY",
                                              "Y",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              direction.y)  ||

                        createFloatTextField ("##directionZ",
                                              "Z",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              direction.z))
                        writePending = true;

                    createSeparatorText      ("Up vector");
                    if (createFloatTextField ("##upVectorX",
                                              "X",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              upVector.x)   ||

                        createFloatTextField ("##upVectorY",
                                              "Y",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              upVector.y)   ||

                        createFloatTextField ("##upVectorZ",
                                              "Z",
                                              "u",
                                              g_styleSettings.precision,
                                              fieldDisable,
                                              g_styleSettings.size.inputFieldSmall,
                                              upVector.z))
                        writePending = true;

                    /* Write data */
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
    };
}   // namespace Gui
#endif  // UI_TRANSFORM_H