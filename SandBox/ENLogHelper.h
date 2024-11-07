#ifndef EN_LOG_HELPER_H
#define EN_LOG_HELPER_H

#include "ENEnum.h"

namespace SandBox {
    const char* getModelTypeString (e_modelType type) {
        switch (type)
        {
            case SAMPLE_1:                  return "SAMPLE_1";
            case SAMPLE_2:                  return "SAMPLE_2";
            case SAMPLE_3:                  return "SAMPLE_3";
            case SAMPLE_4:                  return "SAMPLE_4";
            case T0_GENERIC_NOCAP:          return "T0_GENERIC_NOCAP";
            case T0_CURVE_R6_D90:           return "T0_CURVE_R6_D90";
            case T0_CURVE_R6_D90_CAP:       return "T0_CURVE_R6_D90_CAP";
            case T0_CURVE_R10_D45_Z:        return "T0_CURVE_R10_D45_Z";
            case T0_CURVE_R10_D45_Z_CAP:    return "T0_CURVE_R10_D45_Z_CAP";
            case T0_CURVE_R10_D45_Z_SMT:    return "T0_CURVE_R10_D45_Z_SMT";
            case T0_CURVE_R10_D90:          return "T0_CURVE_R10_D90";
            case VEHICLE_BASE:              return "VEHICLE_BASE";
            case TYRE:                      return "TYRE";
            case SKY_BOX:                   return "SKY_BOX";
            default:                        return "Unhandled e_modelType";
        }
    }

    const char* getCameraTypeString (e_cameraType type) {
        switch (type)
        {
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