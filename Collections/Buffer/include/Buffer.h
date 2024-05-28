#ifndef BUFFER_H
#define BUFFER_H

#include "BufferMgr.h"

// buffer manager methods
#define BUFFER_INIT(id,                                                                                         \
                    type,                                                                                       \
                    dataType,                                                                                   \
                    capacity)                   Buffer::bufferMgr.initBuffer <dataType> (id, type, capacity)
// get buffer instance (pointer to buffer object) from mgr
#define GET_BUFFER(id, dataType)                dynamic_cast <Buffer::Buffer <dataType> *>                      \
                                                (Buffer::bufferMgr.getInstance (id)) 
#define BUFFER_CLOSE(id)                        Buffer::bufferMgr.closeInstance (id)
#define BUFFER_CLOSE_ALL                        Buffer::bufferMgr.closeAllInstances()
#define BUFFER_MGR_DUMP                         Buffer::bufferMgr.dump (std::cout)  

#define BUFFER_PUSH(data)                       push (data)
#define BUFFER_POP_FIRST                        popFirst()
#define BUFFER_POP_LAST                         popLast()
#define BUFFER_FLUSH(stream)                    flush (stream)
#define BUFFER_PEEK_FIRST                       peekFirst()
#define BUFFER_PEEK_LAST                        peekLast()
#define BUFFER_AVAILABILITY                     availability()
#define BUFFER_RESET                            reset()
// default sink for buffer dump is set to cout
#define BUFFER_DUMP                             dump (std::cout)
/* use this to dump buffer containing custom data types by passing in a lambda function specifying how to unravel the
 * custom data type
*/
#define BUFFER_DUMP_CUSTOM(lambda)              dump (std::cout, lambda)
#endif  // BUFFER_H