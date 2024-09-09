#ifndef VK_INSTANCE_DATA_H
#define VK_INSTANCE_DATA_H

#include <nlohmann/json.hpp>
#include "VKModelMatrix.h"

using namespace Collections;

namespace Core {
    class VKInstanceData: protected VKModelMatrix {
        private:
            Log::Record* m_VKInstanceDataLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++; 

        public:
            VKInstanceData (void) {
                m_VKInstanceDataLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
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
                /* Read and parse json file
                */
                std::ifstream fJson (instanceDataPath);
                std::stringstream stream;

                stream << fJson.rdbuf();
                auto json = nlohmann::json::parse (stream.str());

                uint32_t instancesCount  = json["instancesCount"];
                uint32_t modelInstanceId = 0;
                glm::vec3 translate      = {0.0f, 0.0f, 0.0f};
                glm::vec3 rotateAxis     = {0.0f, 1.0f, 0.0f};
                glm::vec3 scale          = {1.0f, 1.0f, 1.0f};
                float rotateAngleDeg     = 0.0f;

                if (instancesCount == 0) {
                    LOG_WARNING (m_VKInstanceDataLog) << "Failed to import instance data "
                                                      << "[" << modelInfoId << "]"
                                                      << " "
                                                      << "[" << instanceDataPath << "]"
                                                      << std::endl;
                    /* Set default instance data
                    */
                    modelInfo->meta.instances.resize (1);
                    modelInfo->meta.instancesCount = 1;
                    createModelMatrix (modelInfoId, 
                                       modelInstanceId, 
                                       translate, 
                                       rotateAxis, 
                                       scale,
                                       rotateAngleDeg);
                }
                else {
                    modelInfo->meta.instances.resize (instancesCount);
                    modelInfo->meta.instancesCount = instancesCount;

                    for (auto const& instance: json["instances"]) {
                        translate  = {instance["translate"][0],  instance["translate"][1],  instance["translate"][2]};
                        rotateAxis = {instance["rotateAxis"][0], instance["rotateAxis"][1], instance["rotateAxis"][2]};
                        scale      = {instance["scale"][0],      instance["scale"][1],      instance["scale"][2]};

                        modelInstanceId = instance["id"];
                        rotateAngleDeg  = instance["rotateAngleDeg"]; 
#if ENABLE_PARSED_INSTANCE_DATA_DUMP
                        LOG_INFO (m_VKInstanceDataLog) << "Model instance id "
                                                       << "[" << modelInstanceId << "]"
                                                       << std::endl;

                        LOG_INFO (m_VKInstanceDataLog) << "Translate "
                                                       << "[" << translate.x << ", "
                                                              << translate.y << ", "
                                                              << translate.z  
                                                       << "]"
                                                       << std::endl;

                        LOG_INFO (m_VKInstanceDataLog) << "Rotate axis "
                                                       << "[" << rotateAxis.x << ", "
                                                              << rotateAxis.y << ", "
                                                              << rotateAxis.z  
                                                       << "]"
                                                       << std::endl;

                        LOG_INFO (m_VKInstanceDataLog) << "Scale "
                                                       << "[" << scale.x << ", "
                                                              << scale.y << ", "
                                                              << scale.z  
                                                       << "]"
                                                       << std::endl;  

                        LOG_INFO (m_VKInstanceDataLog) << "Rotate angle deg "
                                                       << "[" << rotateAngleDeg << "]"
                                                       << std::endl;
#endif  // ENABLE_PARSED_INSTANCE_DATA_DUMP
                        createModelMatrix (modelInfoId,
                                           modelInstanceId, 
                                           translate, 
                                           rotateAxis, 
                                           scale,
                                           rotateAngleDeg);
                    }
                }
                return modelInfo->meta.instancesCount;
            }
    };
}   // namespace Core
#endif  // VK_INSTANCE_DATA_H