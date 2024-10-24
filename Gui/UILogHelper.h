#ifndef UI_LOG_HELPER_H
#define UI_LOG_HELPER_H

#include "UIEnum.h"

namespace Gui {
    const char* getNodeTypeString (e_nodeType type) {
        switch (type)
        {
            case MODEL_ROOT_NODE:       return "MODEL_ROOT_NODE";
            case MODEL_TYPE_NODE:       return "MODEL_TYPE_NODE";
            case MODEL_INSTANCE_NODE:   return "MODEL_INSTANCE_NODE";
            case MODEL_TEXTURE_NODE:    return "MODEL_TEXTURE_NODE";
            case CAMERA_ROOT_NODE:      return "CAMERA_ROOT_NODE";
            case CAMERA_INFO_ID_NODE:   return "CAMERA_INFO_ID_NODE";
            default:                    return "Unhandled e_nodeType";
        }
    }

    const char* getNodeActionTypeString (e_nodeActionType type) {
        switch (type)
        {
            case CLOSE_ACTION:          return "CLOSE_ACTION";
            case OPEN_ACTION:           return "OPEN_ACTION";
            case UNDEFINED_ACTION:      return "UNDEFINED_ACTION";
            default:                    return "Unhandled e_nodeActionType";
        }
    }
}   // namespace Gui
#endif  // UI_LOG_HELPER_H