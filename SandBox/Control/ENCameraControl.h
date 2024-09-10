#ifndef EN_CAMERA_CONTROL_H
#define EN_CAMERA_CONTROL_H

#include "../../Core/Model/VKModelMgr.h"
#include "../../Core/Scene/VKCameraMgr.h"
#include "../../Utils/UserInput.h"
#include "../Config/ENEnvConfig.h"

namespace SandBox {
    class ENCameraControl: protected virtual Core::VKModelMgr,
                           protected virtual Core::VKCameraMgr,
                           protected virtual Utils::UserInput {
        private:
            uint32_t m_deviceInfoId;
            uint32_t m_cameraInfoId;

            e_cameraType m_previousType;
            e_cameraType m_currentType;

            bool m_firstCursorEvent;

            float m_lastCursorX;
            float m_lastCursorY;
            float m_yawDeg;
            float m_pitchDeg;

            Log::Record* m_ENCameraControlLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

            void setCameraType (e_cameraType type) {
                m_previousType = m_currentType;
                m_currentType  = type;

                if (m_previousType != FREE_ROAM  && m_currentType == FREE_ROAM) {
                    auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                    readyCursorPositionCallBack     (deviceInfo->resource.window);
                    readyScrollOffsetCallBack       (deviceInfo->resource.window);
                }
                /* Delete mouse event callbacks if the camera is not in free roam mode
                */
                if (m_previousType == FREE_ROAM && m_currentType != FREE_ROAM) {
                    auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                    deleteCursorPositionCallBack    (deviceInfo->resource.window);
                    deleteScrollOffsetCallBack      (deviceInfo->resource.window);
                }
            }

            e_cameraType getCameraType (void) {
                return m_currentType;
            }

            float getYawDeg (glm::vec3 direction) {
                return glm::degrees (atan2 (direction.x, direction.z));
            }

            float getPitchDeg (glm::vec3 direction) {
                /* Note that, asin() function takes input in the range of [-1.0, 1.0], so we need to normalize the
                 * input vector before calling this function
                */
                return glm::degrees (asin (direction.y));
            }

            void switchToFreeRoam  (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (FREE_ROAM);
                /* Reset boolean whenever we switch to free roam mode
                */
                m_firstCursorEvent = true;
            }

            void switchToSpoiler   (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (SPOILER);
            }

            void switchToFpv       (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (FPV);
            }

            void switchToTopDown   (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (TOP_DOWN);
            }

            void switchToRightProfile (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (RIGHT_PROFILE);
            }

            void switchToLeftProfile (float deltaTime) {
                static_cast <void> (deltaTime);
                setCameraType      (LEFT_PROFILE);
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
                if (getCameraType() != FREE_ROAM)
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
                if (getCameraType() != FREE_ROAM)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position += g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (glm::cross (cameraInfo->meta.direction, 
                                                                         cameraInfo->meta.upVector));

                cameraInfo->meta.updateViewMatrix = true;
            }

            void moveBackward (float deltaTime) {
                if (getCameraType() != FREE_ROAM)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position -= g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (cameraInfo->meta.direction);

                cameraInfo->meta.updateViewMatrix = true;
            }

            void moveForward (float deltaTime) {
                if (getCameraType() != FREE_ROAM)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                cameraInfo->meta.position += g_cameraSettings.movementSpeed * deltaTime *
                                             glm::normalize (cameraInfo->meta.direction);

                cameraInfo->meta.updateViewMatrix = true;
            }

            /* Note that, the camera look around binding returns immediately if the camera state is not in free roam 
             * mode. This is to lock camera movement unless you are in free roam mode. However, the cursor position
             * callback is still triggered eventhough the binding function returns immediately. To prevent this, we
             * will clear the cursor position callback whenever we switch out of free roam mode
            */
            void lookAround (double xPosIn, double yPosIn) {
                if (getCameraType() != FREE_ROAM)
                    return;

                auto cameraInfo = getCameraInfo  (m_cameraInfoId);
                float xPos = static_cast <float> (xPosIn);
                float yPos = static_cast <float> (yPosIn);

                /* As soon as your cursor enters the window the callback function is called with an x and y position 
                 * equal to the location your cursor entered the screen from. This is often a position that is 
                 * significantly far away from the center of the screen, resulting in large offsets and thus a large 
                 * movement jump. We can circumvent this issue by defining a bool variable to check if this is the first
                 * time we receive cursor input. If it is the first time, we will set the cursor to the center of the 
                 * screen
                */
                if (m_firstCursorEvent) {
                    m_lastCursorX      = Core::g_windowSettings.width /2.0f;
                    m_lastCursorY      = Core::g_windowSettings.height/2.0f;
                    auto deviceInfo    = getDeviceInfo (m_deviceInfoId);

                    glfwSetCursorPos (deviceInfo->resource.window, m_lastCursorX, m_lastCursorY);

                    xPos               = m_lastCursorX;
                    yPos               = m_lastCursorY;
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
            void updateFov (double xOffsetIn, double yOffsetIn) {
                if (getCameraType() != FREE_ROAM)
                    return;

                auto cameraInfo = getCameraInfo (m_cameraInfoId);
                static_cast <void> (xOffsetIn);

                float yOffset            = static_cast <float> (yOffsetIn);
                cameraInfo->meta.fovDeg -= yOffset;

                if (cameraInfo->meta.fovDeg < g_cameraSettings.minFovDeg)
                    cameraInfo->meta.fovDeg = g_cameraSettings.minFovDeg;
                if (cameraInfo->meta.fovDeg > g_cameraSettings.maxFovDeg)
                    cameraInfo->meta.fovDeg = g_cameraSettings.maxFovDeg;

                cameraInfo->meta.updateProjectionMatrix = true;
            }

        public:
            ENCameraControl (void) {
                m_ENCameraControlLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~ENCameraControl (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyCameraControl (uint32_t deviceInfoId,
                                     uint32_t cameraInfoId,
                                     e_cameraType type) {

                m_deviceInfoId = deviceInfoId;
                m_cameraInfoId = cameraInfoId;

                setCameraType (type);
                m_yawDeg       = 0.0f;
                m_pitchDeg     = 0.0f;

                /* Note that, we are attempting to pass non-static class methods, which require an object instance to call
                 * them on, hence we use a lambda expression as shown below
                */
                createKeyEventBinding (g_cameraSettings.keyMap.freeRoam,        [this](float deltaTime) {
                    this->switchToFreeRoam              (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.spoiler,         [this](float deltaTime) {
                    this->switchToSpoiler               (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.fpv,             [this](float deltaTime) {
                    this->switchToFpv                   (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.topDown,         [this](float deltaTime) {
                    this->switchToTopDown               (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.rightProfile,    [this](float deltaTime) {
                    this->switchToRightProfile          (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.leftProfile,     [this](float deltaTime) {
                    this->switchToLeftProfile           (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.moveLeft,        [this](float deltaTime) {
                    this->moveLeft                      (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.moveRight,       [this](float deltaTime) {
                    this->moveRight                     (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.moveBackward,    [this](float deltaTime) {
                    this->moveBackward                  (deltaTime);
                });
                createKeyEventBinding (g_cameraSettings.keyMap.moveForward,     [this](float deltaTime) {
                    this->moveForward                   (deltaTime);
                });

                createMouseEventBinding (Utils::CURSOR_POSITION,                [this](float xPos, float yPos) {
                    this->lookAround                    (xPos, yPos);
                });
                createMouseEventBinding (Utils::SCROLL_OFFSET,                  [this](float xOffset, float yOffset) {
                    this->updateFov                     (xOffset, yOffset);
                });
            }

            void updateCameraState (uint32_t modelInfoId, uint32_t modelInstanceId) {
                if (getCameraType() == FREE_ROAM)
                    return;

                auto modelInfo   = getModelInfo  (modelInfoId);
                auto cameraInfo  = getCameraInfo (m_cameraInfoId);
                auto currentType = getCameraType();

                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_ENCameraControlLog) << "Invalid model instance id " 
                                                     << "[" << modelInstanceId << "]"
                                                     << "->"
                                                     << "[" << modelInfo->meta.instancesCount << "]"
                                                     << std::endl; 
                    throw std::runtime_error ("Invalid model instance id");
                }

                glm::mat4 modelMatrix      = modelInfo->meta.instances[modelInstanceId].modelMatrix;                
                cameraInfo->meta.position  = glm::vec3 (modelMatrix * 
                                             glm::vec4 (g_cameraStateInfoPool[currentType].position, 1.0));

                cameraInfo->meta.direction = glm::vec3 (modelMatrix * 
                                             glm::vec4 (g_cameraStateInfoPool[currentType].direction, 1.0)) -
                                             cameraInfo->meta.position;

                cameraInfo->meta.fovDeg    = g_cameraStateInfoPool[currentType].fovDeg;

                /* [ X ] Only recompute camera matrices if it has been updated
                */
                cameraInfo->meta.updateViewMatrix       = true;
                cameraInfo->meta.updateProjectionMatrix = true;
            }
    };
}   // namespace SandBox
#endif  // EN_CAMERA_CONTROL_H