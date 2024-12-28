#ifndef EN_CONFIG_H
#define EN_CONFIG_H

#include <map>
#include <glm/glm.hpp>
#include "ENEnum.h"

namespace SandBox {
    #define SELECT_TRACK_ID                                          (0)

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
        const char* transformDataPath;
    };

    std::unordered_map <e_anchorType, ModelImportInfo> g_cameraAnchorImportInfoPool = {
        {ANCHOR_CAMERA,             {"Asset/Model/Anchor/Anchor_Camera.obj",
                                     "Asset/Model/Anchor/",
                                     "Asset/Model/Anchor/Anchor_Camera_Instances.json"}}
    };

    /* Note that, we are not using unordered map since the order in which the anchors (and thus the lights) are laid
     * is important. This is because the instance datas of all the lights are expected to be in a specific order
    */
    std::map <e_anchorType, ModelImportInfo> g_lightAnchorImportInfoPool = {
        {ANCHOR_DIRECTIONAL_LIGHT,  {"Asset/Model/Anchor/Anchor_Directional_Light.obj",
                                     "Asset/Model/Anchor/",
                                     "Asset/Model/Anchor/Anchor_Directional_Light_Instances.json"}},

        {ANCHOR_POINT_LIGHT,        {"Asset/Model/Anchor/Anchor_Point_Light.obj",
                                     "Asset/Model/Anchor/",
                                     "Asset/Model/Anchor/Anchor_Point_Light_Instances.json"}},

        {ANCHOR_SPOT_LIGHT,         {"Asset/Model/Anchor/Anchor_Spot_Light.obj",
                                     "Asset/Model/Anchor/",
                                     "Asset/Model/Anchor/Anchor_Spot_Light_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_trackModelImportInfoPool = {
#if SELECT_TRACK_ID == 0
        {CUBE,                      {"Asset/Model/Track/0/Cube.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/Cube_Instances.json"}},

        {CYLINDER,                  {"Asset/Model/Track/0/Cylinder.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/Cylinder_Instances.json"}},

        {T_BEAM,                    {"Asset/Model/Track/0/T_Beam.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/T_Beam_Instances.json"}},

        {SLOPE,                     {"Asset/Model/Track/0/Slope.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/Slope_Instances.json"}},

        {BRIDGE,                    {"Asset/Model/Track/0/Bridge.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/Bridge_Instances.json"}},

        {PLATFORM,                  {"Asset/Model/Track/0/Platform.obj",
                                     "Asset/Model/Track/0/",
                                     "Asset/Model/Track/0/Platform_Instances.json"}}
#endif  // SELECT_TRACK_ID
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_skyBoxModelImportInfoPool = {
        {SKY_BOX,                   {"Asset/Model/SkyBox/Sky_Box.obj",
                                     "Asset/Model/SkyBox/",
                                     "Asset/Model/SkyBox/Sky_Box_Instances.json"}}
    };

    /* A sky box is a "large" cube that encompasses the entire scene and contains 6 images of a surrounding environment,
     * giving the player the illusion that the environment they are in is actually much larger than it actually is. The
     * sky box images usually have the following pattern. If you would fold those 6 sides into a cube you'd get the
     * completely textured cube that simulates a large landscape
     *                              |-----------|
     *                              |           |
     *                              |     PY    |
     *                              |           |
     *                  |-----------|-----------|-----------|-----------|
     *                  |           |           |           |           |
     *                  |     NX    |     PZ    |     PX    |     NZ    |
     *                  |           |           |           |           |
     *                  |-----------|-----------|-----------|-----------|
     *                              |           |
     *                              |     NY    |
     *                              |           |
     *                              |-----------|
     * Note that, we are not using unordered map since the order in which the path is laid out is important
    */
    std::map <e_cubeMapTarget, const char*> g_skyBoxTextureImagePool = {
        {POSITIVE_X,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_px.png"},
        {NEGATIVE_X,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_nx.png"},
        {POSITIVE_Y,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_py.png"},
        {NEGATIVE_Y,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_ny.png"},
        {POSITIVE_Z,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_pz.png"},
        {NEGATIVE_Z,                "Asset/Texture/SkyBox/Evening/tex_2Kx2K_sky_box_nz.png"}
    };

    std::unordered_map <std::string, uint32_t> g_shininessImportPool = {
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_2Kx2K_grid.png",          256},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_arrow.png",       64},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_border.png",      48},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_circle.png",      64},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_cross.png",       64},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_strips.png",      64},
        {"/Users/vijoys/Downloads/Projects/Engine/Asset/Texture/Track/tex_512x512_text.png",        48},
    };

    struct PipelineSettings {
        struct SkyBoxShaderStage {
            const char* vertexShaderBinaryPath                       = "Build/Bin/skyBoxShaderVert.spv";
            const char* fragmentShaderBinaryPath                     = "Build/Bin/skyBoxShaderFrag.spv";
        } skyBoxShaderStage;

        struct AnchorShaderStage {
            const char* vertexShaderBinaryPath                       = "Build/Bin/anchorShaderVert.spv";
            const char* fragmentShaderBinaryPath                     = "Build/Bin/anchorShaderFrag.spv";
        } anchorShaderStage;

        struct GridShaderStage {
            const char* vertexShaderBinaryPath                       = "Build/Bin/gridShaderVert.spv";
            const char* fragmentShaderBinaryPath                     = "Build/Bin/gridShaderFrag.spv";
        } gridShaderStage;
    } g_pipelineSettings;

    struct CameraSettings {
        const float movementSpeed                                    =  0.60f;
        const float sensitivity                                      =  0.08f;
        const float minPitchDeg                                      = -89.0f;
        const float maxPitchDeg                                      =  89.0f;
        const float minFovDeg                                        =   1.0f;
        const float maxFovDeg                                        = 110.0f;
        const float nearPlane                                        =  0.01f;
        const float farPlane                                         = 100.0f;
        const glm::vec3 upVector                                     = {0.0f, -1.0f, 0.0f};
    } g_cameraSettings;

    struct CameraTypeInfo {
        glm::vec3 position;
        glm::vec3 direction;
        float fovDeg;
    };

    std::unordered_map <e_cameraType, CameraTypeInfo> g_cameraTypeInfoPool = {
        /* Camera position with respect to vehicle chassis
         *
         *      Z                   |===============|   Front       : {0.0, 0.0, 0.9}
         *      |                   |               |
         *      |                   |---|       |---|
         *     -Y-------X               |       |       Front Axle  : {0.0, 0.0, 0.5}
         *                          |---|       |---|
         *                          |               |
         *                          |       X       |   Origin(*)   : {0.0, 0.0, 0.0}
         *                          |               |
         *                          |---|       |---|
         *                              |       |       Rear Axle   : {0.0, 0.0, -0.5}
         *                          |---|       |---|
         *                          |               |
         *                          |---------------|   Rear        : {0.0, 0.0, -0.8}
         *                                              Length      : 1.7 units
         *                                              Width       : 0.6 units
        */
        {SPOILER,       {{  0.0f,  -0.87f, -2.0f},  {0.0f,  0.0f,   0.5f},      50.0f}},
        {LEFT_PROFILE,  {{ -2.0f,   0.0f,   0.0f},  {0.0f,  0.0f,   0.0f},      80.0f}},
        {REVERSE,       {{  0.0f,  -0.87f,  2.0f},  {0.0f,  0.0f,  -0.5f},      50.0f}},
        {RIGHT_PROFILE, {{  2.0f,   0.0f,   0.0f},  {0.0f,  0.0f,   0.0f},      80.0f}},
        {REAR_AXLE,     {{  0.0f,  -0.47f,  0.0f},  {0.0f, -0.17f, -0.5f},      80.0f}},
        {TOP_DOWN,      {{  0.0f,  -6.37f,  0.0f},  {0.0f,  0.0f,   0.2f},      50.0f}},
        {FRONT_AXLE,    {{  0.0f,  -0.47f,  0.0f},  {0.0f, -0.17f,  0.5f},      80.0f}},
        {DRONE_FOLLOW,  {{  0.0f,   0.0f,   0.0f},  {0.0f,  0.0f,   0.0f},       0.0f}}
    };

    struct KeyMapSettings {
        const int spoiler                                            = '1';
        const int leftProfile                                        = '2';
        const int reverse                                            = '3';
        const int rightProfile                                       = '4';
        const int rearAxle                                           = '5';
        const int topDown                                            = '6';
        const int frontAxle                                          = '7';
        const int droneLock                                          = '8';
        const int droneFollow                                        = '9';
        const int droneFly                                           = '0';
        const int moveLeft                                           = 'A';
        const int moveRight                                          = 'D';
        const int moveBackward                                       = 'S';
        const int moveForward                                        = 'W';
        const int exitWindow                                         = 256; /* ESC key */
    } g_keyMapSettings;
}   // namespace SandBox
#endif  // EN_CONFIG_H