#ifndef VK_TRANSFORM_H
#define VK_TRANSFORM_H

#include <glm/glm.hpp>

namespace Renderer {
    struct TransformInfo {
        struct Model {
            glm::vec3 translate;
            glm::vec3 rotateAxis;
            glm::vec3 scale;

            float rotateAngleDeg;
        } model;

        struct Camera {
            glm::vec3 position; 
            glm::vec3 center;
            glm::vec3 upVector;

            float fovDeg; 
            float nearPlane; 
            float farPlane;
        } camera;
    };
}   // namespace Renderer
#endif  // VK_TRANSFORM_H