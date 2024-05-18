#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "BufferImpl.h"

namespace Collections {
namespace Buffer {
    class BufferMgr: public Admin::InstanceMgr {
        public:
            template <typename T>
            Buffer <T>* initBuffer (size_t instanceId, 
                                    e_type type, 
                                    size_t capacity) {

                // create and add buffer object to pool
                if (m_instancePool.find (instanceId) == m_instancePool.end()) {
                    Buffer <T>* c_buffer = new Buffer <T> (instanceId, type, capacity);

                    // upcasting
                    Admin::NonTemplateBase* c_instance = c_buffer;
                    m_instancePool.insert (std::make_pair (instanceId, c_instance));

                    return c_buffer;
                }
                // instance id already exists
                else
                    assert (false);
            }
    };
    BufferMgr bufferMgr;
}   // namespace Buffer
}   // namespace Collections
#endif  // BUFFER_MGR_H