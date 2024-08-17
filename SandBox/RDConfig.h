#ifndef RD_CONFIG_H
#define RD_CONFIG_H

#include <map>

namespace SandBox {
    struct ModelImportInfo {
        uint32_t instanceCount;
        const char* modelPath;
        const char* mtlFileDirPath;
    };

    std::map <uint32_t, ModelImportInfo> g_modelImportInfoPool = {
        {0, {1, "SandBox/Models/Sample/Model_1.obj", "SandBox/Models/Sample/"}},
        {1, {1, "SandBox/Models/Sample/Model_2.obj", "SandBox/Models/Sample/"}},
        {2, {1, "SandBox/Models/Sample/Model_3.obj", "SandBox/Models/Sample/"}},
        {3, {1, "SandBox/Models/Sample/Model_4.obj", "SandBox/Models/Sample/"}}
    };
}   // namespace SandBox
#endif  // RD_CONFIG_H