#ifndef EN_GENERIC_H
#define EN_GENERIC_H

#include "../../Core/Device/VKDeviceMgr.h"
#include "../../Gui/UIInput.h"
#include "../../Gui/UIUtil.h"
#include "../ENConfig.h"

namespace SandBox {
    class ENGeneric: protected virtual Core::VKDeviceMgr,
                     protected virtual Gui::UIInput,
                     protected virtual Gui::UIUtil {
        private:
            uint32_t m_deviceInfoId;

            Log::Record* m_ENGenericLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void exitWindow (float deltaTime) {
                if (isKeyBoardCapturedByUI())
                    return;

                static_cast <void> (deltaTime);
                auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                LOG_WARNING (m_ENGenericLog) << "Received exit window input"
                                             << std::endl;
                /* This function sets the value of the close flag of the specified window. This can be used to override
                 * the user's attempt to close the window, or to signal that it should be closed
                */
                glfwSetWindowShouldClose (deviceInfo->resource.window, GL_TRUE);
            }

        public:
            ENGeneric (void) {
                m_ENGenericLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~ENGeneric (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyGenericController (uint32_t deviceInfoId) {
                m_deviceInfoId = deviceInfoId;

                createKeyEventBinding (g_keyMapSettings.exitWindow, [this](float deltaTime) {
                    this->exitWindow  (deltaTime);
                });
            }
    };
}   // namespace SandBox
#endif  // EN_GENERIC_H