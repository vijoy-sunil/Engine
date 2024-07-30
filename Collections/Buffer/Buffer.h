#ifndef BUFFER_H
#define BUFFER_H

#include "BufferMgr.h"

#define BUFFER_INIT(id,                                                                                         \
                    type,                                                                                       \
                    dataType,                                                                                   \
                    capacity)                   Buffer::g_bufferMgr.createBuffer <dataType> (id, type, capacity)
#define GET_BUFFER(id, dataType)                dynamic_cast <Buffer::BufferImpl <dataType> *>                  \
                                                (Buffer::g_bufferMgr.getInstance (id)) 
#define BUFFER_CLOSE(id)                        Buffer::g_bufferMgr.closeInstance (id)
#define BUFFER_CLOSE_ALL                        Buffer::g_bufferMgr.closeAllInstances()
#define BUFFER_MGR_DUMP                         Buffer::g_bufferMgr.dump (std::cout)  

#define BUFFER_PUSH(data)                       push (data)
#define BUFFER_POP_FIRST                        popFirst()
#define BUFFER_POP_LAST                         popLast()
#define BUFFER_FLUSH(stream)                    flush (stream)
#define BUFFER_PEEK_FIRST                       getFirst()
#define BUFFER_PEEK_LAST                        getLast()
#define BUFFER_AVAILABILITY                     getAvailability()
#define BUFFER_RESET                            reset()
/* Default sink for buffer dump is set to cout
*/
#define BUFFER_DUMP                             dump (std::cout)
/* Use this to dump buffer containing custom data types by passing in a lambda function specifying how to unravel the
 * custom data type
*/
#define BUFFER_DUMP_CUSTOM(lambda)              dump (std::cout, lambda)
/* When a buffer sink is used for the logger, this id is added to the log instance id for the buffer sink
*/
#define RESERVED_ID_LOG_SINK                    99
#endif  // BUFFER_H