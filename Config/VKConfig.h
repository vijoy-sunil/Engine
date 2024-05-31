#ifndef VK_CONFIG_H
#define VK_CONFIG_H

#include "../Collections/Log/include/Log.h"

using namespace Collections;

namespace Renderer {
    class VKConfig {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKConfigLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 28; 

        public:
            VKConfig (void) {
                m_VKConfigLog = LOG_INIT (m_instanceId, 
                                          Log::VERBOSE,
                                          Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                          "./Build/Log/"); 
            }

            ~VKConfig (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void configVertexData (void) {
                /* TODO
                */
            }

            void configIndexData (void) {
                /* TODO
                */
            }
    };

    Log::Record* VKConfig::m_VKConfigLog;
}   // namespace Renderer
#endif  // VK_CONFIG_H