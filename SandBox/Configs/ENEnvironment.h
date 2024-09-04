#ifndef EN_ENVIRONMENT_H
#define EN_ENVIRONMENT_H

namespace SandBox {    
    struct GridSettings {
        const char* vertexShaderBinaryPath   = "Build/Bin/gridShaderVert.spv";
        const char* fragmentShaderBinaryPath = "Build/Bin/gridShaderFrag.spv";
    } g_gridSettings;

    struct CameraSettings {

    } g_cameraSettings;
}   // namespace SandBox
#endif  // EN_ENVIRONMENT_H