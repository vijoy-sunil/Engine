#ifndef EN_CONFIG_H
#define EN_CONFIG_H

#include <map>

namespace SandBox {
    #define ENABLE_SAMPLE_MODELS_IMPORT         (false)

    struct ModelImportInfo {
        const char* modelPath;
        const char* mtlFileDirPath;
        const char* instanceDataPath;
    };

    std::map <uint32_t, ModelImportInfo> g_modelImportInfoPool = {
        /* |------------------------------------------------------------------------------------------------|
         * | SAMPLE                                                                                         |
         * |------------------------------------------------------------------------------------------------|
        */
#if ENABLE_SAMPLE_MODELS_IMPORT
        {0, {"SandBox/Models/Sample/Model_1.obj", 
             "SandBox/Models/Sample/", 
             "SandBox/Models/Sample/Model_1_Instances.json"}},

        {1, {"SandBox/Models/Sample/Model_2.obj", 
             "SandBox/Models/Sample/", 
             "SandBox/Models/Sample/Model_2_Instances.json"}},

        {2, {"SandBox/Models/Sample/Model_3.obj", 
             "SandBox/Models/Sample/", 
             "SandBox/Models/Sample/Model_3_Instances.json"}},

        {3, {"SandBox/Models/Sample/Model_4.obj", 
             "SandBox/Models/Sample/", 
             "SandBox/Models/Sample/Model_4_Instances.json"}},
        /* |------------------------------------------------------------------------------------------------|
         * | TRACK                                                                                          |
         * |------------------------------------------------------------------------------------------------|
        */
#else
        {4, {"SandBox/Models/Track/T0_Generic_NoCap.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Generic_NoCap_Instances.json"}},

        {5, {"SandBox/Models/Track/T0_Curve_R6_D90.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Curve_R6_D90_Instances.json"}},

        {6, {"SandBox/Models/Track/T0_Curve_R6_D90_Cap.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Curve_R6_D90_Cap_Instances.json"}},

        {7, {"SandBox/Models/Track/T0_Curve_R10_D45_Z.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Curve_R10_D45_Z_Instances.json"}},

        {8, {"SandBox/Models/Track/T0_Curve_R10_D45_Z_Cap.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Curve_R10_D45_Z_Cap_Instances.json"}},

        {9, {"SandBox/Models/Track/T0_Curve_R10_D45_Z_SMT.obj", 
             "SandBox/Models/Track/", 
             "SandBox/Models/Track/T0_Curve_R10_D45_Z_SMT_Instances.json"}},

        {10, {"SandBox/Models/Track/T0_Curve_R10_D90.obj", 
              "SandBox/Models/Track/", 
              "SandBox/Models/Track/T0_Curve_R10_D90_Instances.json"}},
        /* |------------------------------------------------------------------------------------------------|
         * | VEHICLE                                                                                        |
         * |------------------------------------------------------------------------------------------------|
        */
        {11, {"SandBox/Models/Vehicle/Vehicle_Base.obj", 
              "SandBox/Models/Vehicle/", 
              "SandBox/Models/Vehicle/Vehicle_Base_Instances.json"}},

        {12, {"SandBox/Models/Vehicle/Tyre.obj", 
              "SandBox/Models/Vehicle/", 
              "SandBox/Models/Vehicle/Tyre_Instances.json"}}
#endif  // ENABLE_SAMPLE_MODELS_IMPORT
    };
}   // namespace SandBox
#endif  // EN_CONFIG_H