#ifndef EN_LOG_HELPER_H
#define EN_LOG_HELPER_H

#include "ENEnum.h"

namespace SandBox {
    const char* getAnchorTypeString (e_anchorType type) {
        switch (type) {
            case ANCHOR_CAMERA:             return "ANCHOR_CAMERA";
            case ANCHOR_DIRECTIONAL_LIGHT:  return "ANCHOR_DIRECTIONAL_LIGHT";
            case ANCHOR_POINT_LIGHT:        return "ANCHOR_POINT_LIGHT";
            case ANCHOR_SPOT_LIGHT:         return "ANCHOR_SPOT_LIGHT";
            default:                        return "Unhandled e_anchorType";
        }
    }

    const char* getModelTypeString (e_modelType type) {
        switch (type) {
            case CUBE:                      return "CUBE";
            case CYLINDER:                  return "CYLINDER";
            case T_BEAM:                    return "T_BEAM";
            case SLOPE:                     return "SLOPE";
            case BRIDGE:                    return "BRIDGE";
            case PLATFORM:                  return "PLATFORM";
            case SKY_BOX:                   return "SKY_BOX";
            default:                        return "Unhandled e_modelType";
        }
    }

    const char* getCameraTypeString (e_cameraType type) {
        switch (type) {
            case SPOILER:                   return "SPOILER";
            case LEFT_PROFILE:              return "LEFT_PROFILE";
            case REVERSE:                   return "REVERSE";
            case RIGHT_PROFILE:             return "RIGHT_PROFILE";
            case REAR_AXLE:                 return "REAR_AXLE";
            case TOP_DOWN:                  return "TOP_DOWN";
            case FRONT_AXLE:                return "FRONT_AXLE";
            case DRONE_LOCK:                return "DRONE_LOCK";
            case DRONE_FOLLOW:              return "DRONE_FOLLOW";
            case DRONE_FLY:                 return "DRONE_FLY";
            case UNDEFINED:                 return "UNDEFINED";
            default:                        return "Unhandled e_cameraType";
        }
    }
}   // namespace SandBox
#endif  // EN_LOG_HELPER_H