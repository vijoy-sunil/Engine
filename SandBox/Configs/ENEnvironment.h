#ifndef EN_ENVIRONMENT_H
#define EN_ENVIRONMENT_H

namespace SandBox {
    struct CameraSettings {

    } g_cameraSettings;
    
    struct GridSettings {
        const char* vertexShaderBinaryPath   = "Build/Bin/gridShaderVert.spv";
        const char* fragmentShaderBinaryPath = "Build/Bin/gridShaderFrag.spv";
    } g_gridSettings;
}   // namespace SandBox
#endif  // EN_ENVIRONMENT_H