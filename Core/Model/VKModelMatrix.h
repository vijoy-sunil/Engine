#ifndef VK_MODEL_MATRIX_H
#define VK_MODEL_MATRIX_H

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "VKModelMgr.h"

namespace Core {
    class VKModelMatrix: protected virtual VKModelMgr {
        private:
            Log::Record* m_VKModelMatrixLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKModelMatrix (void) {
                m_VKModelMatrixLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKModelMatrix (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createModelMatrix (uint32_t modelInfoId, uint32_t modelInstanceId) {
                auto modelInfo = getModelInfo (modelInfoId);
                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKModelMatrixLog) << "Invalid model instance id "
                                                   << "[" << modelInstanceId << "]"
                                                   << "->"
                                                   << "[" << modelInfo->meta.instancesCount << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Invalid model instance id");
                }

                /* https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/#an-introduction-to-matrices
                 * Translation matrix looks like this
                 * 1 0 0 tx             vx          vx + tx
                 * 0 1 0 ty     *       vy      =   vx + ty
                 * 0 0 1 tz             vz          vz + tz
                 * 0 0 0 1              w           w
                 *
                 * Scaling matrix looks like this
                 * sx 0 0 0             vx          vx * sz
                 * 0 sy 0 0     *       vy      =   vy * sy
                 * 0 0 sz 0             vz          vz * sz
                 * 0 0 0  1             w           w
                 *
                 * Note that,
                 * If w == 1, then the vector (x,y,z,1) is a position in space
                 * If w == 0, then the vector (x,y,z,0) is a direction
                */

                /* Cumulating transformations, note that we perform scaling FIRST, and THEN the rotation, and THEN the
                 * translation. This is how matrix multiplication works
                */
                glm::vec3 position    = modelInfo->meta.instanceDatas[modelInstanceId].position;
                glm::vec3 rotateAxis  = modelInfo->meta.instanceDatas[modelInstanceId].rotateAxis;
                glm::vec3 scale       = modelInfo->meta.instanceDatas[modelInstanceId].scale;
                float rotateAngleDeg  = modelInfo->meta.instanceDatas[modelInstanceId].rotateAngleDeg;

                glm::mat4 modelMatrix = glm::translate (glm::mat4 (1.0f), position) *
                                        glm::rotate    (glm::mat4 (1.0f), glm::radians (rotateAngleDeg), rotateAxis) *
                                        glm::scale     (glm::mat4 (1.0f), scale);

                modelInfo->meta.instances[modelInstanceId].modelMatrix = modelMatrix;
            }
    };
}   // namespace Core
#endif  // VK_MODEL_MATRIX_H