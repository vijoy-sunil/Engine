#ifndef EN_GENERIC_CONTROL_H
#define EN_GENERIC_CONTROL_H

#include "../../Core/Device/VKDeviceMgr.h"
#include "../../Utils/UserInput.h"
#include "../Config/ENEnvConfig.h"

using namespace Collections;

namespace SandBox {
    class ENGenericControl: protected virtual Core::VKDeviceMgr,
                            protected virtual Utils::UserInput {
        private:
            uint32_t m_deviceInfoId;

            Log::Record* m_ENGenericControlLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

            void exitWindow (float deltaTime) {
                static_cast <void> (deltaTime);

                auto deviceInfo = getDeviceInfo (m_deviceInfoId);
                LOG_WARNING (m_ENGenericControlLog) << "Received exit window input"
                                                    << std::endl;
                /* This function sets the value of the close flag of the specified window. This can be used to override 
                 * the user's attempt to close the window, or to signal that it should be closed
                */
                glfwSetWindowShouldClose (deviceInfo->resource.window, GL_TRUE);
            }

        public:
            ENGenericControl (void) {
                m_ENGenericControlLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~ENGenericControl (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyGenericControl (uint32_t deviceInfoId) {
                m_deviceInfoId = deviceInfoId;

                createKeyEventBinding (g_coreSettings.keyMap.exitWindow, [this](float deltaTime) {
                    this->exitWindow  (deltaTime);
                });
            }
    };
}   // namespace SandBox
#endif  // EN_GENERIC_CONTROL_H