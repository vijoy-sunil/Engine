#ifndef EN_ENUM_H
#define EN_ENUM_H

namespace SandBox {
    typedef enum {
        ANCHOR_CAMERA               = 3,
        /* Note that light anchor enums must match light type enums
        */
        ANCHOR_DIRECTIONAL_LIGHT    = 0,
        ANCHOR_POINT_LIGHT          = 1,
        ANCHOR_SPOT_LIGHT           = 2,
    } e_anchorType;

    typedef enum {
        CUBE                        = 4,
        CYLINDER                    = 5,
        T_BEAM                      = 6,
        SLOPE                       = 7,
        BRIDGE                      = 8,
        PLATFORM                    = 9,
        SKY_BOX                     = 10
    } e_modelType;

    typedef enum {
        POSITIVE_X                  = 0,
        NEGATIVE_X                  = 1,
        POSITIVE_Y                  = 2,
        NEGATIVE_Y                  = 3,
        POSITIVE_Z                  = 4,
        NEGATIVE_Z                  = 5
    } e_cubeMapTarget;

    typedef enum {
        SPOILER                     = 1,
        LEFT_PROFILE                = 2,
        REVERSE                     = 3,
        RIGHT_PROFILE               = 4,
        REAR_AXLE                   = 5,
        TOP_DOWN                    = 6,
        FRONT_AXLE                  = 7,
        DRONE_LOCK                  = 8,
        DRONE_FOLLOW                = 9,
        DRONE_FLY                   = 0,
        UNDEFINED                   = 10
    } e_cameraType;
}   // namespace SandBox
#endif  // EN_ENUM_H