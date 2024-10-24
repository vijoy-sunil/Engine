#ifndef UI_ENUM_H
#define UI_ENUM_H

namespace Gui {
    typedef enum {
        MODEL_ROOT_NODE     = 0,
        MODEL_TYPE_NODE     = 1,
        MODEL_INSTANCE_NODE = 2,
        MODEL_TEXTURE_NODE  = 3,
        CAMERA_ROOT_NODE    = 4,
        CAMERA_INFO_ID_NODE = 5
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