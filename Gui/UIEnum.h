#ifndef UI_ENUM_H
#define UI_ENUM_H

namespace Gui {
    typedef enum {
        MODEL_NODE          = 1,
        ANCHOR_NODE         = 2,
        CAMERA_NODE         = 4,
        LIGHT_NODE          = 8,
        ROOT_NODE           = 16,
        TYPE_NODE           = 32,
        INSTANCE_NODE       = 64,
        TEXTURE_NODE        = 128,
        INFO_ID_NODE        = 256
    } e_nodeType;

    typedef enum {
        CLOSE_ACTION        = 0,
        OPEN_ACTION         = 1,
        UNDEFINED_ACTION    = 2
    } e_nodeActionType;

    typedef enum {
        TRANSFORM           = 0,
        VIEW                = 1,
        TEXTURE             = 2,
        LIGHT               = 3,
        PHYSICS             = 4,
        DEBUG               = 5
    } e_propertyType;

    typedef enum {
        CENTER              = 0,
        CUSTOM              = 1,
        TOP_LEFT            = 2,
        TOP_RIGHT           = 3,
        BOTTOM_LEFT         = 4,
        BOTTOM_RIGHT        = 5,
    } e_overlayLocation;
}   // namespace Gui
#endif  // UI_ENUM_H