#ifndef UI_OVERLAY_H
#define UI_OVERLAY_H

#include "../UIConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Gui {
    class UIOverlay {
        private:
            ImGuiWindowFlags m_windowFlags;

            Log::Record* m_UIOverlayLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIOverlay (void) {
                m_windowFlags  = ImGuiWindowFlags_NoDecoration          |
                                 ImGuiWindowFlags_AlwaysAutoResize      |
                                 ImGuiWindowFlags_NoSavedSettings       |
                                 ImGuiWindowFlags_NoFocusOnAppearing    |
                                 ImGuiWindowFlags_NoNav;

                m_UIOverlayLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIOverlay (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            template <typename T>
            void createOverlay (const char* stringId,
                                const char* label,
                                ImVec2 padding,
                                e_overlayLocation& location,
                                T content) {

                if (location == CUSTOM)
                    m_windowFlags &= ~ImGuiWindowFlags_NoMove;

                else if (location == CENTER) {
                    ImGui::SetNextWindowPos (ImGui::GetMainViewport()->GetCenter(),
                                             ImGuiCond_Always,
                                             ImVec2 (0.5f, 0.5f));
                    m_windowFlags |= ImGuiWindowFlags_NoMove;
                }

                else {
                    auto viewport    = ImGui::GetMainViewport();
                    /* Use work area to avoid menu-bar/task-bar, if any
                    */
                    ImVec2 workPos   = viewport->WorkPos;
                    ImVec2 workSize  = viewport->WorkSize;

                    ImVec2 windowPos;
                    windowPos.x      = (location == TOP_RIGHT   || location == BOTTOM_RIGHT) ?
                                       (workPos.x + workSize.x - padding.x):
                                       (workPos.x + padding.x);

                    windowPos.y      = (location == BOTTOM_LEFT || location == BOTTOM_RIGHT) ?
                                       (workPos.y + workSize.y - padding.y):
                                       (workPos.y + padding.y);

                    ImVec2 windowPosPivot;
                    windowPosPivot.x = (location == TOP_RIGHT   || location == BOTTOM_RIGHT) ? 1.0f: 0.0f;
                    windowPosPivot.y = (location == BOTTOM_LEFT || location == BOTTOM_RIGHT) ? 1.0f: 0.0f;

                    ImGui::SetNextWindowPos (windowPos,
                                             ImGuiCond_Always,
                                             windowPosPivot);
                    m_windowFlags   |= ImGuiWindowFlags_NoMove;
                }

                ImGui::SetNextWindowBgAlpha (g_styleSettings.alpha.overlay);
                ImGui::PushStyleVar         (ImGuiStyleVar_ItemSpacing, g_styleSettings.spacing.list);
                ImGui::Begin                (stringId, nullptr, m_windowFlags);

                ImGui::Text ("%s", label);
                ImGui::Separator();
                content();

                if (ImGui::BeginPopupContextWindow()) {
                    if (ImGui::MenuItem ("Custom",       nullptr, location == CUSTOM))       location = CUSTOM;
                    if (ImGui::MenuItem ("Center",       nullptr, location == CENTER))       location = CENTER;
                    if (ImGui::MenuItem ("Top left",     nullptr, location == TOP_LEFT))     location = TOP_LEFT;
                    if (ImGui::MenuItem ("Top right",    nullptr, location == TOP_RIGHT))    location = TOP_RIGHT;
                    if (ImGui::MenuItem ("Bottom left",  nullptr, location == BOTTOM_LEFT))  location = BOTTOM_LEFT;
                    if (ImGui::MenuItem ("Bottom right", nullptr, location == BOTTOM_RIGHT)) location = BOTTOM_RIGHT;
                    ImGui::EndPopup();
                }
                ImGui::End();
                ImGui::PopStyleVar();
            }
    };
}   // namespace Gui
#endif  // UI_OVERLAY_H