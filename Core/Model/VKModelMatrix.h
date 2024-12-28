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

            /* What is a normal matrix?
             * Note that, the lighting calculations in the fragment shader are all done in world space, so we need to
             * transform the normal vectors to world space coordinates as well. However, it's not as simple as simply
             * multiplying it with a model matrix
             *
             * First of all, normal vectors are only direction vectors and do not represent a specific position in space.
             * Second, normal vectors do not have a homogeneous coordinate (the w component of a vertex position). This
             * means that translations should not have any effect on the normal vectors. So if we want to multiply the
             * normal vectors with a model matrix we want to remove the translation part of the matrix by taking the
             * upper-left 3x3 matrix of the model matrix (note that we could also set the w component of a normal vector
             * to 0 and multiply with the 4x4 matrix)
             *
             * Third, if the model matrix would perform a non-uniform scale, the vertices would be changed in such a way
             * that the normal vector is not perpendicular to the surface anymore. Whenever we apply a non-uniform scale
             * (note: a uniform scale only changes the normal's magnitude, not its direction, which is easily fixed by
             * normalizing it) the normal vectors are not perpendicular to the corresponding surface anymore which
             * distorts the lighting. The trick of fixing this behavior is to use a different model matrix specifically
             * tailored for normal vectors. This matrix is called the normal matrix
             *
             * The normal matrix is defined as 'the transpose of the inverse of the upper-left 3x3 part of the model
             * matrix'. In the vertex shader we can generate the normal matrix by using the inverse and transpose
             * functions in the vertex shader that work on any matrix type. However, inversing matrices is a costly
             * operation for shaders, so wherever possible we try to avoid doing inverse operations since they have to
             * be done on each vertex of your scene. For an efficient application we want to calculate the normal matrix
             * on the CPU and send it to the shaders before drawing (just like the model matrix)
            */
            void createNormalMatrix (uint32_t modelInfoId, uint32_t modelInstanceId) {
                auto modelInfo = getModelInfo (modelInfoId);
                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKModelMatrixLog) << "Invalid model instance id "
                                                   << "[" << modelInstanceId << "]"
                                                   << "->"
                                                   << "[" << modelInfo->meta.instancesCount << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Invalid model instance id");
                }

                glm::mat4 modelMatrix  = modelInfo->meta.instances[modelInstanceId].modelMatrix;
                glm::mat3 normalMatrix = glm::transpose (glm::inverse (glm::mat3 (modelMatrix)));
                /* Note that, we are casting the matrix to a 3x3 matrix to ensure it loses its translation properties
                 * and we are casting it back to 4x4 for ease of passing it to the shader
                */
                modelInfo->meta.instances[modelInstanceId].normalMatrix = glm::mat4 (normalMatrix);
            }

        public:
            VKModelMatrix (void) {
                m_VKModelMatrixLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKModelMatrix (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            glm::mat4 getRotationMatrix (glm::vec3 rotateAngleDeg) {
                return glm::rotate (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.z),
                                                      glm::vec3    (0.0f,  0.0f, 1.0f)) *   /* Roll  */
                       glm::rotate (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.y),
                                                      glm::vec3    (0.0f, -1.0f, 0.0f)) *   /* Yaw   */
                       glm::rotate (glm::mat4 (1.0f), glm::radians (rotateAngleDeg.x),
                                                      glm::vec3    (1.0f,  0.0f, 0.0f));    /* Pitch */
            }

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

                glm::vec3 position       = modelInfo->meta.transformDatas[modelInstanceId].position;;
                glm::vec3 scale          = modelInfo->meta.transformDatas[modelInstanceId].scale;
                glm::vec3 rotateAngleDeg = modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg;
                float scaleMultiplier    = modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier;

                /* Cumulating transformations, note that we perform scaling FIRST, and THEN the rotation, and THEN the
                 * translation. This is how matrix multiplication works
                */
                glm::mat4 modelMatrix = glm::translate    (glm::mat4 (1.0f), position) *
                                        getRotationMatrix (rotateAngleDeg)             *
                                        glm::scale        (glm::mat4 (1.0f), scale     * scaleMultiplier);

                modelInfo->meta.instances[modelInstanceId].modelMatrix = modelMatrix;
                createNormalMatrix (modelInfoId, modelInstanceId);
            }
    };
}   // namespace Core
#endif  // VK_MODEL_MATRIX_H