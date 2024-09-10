#ifndef BUFFER_IMPL_H
#define BUFFER_IMPL_H

#include "../InstanceMgr.h"

namespace Collections {
namespace Buffer {
    typedef enum {
        WITH_OVERFLOW    = 0,
        WITHOUT_OVERFLOW = 1
    } e_bufferType;

    template <typename T>
    class BufferImpl: public Admin::NonTemplateBase {
        private:
            uint32_t m_instanceId;
            e_bufferType m_type;
            size_t m_capacity;
            size_t m_numItems;

            T* m_buffer;
            T* m_head;
            T* m_tail;
            T* m_end;

            inline bool isEmpty (void) {
                return m_numItems == 0;
            } 

            inline bool isFull (void) {
                return m_numItems == m_capacity;
            }

        public:
            BufferImpl (uint32_t instanceId, e_bufferType type, size_t capacity) {
                m_instanceId = instanceId;
                m_type       = type;
                m_capacity   = capacity;
                m_numItems   = 0;
                m_buffer     = new T[capacity];
                m_head       = m_buffer;
                m_tail       = m_buffer;
                m_end        = m_buffer + m_capacity - 1;
            }

            ~BufferImpl (void) {
                delete[] m_buffer;
            }
            
            void push (const T& data) {
                /* Always push when in overflow enabled mode
                */
                if (!isFull() || m_type == WITH_OVERFLOW) {
                    *m_head = data;
                    m_numItems++;
                    /* When head pointer is at the end of the buffer
                    */
                    m_head = m_head == m_end ? m_buffer: m_head + 1;
                    /* If num items is greater than capacity, that means we have overflowed over the oldest element, so
                     * we need to update the tail pointer (pointing to the oldest element) and correct num items
                    */
                    if (m_type == WITH_OVERFLOW && m_numItems > m_capacity) {
                        m_tail = m_tail == m_end ? m_buffer: m_tail + 1;
                        /* Correct num items
                        */
                        m_numItems--;
                    }
                }
                /* If push fails due to maximum capacity, do nothing
                */
                else
                    ;
            }

            T* popFirst (void) {
                T* data = nullptr;

                if (!isEmpty()) {
                    data = m_tail;
                    m_numItems--;
                    /* When tail pointer is at the end of the buffer
                    */
                    m_tail = m_tail == m_end ? m_buffer: m_tail + 1;
                }   
                return data; 
            }

            T* popLast (void) {
                T* data = nullptr;

                if (!isEmpty()) {
                    /* When head pointer is at the head of the buffer
                    */
                    m_head = m_head == m_buffer ? m_end: m_head - 1;

                    data = m_head;
                    m_numItems--;
                }
                return data;
            }

            void flush (std::ostream& ost) {
                while (!isEmpty())
                    ost << *popFirst() << "\n";
                
                ost.flush();
            }
            
            inline T* getFirst (void) {
                return isEmpty() ? nullptr: m_tail;
            }

            inline T* getLast (void) {
                return isEmpty() ? nullptr: 
                /* Head pointer will be at the start either when the buffer is empty, or when an item has been inserted at 
                 * the end and wrap around is complete
                */
                m_head == m_buffer ? m_end: (m_head - 1);
            }

            inline size_t getAvailability (void) {
                return m_capacity - m_numItems;
            }

            void reset (void) {
                m_numItems = 0;
                m_head     = m_buffer;
                m_tail     = m_buffer;
            }

            /* Buffer is displayed in the following format
             * Buffer: 
             *          {                               <L1>
             *              Id: ?                       <L2>
             *              Availability: ?
             *              First: ?
             *              Last: ?
             *              Data: 
             *                      {                   <L3>
             *                          ?               <L4>
             *                          ?
             *                          ...
             *                      }                   <L3>
             *          }                               <L1>
            */
            void dump (std::ostream& ost, 
                       void (*lambda) (T*, std::ostream&) = [](T* readPtr, std::ostream& ost) { 
                                                                ost << *readPtr; 
                                                            }) {
                T* readPtr = m_tail;
                size_t numItems = m_numItems;

                ost << "Buffer: " << "\n";
                ost << OPEN_L1;

                ost << TAB_L2 << "Id: "             << m_instanceId         << "\n";
                ost << TAB_L2 << "Availability: "   << getAvailability()    << "\n";

                ost << TAB_L2 << "First: ";        
                if (m_numItems != 0)            lambda (getFirst(), ost); 
                else                            ost << "NULL";                       
                ost << "\n"; 

                ost << TAB_L2 << "Last: ";        
                if (m_numItems != 0)            lambda (getLast(), ost);
                else                            ost << "NULL";                       
                ost << "\n"; 

                ost << TAB_L2 << "Data: "           << "\n";
                ost << OPEN_L3;
                while (numItems != 0) {
                ost << TAB_L4;                  lambda (readPtr, ost);  ost << "\n";
                numItems--;
                readPtr = readPtr == m_end ? m_buffer: readPtr + 1;
                }
                ost << CLOSE_L3;
                
                ost << CLOSE_L1;
            }
    };
}   // namespace Buffer
}   // namespace Collections
#endif  // BUFFER_IMPL_H