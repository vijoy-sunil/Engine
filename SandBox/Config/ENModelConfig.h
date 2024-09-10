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
        {SAMPLE_1,                  {"Assets/Model/Sample/Model_1.obj", 
                                     "Assets/Model/Sample/", 
                                     "Assets/Model/Sample/Model_1_Instances.json"}},

        {SAMPLE_2,                  {"Assets/Model/Sample/Model_2.obj", 
                                     "Assets/Model/Sample/", 
                                     "Assets/Model/Sample/Model_2_Instances.json"}},

        {SAMPLE_3,                  {"Assets/Model/Sample/Model_3.obj", 
                                     "Assets/Model/Sample/", 
                                     "Assets/Model/Sample/Model_3_Instances.json"}},

        {SAMPLE_4,                  {"Assets/Model/Sample/Model_4.obj", 
                                     "Assets/Model/Sample/", 
                                     "Assets/Model/Sample/Model_4_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_staticModelImportInfoPool  = {
        {T0_GENERIC_NOCAP,          {"Assets/Model/Track/T0_Generic_NoCap.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Generic_NoCap_Instances.json"}},

        {T0_CURVE_R6_D90,           {"Assets/Model/Track/T0_Curve_R6_D90.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R6_D90_Instances.json"}},

        {T0_CURVE_R6_D90_CAP,       {"Assets/Model/Track/T0_Curve_R6_D90_Cap.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R6_D90_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z,        {"Assets/Model/Track/T0_Curve_R10_D45_Z.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R10_D45_Z_Instances.json"}},

        {T0_CURVE_R10_D45_Z_CAP,    {"Assets/Model/Track/T0_Curve_R10_D45_Z_Cap.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R10_D45_Z_Cap_Instances.json"}},

        {T0_CURVE_R10_D45_Z_SMT,    {"Assets/Model/Track/T0_Curve_R10_D45_Z_SMT.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R10_D45_Z_SMT_Instances.json"}},

        {T0_CURVE_R10_D90,          {"Assets/Model/Track/T0_Curve_R10_D90.obj", 
                                     "Assets/Model/Track/", 
                                     "Assets/Model/Track/T0_Curve_R10_D90_Instances.json"}}
    };

    std::unordered_map <e_modelType, ModelImportInfo> g_dynamicModelImportInfoPool = {
        {VEHICLE_BASE,              {"Assets/Model/Vehicle/Vehicle_Base.obj", 
                                     "Assets/Model/Vehicle/", 
                                     "Assets/Model/Vehicle/Vehicle_Base_Instances.json"}},

        {TYRE,                      {"Assets/Model/Vehicle/Tyre.obj", 
                                     "Assets/Model/Vehicle/", 
                                     "Assets/Model/Vehicle/Tyre_Instances.json"}}
    };
}   // namespace SandBox
#endif  // EN_MODEL_CONFIG_H