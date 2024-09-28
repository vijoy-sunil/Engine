#ifndef EN_ENUM_H
#define EN_ENUM_H

namespace SandBox {
    typedef enum {
        SAMPLE_1                    = 0,
        SAMPLE_2                    = 1,
        SAMPLE_3                    = 2,
        SAMPLE_4                    = 3,
        T0_GENERIC_NOCAP            = 4,
        T0_CURVE_R6_D90             = 5,
        T0_CURVE_R6_D90_CAP         = 6,
        T0_CURVE_R10_D45_Z          = 7,
        T0_CURVE_R10_D45_Z_CAP      = 8,
        T0_CURVE_R10_D45_Z_SMT      = 9,
        T0_CURVE_R10_D90            = 10,
        VEHICLE_BASE                = 11,
        TYRE                        = 12
    } e_modelType;

    typedef enum {
        DRONE                       = 0,
        SPOILER                     = 1,
        LEFT_PROFILE                = 2,
        REVERSE                     = 3,
        RIGHT_PROFILE               = 4,
        REAR_AXLE                   = 5,
        TOP_DOWN                    = 6,
        FRONT_AXLE                  = 7,
        STADIUM                     = 8,
        UNDEFINED                   = 9
    } e_cameraType;
}   // namespace SandBox
#endif  // EN_ENUM_H