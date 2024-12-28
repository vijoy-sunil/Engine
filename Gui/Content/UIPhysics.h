#ifndef UI_PHYSICS_H
#define UI_PHYSICS_H

#include "../Wrapper/UIPrimitive.h"

namespace Gui {
    class UIPhysics: protected virtual UIPrimitive {
        private:
            Log::Record* m_UIPhysicsLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

        public:
            UIPhysics (void) {
                m_UIPhysicsLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~UIPhysics (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void createContent (bool& showBoundingBox) {
                /* Show data */
                createCheckBoxButton ("##boundingBox",
                                      "Bounding box",
                                      "##postLabelBoundingBox",
                                      true,
                                      showBoundingBox);
            }
    };
}   // namespace Gui
#endif  // UI_PHYSICS_H