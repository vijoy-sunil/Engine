#ifndef UI_UTIL_H
#define UI_UTIL_H

#include "UIConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Gui {
    class UIUtil {
        private:
            Log::Record* m_UIUtilLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIUtil (void) {
                m_UIUtilLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIUtil (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* In order to differentiate between mouse/keyboard usage in imgui window vs the application window, we can
             * use the boolean flags provided by imgui. When WantCaptureMouse is set, you need to discard/hide the mouse
             * inputs from your underlying application, similarly, when WantCaptureKeyboard is set, you need to discard/
             * hide the keyboard inputs from your underlying application. Generally you may always pass all inputs to
             * imgui, and hide them from your application based on these flags
            */
            bool isMouseCapturedByUI (void) {
                auto& io = ImGui::GetIO();
                return io.WantCaptureMouse;
            }

            bool isKeyBoardCapturedByUI (void) {
                auto& io = ImGui::GetIO();
                return io.WantCaptureKeyboard;
            }

            void disableMouseInputsToUI (void) {
                auto& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }

            void enableMouseInputsToUI (void) {
                auto& io = ImGui::GetIO();
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
    };
}   // namespace Gui
#endif  // UI_UTIL_H