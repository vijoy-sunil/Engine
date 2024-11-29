#ifndef VK_INSTANCE_DATA_H
#define VK_INSTANCE_DATA_H

#include <json/single_include/nlohmann/json.hpp>
#include "VKModelMatrix.h"

namespace Core {
    class VKInstanceData: protected virtual VKModelMatrix {
        private:
            Log::Record* m_VKInstanceDataLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            VKInstanceData (void) {
                m_VKInstanceDataLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKInstanceData (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void updateTexIdLUT (uint32_t modelInfoId,
                                 uint32_t modelInstanceId,
                                 uint32_t oldTexId,
                                 uint32_t newTexId) {

                auto modelInfo = getModelInfo (modelInfoId);
                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKInstanceDataLog) << "Invalid model instance id "
                                                    << "[" << modelInstanceId << "]"
                                                    << "->"
                                                    << "[" << modelInfo->meta.instancesCount << "]"
                                                    << std::endl;
                    throw std::runtime_error ("Invalid model instance id");
                }

                if (oldTexId > UINT8_MAX || newTexId > UINT8_MAX) {
                    LOG_ERROR (m_VKInstanceDataLog) << "Failed to encode packet "
                                                    << "[" << oldTexId << "]"
                                                    << " "
                                                    << "[" << newTexId << "]"
                                                    << std::endl;
                    throw std::runtime_error ("Failed to encode packet");
                }
                uint32_t writeIdx  = oldTexId / 4;
                uint32_t offsetIdx = oldTexId % 4;
                uint32_t mask      = UINT8_MAX << offsetIdx * 8;
                uint32_t packet    = modelInfo->meta.instances[modelInstanceId].texIdLUT[writeIdx];

                packet             = packet & ~mask;
                packet             = packet | (newTexId << offsetIdx * 8);

                modelInfo->meta.instances[modelInstanceId].texIdLUT[writeIdx] = packet;
            }

            uint32_t importTransformData (uint32_t modelInfoId, const char* transformDataPath) {
                auto modelInfo = getModelInfo (modelInfoId);
                uint32_t instancesCount = 0;
                /* Read and parse json file
                */
                std::ifstream fJson (transformDataPath);
                std::stringstream stream;

                stream << fJson.rdbuf();
                nlohmann::basic_json json;
                /* When the input is not valid JSON, an exception of type parse_error is thrown. This exception contains
                 * the position in the input where the error occurred, together with a diagnostic message and the last
                 * read input token
                */
                try {
                    json = nlohmann::json::parse (stream.str());
                }
                catch (nlohmann::json::parse_error& exception) {
                    LOG_WARNING (m_VKInstanceDataLog) << "Failed to import transform data "
                                                      << "[" << modelInfoId << "]"
                                                      << " "
                                                      << "[" << transformDataPath << "]"
                                                      << std::endl;
                    /* Set default transform data
                    */
                    instancesCount = 1;
                    modelInfo->meta.instances.resize      (instancesCount);
                    modelInfo->meta.transformDatas.resize (instancesCount);
                    modelInfo->meta.instancesCount       = instancesCount;
                    uint32_t modelInstanceId             = 0;

                    modelInfo->meta.transformDatas[modelInstanceId].position        = {0.0f, 0.0f, 0.0f};
                    modelInfo->meta.transformDatas[modelInstanceId].scale           = {1.0f, 1.0f, 1.0f};
                    modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg  = {0.0f, 0.0f, 0.0f};
                    modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier = 1.0f;
                    createModelMatrix (modelInfoId, modelInstanceId);
                }

                if (instancesCount == 0) {
                    instancesCount  = json["instancesCount"];
                    modelInfo->meta.instances.resize      (instancesCount);
                    modelInfo->meta.transformDatas.resize (instancesCount);
                    modelInfo->meta.instancesCount       = instancesCount;

                    for (auto const& instance: json["instances"]) {
                        uint32_t modelInstanceId = instance["id"];

                        modelInfo->meta.transformDatas[modelInstanceId].position        = {
                            instance["position"][0],
                            instance["position"][1],
                            instance["position"][2]
                        };
                        modelInfo->meta.transformDatas[modelInstanceId].scale           = {
                            instance["scale"][0],
                            instance["scale"][1],
                            instance["scale"][2]
                        };
                        modelInfo->meta.transformDatas[modelInstanceId].rotateAngleDeg  = {
                            instance["rotateAngleDeg"][0],
                            instance["rotateAngleDeg"][1],
                            instance["rotateAngleDeg"][2]
                        };
                        modelInfo->meta.transformDatas[modelInstanceId].scaleMultiplier = 1.0f;
                        createModelMatrix (modelInfoId, modelInstanceId);
                    }
                }
                return modelInfo->meta.instancesCount;
            }
    };
}   // namespace Core
#endif  // VK_INSTANCE_DATA_H