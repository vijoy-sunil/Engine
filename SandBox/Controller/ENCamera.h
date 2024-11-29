#ifndef EN_CAMERA_H
#define EN_CAMERA_H

#include "../../Core/Model/VKModelMgr.h"
#include "../../Core/Scene/VKCameraMgr.h"
#include "../../Gui/UIInput.h"
#include "../../Gui/UIUtil.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENCamera: protected virtual Core::VKModelMgr,
                    protected virtual Core::VKCameraMgr,
                    protected virtual Gui::UIInput,
                    protected virtual Gui::UIUtil {
        private:
            uint32_t m_deviceInfoId;
            uint32_t m_cameraInfoId;

            e_cameraType m_previousType;
            e_cameraType m_currentType;

            bool m_firstCursorEvent;
            bool m_modelTransformRemoved;

            float m_lastCursorX;
            float m_lastCursorY;
            float m_yawDeg;
            float m_pitchDeg;

            Log::Record* m_ENCameraLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            float getYawDeg (glm::vec3 direction) {
                return glm::degrees (atan2 (direction.x, direction.z));
            }

            float getPitchDeg (glm::vec3 direction) {
                /* Note that, asin() function takes input in the range of [-1.0, 1.0], so we need to normalize the
                 * input vector before calling this function
                */
                return glm::degrees (asin (direction.y));
            }

            void switchToSpoiler (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (SPOILER);
            }

            void switchToLeftProfile (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (LEFT_PROFILE);
            }

            void switchToReverse (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (REVERSE);
            }

            void switchToRightProfile (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (RIGHT_PROFILE);
            }

            void switchToRearAxle (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (REAR_AXLE);
            }

            void switchToTopDown (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (TOP_DOWN);
            }

            void switchToFrontAxle (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (FRONT_AXLE);
            }

            void switchToDroneLock (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (DRONE_LOCK);
            }

            void switchToDroneFollow (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (DRONE_FOLLOW);
            }

            void switchToDroneFly (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                setCameraType      (DRONE_FLY);
            }

            /* Whenever we press one of the camera movement keys, the camera's position is updated accordingly. If we
             * want to move forward or backwards we add or subtract the direction vector from the position vector scaled
             * by some speed value. If we want to move sideways we do a cross product to create a right vector and we
             * move along the right vector accordingly. This creates the familiar strafe effect when using the camera
             *
             * Graphics applications and games usually keep track of a delta time variable that stores the time it took
             * to render the last frame. We multiply the movement speed with this delta time value. The result is that
             * when we have a large delta time in a frame, meaning that the last frame took longer than average, the
             * velocity for that frame will also be a bit higher to balance it all out. When using this approach it does
             * not matter if you have a very fast or slow pc, the velocity of the camera will be balanced out accordingly
             * so each user will have the same experience
            */
            void moveLeft (float deltaTime) {
                if (isKeyBoardCapturedByUI() || getCameraType() != DRONE_FLY)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                /* Note that we normalize the resulting right vector. Without normalizing this vector, the resulting cross
                 * product may return differently sized vectors based on the input vector magnitude resulting in slow or
                 * fast movement based on the camera's orientation instead of at a consistent movement speed
                */
                cameraInfo->meta.position -= g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (glm::cross (cameraInfo->meta.direction,
                                                                         cameraInfo->meta.upVector));

                cameraInfo->meta.updateViewMatrix = true;
            }

            void moveRight (float deltaTime) {
                if (isKeyBoardCapturedByUI() || getCameraType() != DRONE_FLY)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position += g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (glm::cross (cameraInfo->meta.direction,
                                                                         cameraInfo->meta.upVector));

                cameraInfo->meta.updateViewMatrix = true;
            }

            void moveBackward (float deltaTime) {
                if (isKeyBoardCapturedByUI() || getCameraType() != DRONE_FLY)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position -= g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (cameraInfo->meta.direction);

                cameraInfo->meta.updateViewMatrix = true;
            }

            void moveForward (float deltaTime) {
                if (isKeyBoardCapturedByUI() || getCameraType() != DRONE_FLY)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position += g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (cameraInfo->meta.direction);

                cameraInfo->meta.updateViewMatrix = true;
            }

            /* Note that, the direction update binding returns immediately if the camera type is not in drone fly
             * mode. This is to lock camera movement unless you are in drone fly mode. However, the cursor position
             * callback is still triggered even though the binding function returns immediately. To prevent this, we
             * will clear the cursor position callback whenever we switch out of drone fly mode
            */
            void setDirection (double xPosIn, double yPosIn) {
                if (getCameraType() != DRONE_FLY)
                    return;

                auto cameraInfo = getCameraInfo  (m_cameraInfoId);
                float xPos = static_cast <float> (xPosIn);
                float yPos = static_cast <float> (yPosIn);

                /* As soon as your cursor enters the window the callback function is called with an x and y position
                 * equal to the location your cursor entered the screen from. This is often a position that is
                 * significantly far away from the center of the screen, resulting in large offsets and thus a large
                 * movement jump. We can circumvent this issue by defining a bool variable to check if this is the first
                 * time we receive cursor input. If it is the first time, we update the initial mouse positions to the
                 * new x and y values. The resulting mouse movements will then use the newly entered mouse's position
                 * coordinates to calculate the offsets
                */
                if (m_firstCursorEvent) {
                    m_lastCursorX      = xPos;
                    m_lastCursorY      = yPos;
                    m_firstCursorEvent = false;
                }

                float xOffset = xPos - m_lastCursorX;
                float yOffset = m_lastCursorY - yPos;
                m_lastCursorX = xPos;
                m_lastCursorY = yPos;

                /* Note that we multiply the offset values by a sensitivity value. If we omit this multiplication the
                 * movement would be way too strong
                */
                xOffset      *= g_cameraSettings.sensitivity;
                yOffset      *= g_cameraSettings.sensitivity;
                /* Next, add the offset values to the pitch and yaw values
                */
                glm::vec3 direction = glm::normalize (cameraInfo->meta.direction);
                m_yawDeg            = getYawDeg      (direction) + xOffset;
                m_pitchDeg          = getPitchDeg    (direction) - yOffset;

                /* Next, we'd like to add some constraints to the camera so users won't be able to make weird camera
                 * movements (also causes a LookAt() flip once direction vector is parallel to the world up direction).
                 * The pitch needs to be constrained in such a way that users won't be able to look higher than ~90
                 * degrees (at 90 degrees we get the LookAt() flip) and also not below - ~90 degrees. This ensures the
                 * user will be able to look up to the sky or below to his feet but not further
                */
                if (m_pitchDeg < g_cameraSettings.minPitchDeg) m_pitchDeg = g_cameraSettings.minPitchDeg;
                if (m_pitchDeg > g_cameraSettings.maxPitchDeg) m_pitchDeg = g_cameraSettings.maxPitchDeg;

                /* Finally, calculate the actual direction vector
                */
                direction.x = sin (glm::radians (m_yawDeg)) * cos (glm::radians (m_pitchDeg));
                direction.y = sin (glm::radians (m_pitchDeg));
                direction.z = cos (glm::radians (m_yawDeg)) * cos (glm::radians (m_pitchDeg));

                cameraInfo->meta.direction        = direction;
                cameraInfo->meta.updateViewMatrix = true;
            }

            /* Field of view or fov largely defines how much we can see of the scene. When the field of view becomes
             * smaller, the scene's projected space gets smaller. This smaller space is projected over the same NDC,
             * giving the illusion of zooming in
            */
            void setFov (double xOffsetIn, double yOffsetIn) {
                if (getCameraType() != DRONE_FLY)
                    return;

                static_cast <void> (xOffsetIn);

                auto cameraInfo          = getCameraInfo (m_cameraInfoId);
                float yOffset            = static_cast <float> (yOffsetIn);
                cameraInfo->meta.fovDeg -= yOffset;

                if (cameraInfo->meta.fovDeg < g_cameraSettings.minFovDeg)
                    cameraInfo->meta.fovDeg = g_cameraSettings.minFovDeg;
                if (cameraInfo->meta.fovDeg > g_cameraSettings.maxFovDeg)
                    cameraInfo->meta.fovDeg = g_cameraSettings.maxFovDeg;

                cameraInfo->meta.updateProjectionMatrix = true;
            }

        public:
            ENCamera (void) {
                m_ENCameraLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~ENCamera (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyCameraController (uint32_t deviceInfoId,
                                        uint32_t cameraInfoId,
                                        e_cameraType type) {

                m_deviceInfoId = deviceInfoId;
                m_cameraInfoId = cameraInfoId;
                m_currentType  = UNDEFINED;

                setCameraType (type);
                /* Note that, we are attempting to pass non-static class methods, which require an object instance to call
                 * them on, hence we use a lambda expression as shown below
                */
                createKeyEventBinding (g_keyMapSettings.spoiler,                [this](float deltaTime) {
                    this->switchToSpoiler               (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.leftProfile,            [this](float deltaTime) {
                    this->switchToLeftProfile           (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.reverse,                [this](float deltaTime) {
                    this->switchToReverse               (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.rightProfile,           [this](float deltaTime) {
                    this->switchToRightProfile          (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.rearAxle,               [this](float deltaTime) {
                    this->switchToRearAxle              (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.topDown,                [this](float deltaTime) {
                    this->switchToTopDown               (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.frontAxle,              [this](float deltaTime) {
                    this->switchToFrontAxle             (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.droneLock,              [this](float deltaTime) {
                    this->switchToDroneLock             (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.droneFollow,            [this](float deltaTime) {
                    this->switchToDroneFollow           (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.droneFly,               [this](float deltaTime) {
                    this->switchToDroneFly              (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.moveLeft,               [this](float deltaTime) {
                    this->moveLeft                      (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.moveRight,              [this](float deltaTime) {
                    this->moveRight                     (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.moveBackward,           [this](float deltaTime) {
                    this->moveBackward                  (deltaTime);
                });
                createKeyEventBinding (g_keyMapSettings.moveForward,            [this](float deltaTime) {
                    this->moveForward                   (deltaTime);
                });

                createMouseEventBinding (Core::CURSOR_POSITION,                 [this](float xPos, float yPos) {
                    this->setDirection                  (xPos, yPos);
                });
                createMouseEventBinding (Core::SCROLL_OFFSET,                   [this](float xOffset, float yOffset) {
                    this->setFov                        (xOffset, yOffset);
                });
            }

            bool isModelTransformRemoved (void) {
                return m_modelTransformRemoved;
            }

            void setModelTransformRemoved (bool val) {
                m_modelTransformRemoved = val;
            }

            e_cameraType getCameraType (void) {
                return m_currentType;
            }

            void setCameraType (e_cameraType type) {
                m_previousType = m_currentType;
                m_currentType  = type;
                /* Note that, we can only switch to drone follow if we are in either of the drone modes. Drone follow
                 * mode would not make sense otherwise. We need to revert previous and current camera type before
                 * returning
                */
                if ((m_previousType != DRONE_LOCK && m_previousType != DRONE_FLY) &&
                     m_currentType  == DRONE_FOLLOW) {

                    m_currentType   = m_previousType;
                    m_previousType  = UNDEFINED;
                    return;
                }
                /* Note that, when we switch to drone follow mode we need to remove the model transformation done to the
                 * camera vectors before using them. The boolean ensures that the removal/inverse operation is only done
                 * once after the switch happens
                */
                if (m_previousType != DRONE_FOLLOW && m_currentType == DRONE_FOLLOW)
                    setModelTransformRemoved (false);
                /* Note that, we need to reinstall application callbacks with imgui using _RestoreCallbacks() and
                 * _InstallCallbacks() methods so that imgui can chain glfw callbacks
                */
                if (m_previousType != DRONE_FLY  && m_currentType == DRONE_FLY) {
                    auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                    ImGui_ImplGlfw_RestoreCallbacks (deviceInfo->resource.window);

                    readyCursorPositionCallback     (m_deviceInfoId);
                    readyScrollOffsetCallback       (m_deviceInfoId);

                    ImGui_ImplGlfw_InstallCallbacks (deviceInfo->resource.window);
                    disableMouseInputsToUI();

                    m_firstCursorEvent = true;
                }
                /* Delete mouse event callbacks if the camera is not in drone fly mode
                */
                if (m_previousType == DRONE_FLY && m_currentType != DRONE_FLY) {
                    auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                    ImGui_ImplGlfw_RestoreCallbacks (deviceInfo->resource.window);

                    deleteCursorPositionCallback    (m_deviceInfoId);
                    deleteScrollOffsetCallback      (m_deviceInfoId);

                    ImGui_ImplGlfw_InstallCallbacks (deviceInfo->resource.window);
                    enableMouseInputsToUI();
                }
            }

            void setCameraState (uint32_t modelInfoId, uint32_t modelInstanceId) {
                if (getCameraType() == DRONE_LOCK || getCameraType() == DRONE_FLY)
                    return;

                auto modelInfo   = getModelInfo  (modelInfoId);
                auto cameraInfo  = getCameraInfo (m_cameraInfoId);
                auto currentType = getCameraType();

                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_ENCameraLog) << "Invalid model instance id "
                                              << "[" << modelInstanceId << "]"
                                              << "->"
                                              << "[" << modelInfo->meta.instancesCount << "]"
                                              << std::endl;
                    throw std::runtime_error ("Invalid model instance id");
                }

                /* Instead of using the pre computed model matrix, we will recreate it using a unit scale vector to
                 * make the transformation of camera vectors independent of the model scale
                */
                glm::vec3 position       = modelInfo->meta.transformDatas[modelInstanceId].position;
                glm::vec3 rotateAngleDeg = modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg;

                glm::mat4 modelMatrix    = glm::translate (glm::mat4 (1.0f), position)    *
                                           glm::rotate    (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.z),
                                                           glm::vec3 (0.0f,  0.0f, 1.0f)) *     /* Roll  */
                                           glm::rotate    (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.y),
                                                           glm::vec3 (0.0f, -1.0f, 0.0f)) *     /* Yaw   */
                                           glm::rotate    (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.x),
                                                           glm::vec3 (1.0f,  0.0f, 0.0f));      /* Pitch */
                /* Why do we need to remove the model transformation that was done to the camera vectors before using
                 * them in drone follow mode? The reason is, when we switch to drone follow mode, we use the camera
                 * vectors from the previous mode, which have already been multiplied by the model matrix. But, what
                 * we need is camera vectors that assumes the model is at the origin (just like the vectors stored in
                 * the camera info pool). Once we have obtained them, we can then easily apply the model transformation
                 * to it so that it follows the target
                 *
                 * Note that, the boolean is unset when we switch to drone follow mode, and, when we write new data to
                 * camera vectors via ui
                */
                if (currentType == DRONE_FOLLOW && !isModelTransformRemoved()) {
                    glm::mat4 inverseModelMatrix                 = glm::inverse (modelMatrix);
                    g_cameraStateInfoPool[currentType].position  = glm::vec3    (inverseModelMatrix *
                                                                   glm::vec4    (cameraInfo->meta.position, 1.0f));

                    g_cameraStateInfoPool[currentType].direction = glm::vec3    (inverseModelMatrix *
                                                                   glm::vec4    (cameraInfo->meta.direction +
                                                                                 cameraInfo->meta.position, 1.0f));

                    g_cameraStateInfoPool[currentType].fovDeg    = cameraInfo->meta.fovDeg;
                    setModelTransformRemoved (true);
                }

                cameraInfo->meta.position  = glm::vec3 (modelMatrix *
                                             glm::vec4 (g_cameraStateInfoPool[currentType].position,  1.0f));

                cameraInfo->meta.direction = glm::vec3 (modelMatrix *
                                             glm::vec4 (g_cameraStateInfoPool[currentType].direction, 1.0f)) -
                                             cameraInfo->meta.position;

                cameraInfo->meta.fovDeg    = g_cameraStateInfoPool[currentType].fovDeg;

                cameraInfo->meta.updateViewMatrix       = true;
                cameraInfo->meta.updateProjectionMatrix = true;
            }
    };
}   // namespace SandBox
#endif  // EN_CAMERA_H