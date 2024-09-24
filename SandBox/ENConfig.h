#ifndef EN_CONFIG_H
#define EN_CONFIG_H

#include <map>
#include <glm/glm.hpp>
#include "ENEnum.h"

namespace SandBox {
    #define ENABLE_SAMPLE_MODELS_IMPORT                              (false)

    struct CollectionSettings {
        /* Collection instance id range assignments
         * Reserved     [0]
         * Core/        [1,   100]
         * SandBox/     [101, 200]
        */
        uint32_t instanceId                                          = 101;
        const char* logSaveDirPath                                   = "Build/Log/SandBox/";
    } g_collectionSettings;

    struct ModelImportInfo {
        const char* modelPath;
        const char* mtlFileDirPath;
        const char* instanceDataPath;
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_sampleModelImportInfoPool  = {
        {SAMPLE_1,                  {"Asset/Model/Sample/Model_1.obj",
                                     "Asset/Model/Sample/",
                                     "Asset/Model/Sample/Model_1_Instances.json"}},

        {SAMPLE_2,                  {"Asset/Model/Sample/Model_2.obj",
                                     "Asset/Model/Sample/",
                                     "Asset/Model/Sample/Model_2_Instances.json"}},

        {SAMPLE_3,                  {"Asset/Model/Sample/Model_3.obj",
                                     "Asset/Model/Sample/",
                                     "Asset/Model/Sample/Model_3_Instances.json"}},

        {SAMPLE_4,                  {"Asset/Model/Sample/Model_4.obj",
                                     "Asset/Model/Sample/",
                                     "Asset/Model/Sample/Model_4_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_staticModelImportInfoPool  = {
        {T0_GENERIC_NOCAP,          {"Asset/Model/Track/T0_Generic_NoCap.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Generic_NoCap_Instances.json"}},

        {T0_CURVE_R6_D90,           {"Asset/Model/Track/T0_Curve_R6_D90.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R6_D90_Instances.json"}},

        {T0_CURVE_R6_D90_CAP,       {"Asset/Model/Track/T0_Curve_R6_D90_Cap.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R6_D90_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z,        {"Asset/Model/Track/T0_Curve_R10_D45_Z.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R10_D45_Z_Instances.json"}},

        {T0_CURVE_R10_D45_Z_CAP,    {"Asset/Model/Track/T0_Curve_R10_D45_Z_Cap.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R10_D45_Z_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z_SMT,    {"Asset/Model/Track/T0_Curve_R10_D45_Z_SMT.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R10_D45_Z_SMT_Instances.json"}},

        {T0_CURVE_R10_D90,          {"Asset/Model/Track/T0_Curve_R10_D90.obj",
                                     "Asset/Model/Track/",
                                     "Asset/Model/Track/T0_Curve_R10_D90_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_dynamicModelImportInfoPool = {
        {VEHICLE_BASE,              {"Asset/Model/Vehicle/Vehicle_Base.obj",
                                     "Asset/Model/Vehicle/",
                                     "Asset/Model/Vehicle/Vehicle_Base_Instances.json"}},

        {TYRE,                      {"Asset/Model/Vehicle/Tyre.obj",
                                     "Asset/Model/Vehicle/",
                                     "Asset/Model/Vehicle/Tyre_Instances.json"}}
    };

    struct PipelineSettings {
        struct ShaderStage {
            const char* vertexShaderBinaryPath                       = "Build/Bin/gridShaderVert.spv";
            const char* fragmentShaderBinaryPath                     = "Build/Bin/gridShaderFrag.spv";
        } shaderStage;
    } g_pipelineSettings;

    struct CameraSettings {
        const float movementSpeed                                    = 1.5f;
        const float sensitivity                                      = 0.1f;
        const float minPitchDeg                                      = -89.0f;
        const float maxPitchDeg                                      = 89.0f;
        const float minFovDeg                                        = 1.0f;
        const float maxFovDeg                                        = 110.0f;
        const float nearPlane                                        = 0.01f;
        const float farPlane                                         = 100.0f;
        const glm::vec3 upVector                                     = {0.0f, -1.0f,  0.0f};
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

        /* Dummy values for drone mode, since we will be using previous state info to move around
        */
        {DRONE,         {{  0.0f,   0.0f,   0.0f},  {0.0f,  0.0f,  0.0f},   0.0f}},
        {SPOILER,       {{  0.0f,  -0.87f, -3.0f},  {0.0f,  0.0f,  0.0f},   50.0f}},
        {FPV,           {{  0.0f,  -0.17f,  0.9f},  {0.0f, -0.17f, 1.0f},   80.0f}},
        {TOP_DOWN,      {{  0.0f,  -6.37f,  0.0f},  {0.0f,  0.0f,  0.2f},   50.0f}},
        {RIGHT_PROFILE, {{  2.0f,   0.0f,   0.0f},  {0.0f,  0.0f,  0.0f},   80.0f}},
        {LEFT_PROFILE,  {{ -2.0f,   0.0f,   0.0f},  {0.0f,  0.0f,  0.0f},   80.0f}},
        {STADIUM,       {{-10.0f, -10.0f, -10.0f},  {0.0f,  0.0f,  0.0f},   80.0f}}
    };

    struct KeyMapSettings {
        const int drone                                              = '0';
        const int spoiler                                            = '1';
        const int fpv                                                = '2';
        const int topDown                                            = '3';
        const int rightProfile                                       = '4';
        const int leftProfile                                        = '5';
        const int stadium                                            = '6';
        const int moveLeft                                           = 'A';
        const int moveRight                                          = 'D';
        const int moveBackward                                       = 'S';
        const int moveForward                                        = 'W';
        const int exitWindow                                         = 256; /* ESC key */
    } g_keyMapSettings;
}   // namespace SandBox
#endif  // EN_CONFIG_H