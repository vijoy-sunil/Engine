#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include <imgui.h>
#include "UIEnum.h"

namespace Gui {
    #define SHOW_DEMO_IMGUI                                          (false)
    #define SHOW_DEMO_IMPLOT                                         (false)

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

    struct DefaultStateSettings {
        struct ShowWindow {
            const bool worldCollection                               = true;
            const bool propertyEditor                                = true;
            const bool metricsOverlay                                = true;
            const bool boundingBox                                   = false;
            const bool shadow                                        = false;
        } showWindow;

        struct Button {
            const e_propertyType propertyEditor                      = VIEW;
        } button;

        struct TreeNode {
            /* Node info ids for specific nodes are obtained from dump file. Note that, whenever a new node is added or
             * removed, the below node info ids need to be changed as well
            */
            const uint32_t worldCollection                           = 114;     /* Camera info id 0 node    */
            const uint32_t worldCollectionSample                     = 27;      /* Camera info id 0 node    */
            /* Locked nodes are nodes with their properties locked. Note that there can be more than one locked nodes
            */
            const std::vector <uint32_t> lockedNodes                 = {
                                                                        111     /* Sky box instance 0 node  */
                                                                       };
            const std::vector <uint32_t> lockedNodesSample           = {
                                                                        24      /* Sky box instance 0 node  */
                                                                       };
        } treeNode;

        struct OverlayLocation {
            const e_overlayLocation metrics                          = BOTTOM_LEFT;
        } overlayLocation;
    } g_defaultStateSettings;

    struct StyleSettings {
        const char* iniSaveFilePath                                  = "Gui/imgui.ini";
        const char* precision                                        = "%0.2f";
        const float hoverDelay                                       = 0.2f;

        struct Font {
            const char* filePath                                     = "Asset/Font/NotoSansMono-Light.ttf";
            const float size                                         = 18.0f;
        } font;

        struct Icon {
            const char* filePath                                     = "Asset/Font/FontAwesome/fa-solid-900.ttf";
            const float size                                         = 12.0f;
        } icon;

        struct Alpha {
            const float global                                       = 1.00f;
            const float disabled                                     = 0.30f;
            const float overlay                                      = 0.35f;
        } alpha;

        struct Padding {
            const ImVec2 window                                      = ImVec2 (4.0f, 4.0f);
            const ImVec2 frame                                       = ImVec2 (4.0f, 4.0f);
            const ImVec2 child                                       = ImVec2 (4.0f, 4.0f);
            const ImVec2 cell                                        = ImVec2 (4.0f, 4.0f);
            const ImVec2 overlay                                     = ImVec2 (4.0f, 4.0f);
        } padding;

        struct Rounding {
            const float window                                       = 4.0f;
            const float frame                                        = 0.0f;
            const float child                                        = 0.0f;
            const float popUp                                        = 0.0f;
            const float scrollBar                                    = 4.0f;
            const float inputField                                   = 0.0f;
        } rounding;

        struct BorderSize {
            const float window                                       = 0.0f;
            const float frame                                        = 0.0f;
            const float child                                        = 0.0f;
            const float popUp                                        = 0.0f;
        } borderSize;

        struct Spacing {
            const float intend                                       = 12.0f;
            const ImVec2 item                                        = ImVec2 (0.0f, 4.0f);
            const ImVec2 itemInner                                   = ImVec2 (8.0f, 8.0f);
            const ImVec2 list                                        = ImVec2 (4.0f, 4.0f);
        } spacing;

        struct Size {
            const float scrollBar                                    = 12.0f;
            const float inputFieldSmall                              = 80.0f;
            const float inputFieldLarge                              = 160.0f;
            const ImVec2 image                                       = ImVec2 (200.0f, 200.0f);
            const ImVec2 verticalTabButton                           = ImVec2 (48.0f,  40.0f);
        } size;

        struct Alignment {
            const float inputField                                   = 120.0f;
            const ImVec2 windowTitle                                 = ImVec2 (0.5f, 0.5f);
            const ImVec2 buttonText                                  = ImVec2 (0.5f, 0.5f);
        } alignment;

        struct Color {
            const ImVec4 unused                                      = ImVec4 (1.00f, 0.00f, 0.00f, 1.00f);
            const ImVec4 windowBackground                            = ImVec4 (0.00f, 0.00f, 0.00f, 1.00f);

            const ImVec4 titleBackgroundActive                       = ImVec4 (0.11f, 0.33f, 0.00f, 1.00f);
            const ImVec4 titleBackgroundInactive                     = ImVec4 (0.02f, 0.05f, 0.00f, 1.00f);
            const ImVec4 titleBackgroundCollapsed                    = titleBackgroundInactive;

            const ImVec4 headerHovered                               = ImVec4 (0.08f, 0.08f, 0.08f, 1.00f);
            const ImVec4 headerActive                                = headerHovered;
            const ImVec4 header                                      = headerHovered;

            const ImVec4 frameBackgroundHovered                      = ImVec4 (0.16f, 0.16f, 0.16f, 1.00f);
            const ImVec4 frameBackgroundActive                       = frameBackgroundHovered;
            const ImVec4 frameBackground                             = header;

            const ImVec4 childBackground                             = ImVec4 (0.04f, 0.04f, 0.04f, 1.00f);
            const ImVec4 tabActive                                   = childBackground;
            const ImVec4 tabInactive                                 = ImVec4 (0.02f, 0.02f, 0.02f, 1.00f);
            const ImVec4 popUpBackground                             = tabInactive;

            const ImVec4 text                                        = ImVec4 (1.00f, 1.00f, 1.00f, 1.00f);
            const ImVec4 textActive                                  = ImVec4 (1.00f, 0.44f, 0.00f, 1.00f);
            const ImVec4 textSelectedBackground                      = textActive;
            const ImVec4 checkMark                                   = textActive;

            const ImVec4 separatorHovered                            = textActive;
            const ImVec4 separatorActive                             = separatorHovered;
            const ImVec4 separator                                   = separatorHovered;

            const ImVec4 scrollBarGrabHovered                        = textActive;
            const ImVec4 scrollBarGrabActive                         = scrollBarGrabHovered;
            const ImVec4 scrollBarGrab                               = scrollBarGrabHovered;
            const ImVec4 scrollBarBackground                         = tabInactive;

            const ImVec4 buttonHovered                               = tabActive;
            const ImVec4 buttonActive                                = buttonHovered;
            const ImVec4 button                                      = tabInactive;

            const ImVec4 resizeGripHovered                           = textActive;
            const ImVec4 resizeGripActive                            = resizeGripHovered;
            const ImVec4 resizeGrip                                  = ImVec4 (0.00f, 0.00f, 0.00f, 0.00f);

            const ImVec4 tableRowBackground                          = tabInactive;
            const ImVec4 tableRowBackgroundAlt                       = tabActive;
            const ImVec4 tableBorder                                 = header;

            const ImVec4 border                                      = ImVec4 (1.00f, 1.00f, 1.00f, 1.00f);
        } color;
    } g_styleSettings;

    struct PlotSettings {
        const float history                                          = 5.0f;
        /* Note that, the buffer capacity for plotting data depends upon the plot history length and the frame
         * rate. For example, at 60 fps with 5s history will need a buffer capacity of 300
        */
        const size_t bufferCapacity                                  = 500;
        const ImVec2 padding                                         = ImVec2 (0.0f,    0.0f);
        const ImVec2 size                                            = ImVec2 (200.0f, 40.0f);
    } g_plotSettings;
}   // namespace Gui
#endif  // UI_CONFIG_H