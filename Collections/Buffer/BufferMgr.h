#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "BufferImpl.h"

namespace Collections {
namespace Buffer {
    class BufferMgr: public Admin::InstanceMgr {
        public:
            template <typename T>
            BufferImpl <T>* createBuffer (uint32_t instanceId, e_type type, size_t capacity) {
                /* Create and add buffer object to pool
                */
                if (m_instancePool.find (instanceId) == m_instancePool.end()) {
                    BufferImpl <T>* c_buffer = new BufferImpl <T> (instanceId, type, capacity);
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
    BufferMgr g_bufferMgr;
}   // namespace Buffer
}   // namespace Collections
#endif  // BUFFER_MGR_H