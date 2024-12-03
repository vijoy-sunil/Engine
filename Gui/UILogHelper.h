#ifndef UI_LOG_HELPER_H
#define UI_LOG_HELPER_H

#include "UIEnum.h"

namespace Gui {
    const char* getNodeTypeString (e_nodeType type) {
        switch (type)
        {
            case MODEL_NODE:            return "MODEL_NODE";
            case ANCHOR_NODE:           return "ANCHOR_NODE";
            case CAMERA_NODE:           return "CAMERA_NODE";
            case LIGHT_NODE:            return "LIGHT_NODE";
            case ROOT_NODE:             return "ROOT_NODE";
            case TYPE_NODE:             return "TYPE_NODE";
            case INSTANCE_NODE:         return "INSTANCE_NODE";
            case TEXTURE_NODE:          return "TEXTURE_NODE";
            case INFO_ID_NODE:          return "INFO_ID_NODE";
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