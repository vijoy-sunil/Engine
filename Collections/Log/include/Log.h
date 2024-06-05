#ifndef LOG_H
#define LOG_H

#include "RecordMgr.h"

// macro overloading
#define GET_MACRO(_1, _2, _3, _4, _5, NAME, ...)            NAME
#define LOG_INIT(...)                                       GET_MACRO(__VA_ARGS__, LOG_INIT_A, LOG_INIT_B)(__VA_ARGS__)
// mgr methods
#define LOG_INIT_A(id, level, sink, saveDir, bufferCap)      Log::recordMgr.initRecord (id,                             \
                                                                                        LOG_GET_FILE,                   \
                                                                                        level,                          \
                                                                                        sink,                           \
                                                                                        saveDir,                        \
                                                                                        bufferCap);
#define LOG_INIT_B(id, level, sink, saveDir)                 Log::recordMgr.initRecord (id,                             \
                                                                                        LOG_GET_FILE,                   \
                                                                                        level,                          \
                                                                                        sink,                           \
                                                                                        saveDir);

#define GET_LOG(id)                             static_cast <Log::Record*>                                              \
                                                (Log::recordMgr.getInstance (id))
// override close methods from instance mgr
#define LOG_CLOSE(id)                           Log::recordMgr.closeRecord (id)
#define LOG_CLOSE_ALL                           Log::recordMgr.closeAllRecords()
/* Normally, the dump method in the manager contains only the instance ids that are currently active, but we can use a
 * lambda function to provide more information about each instance. For example, here we can dump the level and the sink
 * type of each instance in the format: instanceId-LX-SX (-L is the level, -S is the sink)
*/
#define LOG_MGR_DUMP                            Log::recordMgr.dump (std::cout,                                         \
                                                [](Admin::NonTemplateBase* val, std::ostream& ost) {                    \
                                                Log::Record* c_record =                                                 \
                                                static_cast <Log::Record*> (val);                                       \
                                                ost << TAB_L4   << "level : "   << c_record-> getLevel()    << "\n";    \
                                                ost << TAB_L4   << "sink : "    << c_record-> getSink()     << "\n";    \
                                                }                                                                       \
                                                )

// logging methods
#define LOG_INFO(c_record)                      LOG (c_record, Log::INFO)
#define LOG_WARNING(c_record)                   LOG (c_record, Log::WARNING)
#define LOG_ERROR(c_record)                     LOG (c_record, Log::ERROR)

// under the hood   
#define LOG_GET_FILE                            __FILE__
#define LOG_GET_FUNCTION                        __FUNCTION__
#define LOG_GET_LINE                            __LINE__
#define LOG(c_record, level)                    if (! (c_record-> filterLevel (level))) { ; }                           \
                                                else                                                                    \
                                                    c_record-> getReference() <<                                        \
                                                    c_record-> getHeader (level,                                        \
                                                                          LOG_GET_FILE,                                 \
                                                                          LOG_GET_FUNCTION,                             \
                                                                          LOG_GET_LINE) 
#endif  // LOG_H