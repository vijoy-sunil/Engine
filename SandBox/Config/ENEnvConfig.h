#ifndef EN_ENV_CONFIG_H
#define EN_ENV_CONFIG_H

#include <map>
#include <glm/glm.hpp>
#include "../ENEnum.h"

namespace SandBox {
    struct CollectionsSettings {
        /* Collections instance id range assignments
         * Reserved     [0]
         * Core/        [1,   100]
         * SandBox/     [101, 200]
        */
        uint32_t instanceId                  = 101;
        const char* logSaveDirPath           = "Build/Log/SandBox/";
    } g_collectionsSettings;

    struct GridSettings {
        const char* vertexShaderBinaryPath   = "Build/Bin/gridShaderVert.spv";
        const char* fragmentShaderBinaryPath = "Build/Bin/gridShaderFrag.spv";
    } g_gridSettings;

    struct CameraSettings {
        const float movementSpeed            = 1.5f;
        const float sensitivity              = 0.1f;
        const float minPitchDeg              = -89.0f;
        const float maxPitchDeg              = 89.0f;
        const float minFovDeg                = 1.0f;
        const float maxFovDeg                = 110.0f;
        const float nearPlane                = 0.01f;
        const float farPlane                 = 100.0f;
        const glm::vec3 upVector             = {0.0f, -1.0f,  0.0f};

        struct KeyMap {
            const int freeRoam               = '0';
            const int spoiler                = '1';
            const int fpv                    = '2';
            const int topDown                = '3';
            const int rightProfile           = '4';
            const int leftProfile            = '5';
            const int moveLeft               = 'A';
            const int moveRight              = 'D';
            const int moveBackward           = 'S';
            const int moveForward            = 'W';
        } keyMap;
    } g_cameraSettings;

    struct CameraStateInfo {
        glm::vec3 position;
        glm::vec3 direction;
        float fovDeg;
    };

    std::unordered_map <e_cameraType, CameraStateInfo> g_cameraStateInfoPool = {
        /* Camera position with respect to vehicle base
         * 
         *      Z                   |===============|   Front       : {0.0, 0.0, 0.9}
         *      |                   |               |
         *      |                   |---|       |---|
         *     -Y-------X               |       |
         *                          |---|       |---|
         *                          |               |
         *                          |       X       |   Origin(*)   : {0.0, 0.0, 0.0}
         *                          |               |
         *                          |---|       |---|
         *                              |       |
         *                          |---|       |---|
         *                          |               |
         *                          |---------------|   Rear        : {0.0, 0.0, -0.8}
         *                                              Length      : 1.7 units
         *                                              Width       : 0.6 units
        */

        /* Dummy values for free roam, since we will be using previus state info to roam around
        */
        {FREE_ROAM,     {{ 0.0f,  0.0f,   0.0f},    {0.0f,  0.0f,  0.0f},        0.0f}},
        {SPOILER,       {{ 0.0f, -0.87f, -3.0f},    {0.0f,  0.0f,  0.0f},       50.0f}},
        {FPV,           {{ 0.0f, -0.17f,  0.9f},    {0.0f, -0.17f, 1.0f},       80.0f}},
        {TOP_DOWN,      {{ 0.0f, -6.37f,  0.0f},    {0.0f,  0.0f,  0.2f},       50.0f}},
        {RIGHT_PROFILE, {{ 2.0f,  0.0f,   0.0f},    {0.0f,  0.0f,  0.0f},       80.0f}},
        {LEFT_PROFILE,  {{-2.0f,  0.0f,   0.0f},    {0.0f,  0.0f,  0.0f},       80.0f}}
    };

    struct CoreSettings {
        struct KeyMap {
            const int exitWindow             = 256; /* ESC key */
            const int steerRight             = 262;
            const int steerLeft              = 263;
            const int brake                  = 264;
            const int accelerate             = 265;
        } keyMap;
    } g_coreSettings;
}   // namespace SandBox
#endif  // EN_ENV_CONFIG_H