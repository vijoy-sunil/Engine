#ifndef UI_DEBUG_H
#define UI_DEBUG_H

#include "../Wrapper/UIPrimitive.h"
#include "../Wrapper/UITree.h"

namespace Gui {
    class UIDebug: protected virtual UIPrimitive,
                   protected virtual UITree {
        private:
            Log::Record* m_UIDebugLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIDebug (void) {
                m_UIDebugLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIDebug (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createContent (void) {
            }
    };
}   // namespace Gui
#endif  // UI_DEBUG_H