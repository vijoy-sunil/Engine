#ifndef RECORD_MGR_H
#define RECORD_MGR_H

#include "Record.h"

namespace Collections {
namespace Log {
    class RecordMgr: public Admin::InstanceMgr {
        public:
            Record* initRecord (uint32_t instanceId, 
                                std::string callingFile,
                                std::string saveDir       = "",
                                size_t bufferCapacity     = 0,
                                const char* format        = ".txt") {
            
                /* Add record object to pool
                */
                if (m_instancePool.find (instanceId) == m_instancePool.end()) {
                    Record* c_record = new Record (instanceId, 
                                                   callingFile,
                                                   saveDir,
                                                   bufferCapacity,
                                                   format);

                    Admin::NonTemplateBase* c_instance = c_record;
                    m_instancePool.insert (std::make_pair (instanceId, c_instance));

                    return c_record;
                }
                else
                    throw std::runtime_error ("Record instance id already exists");
            }

            void closeRecord (uint32_t instanceId) {
                if (m_instancePool.find (instanceId) != m_instancePool.end()) { 
                    Record* c_record = static_cast <Record*> (m_instancePool[instanceId]);

                    /* Flush buffered sink
                    */
                    if (c_record->getSink() & TO_FILE_BUFFER_CIRCULAR)
                        c_record->flushBufferToFile();

                    delete c_record;
                    /* Remove from map, so you are able to reuse the instance id
                    */
                    m_instancePool.erase (instanceId);   
                    /* Close log buffer
                    */
                    BUFFER_CLOSE (RESERVED_ID_LOG_SINK + instanceId);
                }
                /* Closing a record instance that doesn't exist, do nothing
                */
                else
                    ;
            }

            void closeAllRecords (void) {
                for (auto const& [key, val]: m_instancePool) {
                    Record* c_record = static_cast <Record*> (val);
                    /* Flush buffered sink
                    */
                    if (c_record->getSink() & TO_FILE_BUFFER_CIRCULAR)
                        c_record->flushBufferToFile();

                    delete c_record;
                    BUFFER_CLOSE (RESERVED_ID_LOG_SINK + key);
                }
                m_instancePool.clear();
            }

            /* Clear all configs can be used to disable logging at a global level. Note that, the log objects still exist
             * in the pool since they are not freed yet, meaning you could re add a config and start logging again
            */
            void clearAllConfigs (void) {
                for (auto const& [key, val]: m_instancePool) {
                    Record* c_record = static_cast <Record*> (val);                        
                    c_record->clearConfig();
                }
            }
    };
    RecordMgr recordMgr;
}   // namespace Log
}   // namespace Collections
#endif  // RECORD_MGR_H