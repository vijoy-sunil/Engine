#ifndef UI_PRIMITIVE_H
#define UI_PRIMITIVE_H

#include "../UIConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Gui {
    class UIPrimitive {
        private:
            Log::Record* m_UIPrimitiveLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIPrimitive (void) {
                m_UIPrimitiveLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIPrimitive (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createColorButton (const char* stringId,
                                    const char* label,
                                    bool buttonDisable,
                                    float buttonWidth,
                                    ImVec4& color) {

                auto colorEditFlags = ImGuiColorEditFlags_NoBorder      |
                                      ImGuiColorEditFlags_NoTooltip     |
                                      ImGuiColorEditFlags_AlphaBar      |
                                      ImGuiColorEditFlags_AlphaPreview  |
                                      ImGuiColorEditFlags_NoSidePreview;

                ImGui::Text             ("%s", label);
                ImGui::SameLine         (g_styleSettings.alignment.inputField);

                ImGui::BeginDisabled    (buttonDisable);
                if (ImGui::ColorButton  (stringId, color, colorEditFlags, ImVec2 (buttonWidth, 0.0f))) {
                    ImGui::OpenPopup    (stringId);
                }
                if (ImGui::BeginPopup   (stringId)) {
                    ImGui::ColorPicker4 (stringId, (float*) &color, colorEditFlags);
                    ImGui::EndPopup();
                }
                ImGui::EndDisabled();
            }

            void createCheckBoxButton (const char* stringId,
                                       const char* preLabel,
                                       const char* postLabel,
                                       bool buttonDisable,
                                       bool& selected) {

                ImGui::PushID        (stringId);
                ImGui::Text          ("%s", preLabel);
                ImGui::SameLine      (g_styleSettings.alignment.inputField);

                ImGui::BeginDisabled (buttonDisable);
                ImGui::Checkbox      (postLabel, &selected);
                ImGui::EndDisabled();
                ImGui::PopID();
            }

            bool createFloatTextField (const char* stringId,
                                       const char* preLabel,
                                       const char* postLabel,
                                       const char* precision,
                                       bool fieldDisable,
                                       float fieldWidth,
                                       float& fieldValue) {

                bool enterPressed;
                ImGui::PushID        (stringId);
                ImGui::Text          ("%s", preLabel);
                ImGui::SameLine      (g_styleSettings.alignment.inputField);

                ImGui::BeginDisabled (fieldDisable);
                ImGui::PushItemWidth (fieldWidth);
                enterPressed = ImGui::InputFloat (postLabel,
                                                  &fieldValue,
                                                  0.0f, 0.0f, precision,
                                                  ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopItemWidth();
                ImGui::EndDisabled();
                ImGui::PopID();
                return enterPressed;
            }

            bool createCombo (const char* stringId,
                              const char* preLabel,
                              const char* postLabel,
                              const std::vector <std::string>& labels,
                              bool fieldDisable,
                              float fieldWidth,
                              uint32_t& selectedLabelIdx) {

                bool selectionUpdated = false;
                ImGui::PushID         (stringId);
                ImGui::Text           ("%s", preLabel);
                ImGui::PopID();
                ImGui::SameLine       (g_styleSettings.alignment.inputField);

                ImGui::BeginDisabled  (fieldDisable);
                ImGui::PushItemWidth  (fieldWidth);
                ImGui::PushStyleVar   (ImGuiStyleVar_ItemSpacing, g_styleSettings.spacing.list);
                if (ImGui::BeginCombo (postLabel, labels[selectedLabelIdx].c_str(), ImGuiComboFlags_HeightRegular)) {

                    for (size_t i = 0; i < labels.size(); i++) {
                        bool selected = (selectedLabelIdx == i);

                        if (ImGui::Selectable (labels[i].c_str(), selected)) {
                            /* Set boolean only if new selection is different from the previous one
                            */
                            if (selectedLabelIdx != i) {
                                selectionUpdated = true;
                                selectedLabelIdx = static_cast <uint32_t> (i);
                            }
                        }
                        /* Set the initial focus when opening the combo
                        */
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
                ImGui::EndDisabled();
                return selectionUpdated;
            }

            void createImagePreview (VkDescriptorSet imageDescriptorSet,
                                     ImVec2 imageSize,
                                     ImVec4 borderColor,
                                     ImVec2 uvMin = ImVec2 (0.0f, 0.0f),
                                     ImVec2 uvMax = ImVec2 (1.0f, 1.0f)) {

                ImGui::SetCursorPosX ((ImGui::GetContentRegionAvail().x - imageSize.x) * 0.5f);
                ImGui::Image         ((ImTextureID) imageDescriptorSet,
                                      imageSize,
                                      uvMin, uvMax,
                                      ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                                      borderColor);
            }

            void createVerticalTabs (const std::vector <const char*>& icons,
                                     const std::vector <const char*>& labels,
                                     ImVec2 tabSize,
                                     ImVec4 tabActiveColor,
                                     ImVec4 tabInactiveColor,
                                     uint32_t& selectedLabelIdx) {

                /* Note that, cell padding for x axis is locked in BeginTable() and cannot be changed afterwards, and,
                 * cell padding for y axis is locked in TableNextRow() so if you want to change it for a row you need
                 * to change it before TableNextRow()
                */
                ImGui::PushStyleVar   (ImGuiStyleVar_CellPadding, ImVec2 (0.0f, 0.0f));
                if (ImGui::BeginTable ("##table", 1, 0, ImVec2 (0.0f, 0.0f))) {

                    for (size_t i = 0; i < icons.size(); i++) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        bool selected = (selectedLabelIdx == i);

                        ImGui::PushID (static_cast <int> (i));
                        if (selected) ImGui::PushStyleColor (ImGuiCol_Button, tabActiveColor);
                        else          ImGui::PushStyleColor (ImGuiCol_Button, tabInactiveColor);

                        if (ImGui::Button (icons[i], tabSize))
                            selectedLabelIdx = static_cast <uint32_t> (i);

                        if (ImGui::IsItemHovered (ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay))
                            ImGui::SetTooltip ("%s", labels[i]);

                        ImGui::PopStyleColor();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
    };
}   // namespace Gui
#endif  // UI_PRIMITIVE_H