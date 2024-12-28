#ifndef UI_LIGHT_H
#define UI_LIGHT_H

#include "../../Core/Model/VKInstanceData.h"
#include "../../Core/Scene/VKSceneMgr.h"
#include "../../Core/Scene/VKLightMgr.h"
#include "../Wrapper/UIPrimitive.h"
#include "../Wrapper/UITree.h"

namespace Gui {
    class UILight: protected virtual Core::VKInstanceData,
                   protected virtual Core::VKSceneMgr,
                   protected virtual Core::VKLightMgr,
                   protected virtual UIPrimitive,
                   protected virtual UITree {
        private:
            struct UILightInfo {
                struct Meta {
                    uint32_t instanceId;
                    std::string label;
                } meta;
            };
            std::unordered_map <Core::e_lightType, std::vector <UILightInfo>> m_uiLightInfoPool;
            std::unordered_map <Core::e_lightType, std::vector <std::string>> m_lightInfoIdLabels;
            std::unordered_map <std::string, Core::e_lightType> m_typeLabelPool;
            std::vector <std::string> m_typeLabels;

            uint32_t selectedLightInfoIdLabelIdx;
            uint32_t selectedtypeLabelIdx;

            Log::Record* m_UILightLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UILight (void) {
                m_UILightLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UILight (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyContent (const std::vector <uint32_t>& lightInfoIds) {
                selectedLightInfoIdLabelIdx = 0;
                selectedtypeLabelIdx        = 0;

                for (auto const& infoId: lightInfoIds) {
                    auto lightInfo = getLightInfo (infoId);
                    auto type      = static_cast <Core::e_lightType> (infoId);
                    auto typeLabel = Core::getLightTypeString (type);

                    for (uint32_t i = 0; i < lightInfo->meta.instancesCount; i++) {
                        std::string label = "Instance ["  + std::to_string (i) + "]";
                        m_uiLightInfoPool[type].push_back ({{i, label}});
                    }

                    m_typeLabelPool[typeLabel] = type;
                }

                for (auto const& [type, infos]: m_uiLightInfoPool) {
                    for (auto const& info: infos)
                        m_lightInfoIdLabels[type].push_back (info.meta.label);
                }

                for (auto const& [label, type]: m_typeLabelPool)
                    m_typeLabels.push_back (label);
            }

            void createContent (uint32_t nodeInfoId,
                                bool& showShadow) {
                auto nodeInfo = getNodeInfo (nodeInfoId);

                Core::e_lightType type;
                /* Default data */
                bool fieldDisable = false;
                bool writePending = false;

                if ((nodeInfo->meta.type & LIGHT_NODE) && (nodeInfo->meta.type & INSTANCE_NODE)) {
                    /* Read data */
                    auto parentNodeInfo         = getNodeInfo (nodeInfo->meta.parentInfoId);
                    type                        = static_cast <Core::e_lightType> (parentNodeInfo->meta.coreInfoId);

                    std::string typeLabel       = Core::getLightTypeString (type);
                    selectedtypeLabelIdx        = std::find (m_typeLabels.begin(),
                                                             m_typeLabels.end(), typeLabel) -
                                                             m_typeLabels.begin();

                    uint32_t instanceId         = nodeInfo->meta.coreInfoId;
                    std::string instanceIdLabel = "Instance ["  + std::to_string (instanceId) + "]";
                    selectedLightInfoIdLabelIdx = std::find (m_lightInfoIdLabels[type].begin(),
                                                             m_lightInfoIdLabels[type].end(), instanceIdLabel) -
                                                             m_lightInfoIdLabels[type].begin();
                    fieldDisable                = true;
                }

                /* Show data */
                createCombo ("##lightType",
                             "Type",
                             "##postLabelLightType",
                             m_typeLabels,
                             fieldDisable,
                             g_styleSettings.size.inputFieldLarge,
                             selectedtypeLabelIdx);

                type = m_typeLabelPool[m_typeLabels[selectedtypeLabelIdx]];

                createCombo ("##lightInstance",
                             "Id",
                             "##postLabelLightInstance",
                             m_lightInfoIdLabels[type],
                             fieldDisable,
                             g_styleSettings.size.inputFieldLarge,
                             selectedLightInfoIdLabelIdx);

                /* Read data */
                auto lightInfo           = getLightInfo (type);
                auto infos               = m_uiLightInfoPool[type];
                auto iter                = std::next (infos.begin(), selectedLightInfoIdLabelIdx);
                uint32_t lightInstanceId = iter->meta.instanceId;

                glm::vec4 ambient        = lightInfo->meta.instances[lightInstanceId].ambient;
                glm::vec4 diffuse        = lightInfo->meta.instances[lightInstanceId].diffuse;
                glm::vec4 specular       = lightInfo->meta.instances[lightInstanceId].specular;

                float constant           = lightInfo->meta.instances[lightInstanceId].constant;
                float linear             = lightInfo->meta.instances[lightInstanceId].linear;
                float quadratic          = lightInfo->meta.instances[lightInstanceId].quadratic;

                float innerRadiusDeg;
                float outerRadiusDeg;
                getLightRadiusDeg (type, lightInstanceId, innerRadiusDeg, outerRadiusDeg);
                /* Convert glm::vec4 to ImVec4
                */
                ImVec4 ambientColor      = { ambient.x,  ambient.y,  ambient.z,  ambient.w};
                ImVec4 diffuseColor      = { diffuse.x,  diffuse.y,  diffuse.z,  diffuse.w};
                ImVec4 specularColor     = {specular.x, specular.y, specular.z, specular.w};

                /* Show data */
                createSeparatorText ("Color");
                createColorButton   ("##ambient",
                                     "Ambient",
                                     false,
                                     g_styleSettings.size.inputFieldLarge,
                                     ambientColor);

                createColorButton   ("##diffuse",
                                     "Diffuse",
                                     false,
                                     g_styleSettings.size.inputFieldLarge,
                                     diffuseColor);

                createColorButton   ("##specular",
                                     "Specular",
                                     false,
                                     g_styleSettings.size.inputFieldLarge,
                                     specularColor);
                /* Convert ImVec4 to glm::vec4
                */
                ambient     = { ambientColor.x,  ambientColor.y,  ambientColor.z,  ambientColor.w};
                diffuse     = { diffuseColor.x,  diffuseColor.y,  diffuseColor.z,  diffuseColor.w};
                specular    = {specularColor.x, specularColor.y, specularColor.z, specularColor.w};
                /* Write data */
                lightInfo->meta.instances[lightInstanceId].ambient  = ambient;
                lightInfo->meta.instances[lightInstanceId].diffuse  = diffuse;
                lightInfo->meta.instances[lightInstanceId].specular = specular;
                /* Set anchor color (with diffuse component)
                */
                updateTexIdLUT (type, lightInstanceId, Core::DIFFUSE_TEXTURE, 0,  diffuse.x * UINT8_MAX);
                updateTexIdLUT (type, lightInstanceId, Core::DIFFUSE_TEXTURE, 4,  diffuse.y * UINT8_MAX);
                updateTexIdLUT (type, lightInstanceId, Core::DIFFUSE_TEXTURE, 8,  diffuse.z * UINT8_MAX);
                updateTexIdLUT (type, lightInstanceId, Core::DIFFUSE_TEXTURE, 12, diffuse.w * UINT8_MAX);

                /* Show data */
                createSeparatorText      ("Attenuation");
                if (createFloatTextField ("##constant",
                                          "Constant",
                                          "u",
                                          g_styleSettings.precision,
                                          false,
                                          g_styleSettings.size.inputFieldSmall,
                                          constant) ||

                    createFloatTextField ("##linear",
                                          "Linear",
                                          "u",
                                          g_styleSettings.precision,
                                          false,
                                          g_styleSettings.size.inputFieldSmall,
                                          linear)   ||

                    createFloatTextField ("##quadratic",
                                          "Quadratic",
                                          "u",
                                          g_styleSettings.precision,
                                          false,
                                          g_styleSettings.size.inputFieldSmall,
                                          quadratic))
                    writePending = true;

                /* Write data */
                if (writePending) {
                    auto lightInfo           = getLightInfo (type);
                    auto infos               = m_uiLightInfoPool[type];
                    auto iter                = std::next (infos.begin(), selectedLightInfoIdLabelIdx);
                    uint32_t lightInstanceId = iter->meta.instanceId;

                    lightInfo->meta.instances[lightInstanceId].constant  = constant;
                    lightInfo->meta.instances[lightInstanceId].linear    = linear;
                    lightInfo->meta.instances[lightInstanceId].quadratic = quadratic;
                }

                /* Reset data */
                writePending = false;
                if (type == Core::SPOT_LIGHT)   fieldDisable = false;
                else                            fieldDisable = true;

                /* Default data */
                /* Note that the radii vector has to be in decending order since the create shape function uses the
                 * first radius in the vector to set the cursor position offsets. Also, note that the radii are stored
                 * as fractions of the max radius in order for easier visualization
                */
                float maxRadius     = 64.0f;
                float segmentsCount = 64.0f;
                /* Read data */
                auto radii          = std::vector <float>  {
                    maxRadius,
                    maxRadius * (innerRadiusDeg/outerRadiusDeg)
                };
                auto colors         = std::vector <ImVec4> {
                    g_styleSettings.color.frameBackground,
                    g_styleSettings.color.frameBackgroundHovered
                };

                /* Show data */
                createSeparatorText      ("Radius");
                createMultiCirclesShape  (segmentsCount,
                                          g_styleSettings.padding.separatorText.y,
                                          fieldDisable,
                                          radii,
                                          colors);

                if (createFloatTextField ("##innerRadius",
                                          "Inner",
                                          "deg",
                                          g_styleSettings.precision,
                                          fieldDisable,
                                          g_styleSettings.size.inputFieldSmall,
                                          innerRadiusDeg)   ||

                    createFloatTextField ("##outerRadius",
                                          "Outer",
                                          "deg",
                                          g_styleSettings.precision,
                                          fieldDisable,
                                          g_styleSettings.size.inputFieldSmall,
                                          outerRadiusDeg))
                    writePending = true;

                /* Write data */
                if (writePending) {
                    auto infos               = m_uiLightInfoPool[type];
                    auto iter                = std::next (infos.begin(), selectedLightInfoIdLabelIdx);
                    uint32_t lightInstanceId = iter->meta.instanceId;

                    setLightRadiusDeg (type, lightInstanceId, innerRadiusDeg, outerRadiusDeg);
                }

                /* Show data */
                createCheckBoxButton ("##shadow",
                                      "Shadow",
                                      "##postLabelShadow",
                                      true,
                                      showShadow);
            }
    };
}   // namespace Gui
#endif  // UI_LIGHT_H