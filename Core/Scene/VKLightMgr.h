#ifndef VK_LIGHT_MGR_H
#define VK_LIGHT_MGR_H

#include "VKUniform.h"
#include "../VKLogHelper.h"
#include "../VKConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Core {
    class VKLightMgr {
        private:
            struct LightInfo {
                struct Meta {
                    std::vector <LightInstanceDataSSBO> instances;
                    uint32_t instancesCount;
                } meta;
            };
            std::unordered_map <uint32_t, LightInfo> m_lightInfoPool;

            Log::Record* m_VKLightMgrLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deleteLightInfo (uint32_t lightInfoId) {
                if (m_lightInfoPool.find (lightInfoId) != m_lightInfoPool.end()) {
                    m_lightInfoPool.erase (lightInfoId);
                    return;
                }

                LOG_ERROR (m_VKLightMgrLog) << "Failed to delete light info "
                                            << "[" << lightInfoId << "]"
                                            << std::endl;
                throw std::runtime_error ("Failed to delete light info");
            }

        public:
            VKLightMgr (void) {
                m_VKLightMgrLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKLightMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* When we define a light source, we want to give this light source a color. If we would then multiply the
             * light source's color with an object's color value (a component-wise multiplication between the light and
             * object color vectors), the resulting color would be the reflected color of the object (and thus its
             * perceived color)
             *
             * Lighting in the real world is extremely complicated and depends on way too many factors, something we
             * can't afford to calculate on the limited processing power we have. It is therefore based on approximations
             * of reality using simplified models that are much easier to process and look relatively similar. These
             * lighting models are based on the physics of light as we understand it. One of those models is called the
             * Phong lighting model. The major building blocks of the Phong lighting model consist of 3 components:
             *
             * (1) Ambient
             * Even when it is dark there is usually still some light somewhere in the world (the moon, a distant light)
             * so objects are almost never completely dark. To simulate this we use an ambient lighting constant that
             * always gives the object some color
             *
             * (2) Diffuse
             * This simulates the directional impact a light object has on an object. This is the most visually
             * significant component of the lighting model. The more a part of an object faces the light source, the
             * brighter it becomes
             *
             * (3) Specular
             * This simulates the bright spot of a light that appears on shiny objects. Specular highlights are more
             * inclined to the color of the light than the color of the object
            */
            void readyLightInfo (uint32_t lightInfoId) {
                if (m_lightInfoPool.find (lightInfoId) != m_lightInfoPool.end()) {
                    LOG_ERROR (m_VKLightMgrLog) << "Light info id already exists "
                                                << "[" << lightInfoId << "]"
                                                << std::endl;
                    throw std::runtime_error ("Light info id already exists");
                }

                LightInfo info{};
                m_lightInfoPool[lightInfoId] = info;
            }

            /* Note that, the cut off radius values are not stored in degrees, instead we calculate the cosine value
             * based on an angle and pass the cosine result to the fragment shader. The reason for this is that in the
             * fragment shader we're calculating the dot product between the light direction and the spot direction
             * vector and the dot product returns a cosine value and not an angle; and we can't directly compare an
             * angle with a cosine value. To get the angle in the shader we then have to calculate the inverse cosine
             * of the dot product's result which is an expensive operation
             *
             * So to save some performance we calculate the cosine value of a given cut off angle before hand and pass
             * this result to the fragment shader. Since both angles are now represented as cosines, we can directly
             * compare between them without expensive operations
            */
            void setLightRadiusDeg (uint32_t lightInfoId,
                                    uint32_t lightInstanceId,
                                    float innerRadiusDeg, float outerRadiusDeg) {

                auto lightInfo = getLightInfo (lightInfoId);
                if (lightInstanceId >= lightInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKLightMgrLog) << "Invalid light instance id "
                                                << "[" << lightInstanceId << "]"
                                                << "->"
                                                << "[" << lightInfo->meta.instancesCount << "]"
                                                << std::endl;
                    throw std::runtime_error ("Invalid light instance id");
                }

                lightInfo->meta.instances[lightInstanceId].innerRadiusCosine = glm::cos (glm::radians (innerRadiusDeg));
                lightInfo->meta.instances[lightInstanceId].outerRadiusCosine = glm::cos (glm::radians (outerRadiusDeg));
            }

            void getLightRadiusDeg (uint32_t lightInfoId,
                                    uint32_t lightInstanceId,
                                    float& innerRadiusDeg, float& outerRadiusDeg) {

                auto lightInfo = getLightInfo (lightInfoId);
                if (lightInstanceId >= lightInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKLightMgrLog) << "Invalid light instance id "
                                                << "[" << lightInstanceId << "]"
                                                << "->"
                                                << "[" << lightInfo->meta.instancesCount << "]"
                                                << std::endl;
                    throw std::runtime_error ("Invalid light instance id");
                }

                innerRadiusDeg = glm::degrees (glm::acos (lightInfo->meta.instances[lightInstanceId].innerRadiusCosine));
                outerRadiusDeg = glm::degrees (glm::acos (lightInfo->meta.instances[lightInstanceId].outerRadiusCosine));
            }

            LightInfo* getLightInfo (uint32_t lightInfoId) {
                if (m_lightInfoPool.find (lightInfoId) != m_lightInfoPool.end())
                    return &m_lightInfoPool[lightInfoId];

                LOG_ERROR (m_VKLightMgrLog) << "Failed to find light info "
                                            << "[" << lightInfoId << "]"
                                            << std::endl;
                throw std::runtime_error ("Failed to find light info");
            }

            void dumpLightInfoPool (void) {
                LOG_INFO (m_VKLightMgrLog) << "Dumping light info pool"
                                           << std::endl;

                for (auto const& [key, val]: m_lightInfoPool) {
                    LOG_INFO (m_VKLightMgrLog) << "Type "
                                               << "[" << getLightTypeString (static_cast <e_lightType> (key)) << "]"
                                               << std::endl;

                    uint32_t lightInstanceId = 0;
                    for (auto const& instance: val.meta.instances) {
                        LOG_INFO (m_VKLightMgrLog) << "Light instance id "
                                                   << "[" << lightInstanceId << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Position "
                                                   << "[" << instance.position.x << ", "
                                                          << instance.position.y << ", "
                                                          << instance.position.z
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Direction "
                                                   << "[" << instance.direction.x << ", "
                                                          << instance.direction.y << ", "
                                                          << instance.direction.z
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Ambient "
                                                   << "[" << instance.ambient.x << ", "
                                                          << instance.ambient.y << ", "
                                                          << instance.ambient.z << ", "
                                                          << instance.ambient.w
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Diffuse "
                                                   << "[" << instance.diffuse.x << ", "
                                                          << instance.diffuse.y << ", "
                                                          << instance.diffuse.z << ", "
                                                          << instance.diffuse.w
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Specular "
                                                   << "[" << instance.specular.x << ", "
                                                          << instance.specular.y << ", "
                                                          << instance.specular.z << ", "
                                                          << instance.specular.w
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Constant attenuation "
                                                   << "[" << instance.constant << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Linear attenuation "
                                                   << "[" << instance.linear << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Quadratic attenuation "
                                                   << "[" << instance.quadratic << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Inner radius cosine"
                                                   << "[" << instance.innerRadiusCosine << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKLightMgrLog) << "Outer radius cosine"
                                                   << "[" << instance.outerRadiusCosine << "]"
                                                   << std::endl;
                    }
                }
            }

            void cleanUp (uint32_t lightInfoId) {
                deleteLightInfo (lightInfoId);
            }
    };
}   // namespace Core
#endif  // VK_LIGHT_MGR_H