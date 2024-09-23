#ifndef UI_CONFIG_H
#define UI_CONFIG_H

namespace Gui {
    struct CollectionSettings {
        /* Collection instance id range assignments
         * Reserved     [0]
         * Core/        [1,   100]
         * SandBox/     [101, 200]
         * Gui/         [201, 300]
        */
        uint32_t instanceId                                          = 201;
        const char* logSaveDirPath                                   = "Build/Log/Gui/";
    } g_collectionSettings;

    struct StyleSettings {
        const char* iniSaveFilePath                                  = "Gui/imgui.ini";
    } g_styleSettings;
}   // namespace Gui
#endif  // UI_CONFIG_H