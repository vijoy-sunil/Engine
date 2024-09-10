#ifndef EN_MODEL_CONFIG_H
#define EN_MODEL_CONFIG_H

#include <map>
#include "../ENEnum.h"

namespace SandBox {
    #define ENABLE_SAMPLE_MODELS_IMPORT         (false)

    struct ModelImportInfo {
        const char* modelPath;
        const char* mtlFileDirPath;
        const char* instanceDataPath;
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_sampleModelImportInfoPool  = {
        {SAMPLE_1,                  {"SandBox/Models/Sample/Model_1.obj", 
                                     "SandBox/Models/Sample/", 
                                     "SandBox/Models/Sample/Model_1_Instances.json"}},

        {SAMPLE_2,                  {"SandBox/Models/Sample/Model_2.obj", 
                                     "SandBox/Models/Sample/", 
                                     "SandBox/Models/Sample/Model_2_Instances.json"}},

        {SAMPLE_3,                  {"SandBox/Models/Sample/Model_3.obj", 
                                     "SandBox/Models/Sample/", 
                                     "SandBox/Models/Sample/Model_3_Instances.json"}},

        {SAMPLE_4,                  {"SandBox/Models/Sample/Model_4.obj", 
                                     "SandBox/Models/Sample/", 
                                     "SandBox/Models/Sample/Model_4_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_staticModelImportInfoPool  = {
        {T0_GENERIC_NOCAP,          {"SandBox/Models/Track/T0_Generic_NoCap.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Generic_NoCap_Instances.json"}},

        {T0_CURVE_R6_D90,           {"SandBox/Models/Track/T0_Curve_R6_D90.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R6_D90_Instances.json"}},

        {T0_CURVE_R6_D90_CAP,       {"SandBox/Models/Track/T0_Curve_R6_D90_Cap.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R6_D90_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z,        {"SandBox/Models/Track/T0_Curve_R10_D45_Z.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R10_D45_Z_Instances.json"}},

        {T0_CURVE_R10_D45_Z_CAP,    {"SandBox/Models/Track/T0_Curve_R10_D45_Z_Cap.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R10_D45_Z_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z_SMT,    {"SandBox/Models/Track/T0_Curve_R10_D45_Z_SMT.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R10_D45_Z_SMT_Instances.json"}},

        {T0_CURVE_R10_D90,          {"SandBox/Models/Track/T0_Curve_R10_D90.obj", 
                                     "SandBox/Models/Track/", 
                                     "SandBox/Models/Track/T0_Curve_R10_D90_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_dynamicModelImportInfoPool = {
        {VEHICLE_BASE,              {"SandBox/Models/Vehicle/Vehicle_Base.obj", 
                                     "SandBox/Models/Vehicle/", 
                                     "SandBox/Models/Vehicle/Vehicle_Base_Instances.json"}},

        {TYRE,                      {"SandBox/Models/Vehicle/Tyre.obj", 
                                     "SandBox/Models/Vehicle/", 
                                     "SandBox/Models/Vehicle/Tyre_Instances.json"}}
    };
}   // namespace SandBox
#endif  // EN_MODEL_CONFIG_H