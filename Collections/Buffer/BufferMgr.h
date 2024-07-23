#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "BufferImpl.h"

namespace Collections {
namespace Buffer {
    class BufferMgr: public Admin::InstanceMgr {
        public:
            template <typename T>
            Buffer <T>* initBuffer (uint32_t instanceId, e_type type, size_t capacity) {
                /* Create and add buffer object to pool
                */
                if (m_instancePool.find (instanceId) == m_instancePool.end()) {
                    Buffer <T>* c_buffer = new Buffer <T> (instanceId, type, capacity);
                    /* Upcasting
                    */
                    Admin::NonTemplateBase* c_instance = c_buffer;
                    m_instancePool.insert (std::make_pair (instanceId, c_instance));
                    return c_buffer;
                }
                else
                    throw std::runtime_error ("Buffer instance id already exists");
            }
    };
    BufferMgr bufferMgr;
}   // namespace Buffer
}   // namespace Collections
#endif  // BUFFER_MGR_H