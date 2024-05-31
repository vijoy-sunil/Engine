#ifndef RD_APP_H
#define RD_APP_H

#include "../Config/VKConfig.h"
#include "../Core/VKRun.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class RDApp: protected VKConfig,
                 protected VKRun {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_RDAppLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 0; 

        public:
            RDApp (void) {
                m_RDAppLog = LOG_INIT (m_instanceId, 
                                       Log::VERBOSE,
                                       Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                       "./Build/Log/");
                LOG_INFO (m_RDAppLog) << "Hello World!" << std::endl;
            }

            ~RDApp (void) {
                LOG_CLOSE (m_instanceId);
            }

            void runApp (void) {
                runSequence();
            }
    };

    Log::Record* RDApp::m_RDAppLog;
}   // namespace Renderer
#endif  // RD_APP_H