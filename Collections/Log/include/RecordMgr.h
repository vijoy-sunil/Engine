#ifndef RECORD_MGR_H
#define RECORD_MGR_H

#include "Record.h"

namespace Collections {
namespace Log {
    class RecordMgr: public Admin::InstanceMgr {
        public:
            Record* initRecord (size_t instanceId, 
                                e_level level, 
                                e_sink sink,
                                std::string saveDir = "",
                                size_t bufferCapacity = 0,
                                std::string format = ".txt") {
            
                // add record object to pool
                if (m_instancePool.find (instanceId) == m_instancePool.end()) {
                    Record* c_record = new Record (instanceId, 
                                                   level, 
                                                   sink, 
                                                   saveDir,
                                                   bufferCapacity,
                                                   format);

                    Admin::NonTemplateBase* c_instance = c_record;
                    m_instancePool.insert (std::make_pair (instanceId, c_instance));

                    return c_record;
                }

                // instance id already exists
                else
                    throw std::runtime_error ("Record instance id already exists");
            }

            void closeRecord (size_t instanceId) {
                if (m_instancePool.find (instanceId) != m_instancePool.end()) { 
                    Record* c_record = static_cast <Record*> (m_instancePool[instanceId]);

                    // flush buffered sink
                    if (c_record-> getSink() & TO_FILE_BUFFER_CIRCULAR)
                        c_record-> flushBufferToFile();

                    delete c_record;
                    // remove from map, so you are able to reuse the instance id
                    m_instancePool.erase (instanceId);   

                    // close log buffer    
                    BUFFER_CLOSE (RESERVED_0 + instanceId);
                }
                // closing a record instance that doesn't exist, do nothing
                else
                    ;
            }

            void closeAllRecords (void) {
                for (auto const& [key, val] : m_instancePool) {
                    Record* c_record = static_cast <Record*> (val);
                    // flush buffered sink
                    if (c_record-> getSink() & TO_FILE_BUFFER_CIRCULAR)
                        c_record-> flushBufferToFile();

                    delete m_instancePool[key];
                    BUFFER_CLOSE (RESERVED_0 + key);
                }
                m_instancePool.clear();
            }
    };
    // single instance
    RecordMgr recordMgr;
}   // namespace Log
}   // namespace Collections
#endif  // RECORD_MGR_H