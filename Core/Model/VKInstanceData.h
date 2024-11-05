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

                bool oldTexIdValid = false;
                bool newTexIdValid = false;
                for (auto const& [path, infoId]: getTextureImagePool()) {
                    if (infoId == oldTexId) oldTexIdValid = true;
                    if (infoId == newTexId) newTexIdValid = true;
                }

                if (!oldTexIdValid || !newTexIdValid) {
                    LOG_WARNING (m_VKInstanceDataLog) << "Invalid texture id "
                                                      << "[" << oldTexId << "]"
                                                      << " "
                                                      << "[" << newTexId << "]"
                                                      << std::endl;
                }
                else {
                    const uint32_t numColumns = 4;
                    uint32_t rowIdx = oldTexId / numColumns;
                    uint32_t colIdx = oldTexId % numColumns;

                    modelInfo->meta.instances[modelInstanceId].texIdLUT[rowIdx][colIdx] = newTexId;
                }
            }

            uint32_t importInstanceData (uint32_t modelInfoId, const char* instanceDataPath) {
                auto modelInfo = getModelInfo (modelInfoId);
                uint32_t instancesCount = 0;
                /* Read and parse json file
                */
                std::ifstream fJson (instanceDataPath);
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
                    LOG_WARNING (m_VKInstanceDataLog) << "Failed to import instance data "
                                                      << "[" << modelInfoId << "]"
                                                      << " "
                                                      << "[" << instanceDataPath << "]"
                                                      << std::endl;
                    /* Set default instance data
                    */
                    instancesCount = 1;
                    modelInfo->meta.instances.resize     (instancesCount);
                    modelInfo->meta.instanceDatas.resize (instancesCount);
                    modelInfo->meta.instancesCount      = instancesCount;
                    uint32_t modelInstanceId            = 0;

                    modelInfo->meta.instanceDatas[modelInstanceId].position       = {0.0f, 0.0f, 0.0f};
                    modelInfo->meta.instanceDatas[modelInstanceId].rotateAxis     = {0.0f, 1.0f, 0.0f};
                    modelInfo->meta.instanceDatas[modelInstanceId].scale          = {1.0f, 1.0f, 1.0f};
                    modelInfo->meta.instanceDatas[modelInstanceId].rotateAngleDeg = 0.0f;
                    createModelMatrix (modelInfoId, modelInstanceId);
                }

                if (instancesCount == 0) {
                    instancesCount  = json["instancesCount"];
                    modelInfo->meta.instances.resize     (instancesCount);
                    modelInfo->meta.instanceDatas.resize (instancesCount);
                    modelInfo->meta.instancesCount      = instancesCount;

                    for (auto const& instance: json["instances"]) {
                        uint32_t modelInstanceId = instance["id"];

                        modelInfo->meta.instanceDatas[modelInstanceId].position       = {
                            instance["position"][0],
                            instance["position"][1],
                            instance["position"][2]
                        };
                        modelInfo->meta.instanceDatas[modelInstanceId].rotateAxis     = {
                            instance["rotateAxis"][0],
                            instance["rotateAxis"][1],
                            instance["rotateAxis"][2]
                        };
                        modelInfo->meta.instanceDatas[modelInstanceId].scale          = {
                            instance["scale"][0],
                            instance["scale"][1],
                            instance["scale"][2]
                        };
                        modelInfo->meta.instanceDatas[modelInstanceId].rotateAngleDeg = instance["rotateAngleDeg"];
                        createModelMatrix (modelInfoId, modelInstanceId);
                    }
                }
                return modelInfo->meta.instancesCount;
            }
    };
}   // namespace Core
#endif  // VK_INSTANCE_DATA_H