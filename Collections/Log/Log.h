#ifndef LOG_H
#define LOG_H

#include "RecordMgr.h"

/* Macro overloading
*/
#define GET_MACRO(_1, _2, _3, NAME, ...)        NAME
#define LOG_INIT(...)                           GET_MACRO(__VA_ARGS__, LOG_INIT_A, LOG_INIT_B)(__VA_ARGS__)
#define LOG_INIT_A(id, saveDir, bufferCap)      Log::recordMgr.initRecord (id,                                          \
                                                                           LOG_GET_FILE,                                \
                                                                           saveDir,                                     \
                                                                           bufferCap);
#define LOG_INIT_B(id, saveDir)                 Log::recordMgr.initRecord (id,                                          \
                                                                           LOG_GET_FILE,                                \
                                                                           saveDir);
    
#define LOG_CLOSE(id)                           Log::recordMgr.closeRecord (id)
#define LOG_CLOSE_ALL                           Log::recordMgr.closeAllRecords()
#define LOG_ADD_CONFIG(id, level, sink)         GET_LOG (id)->addConfig (level, sink);
#define LOG_CLEAR_CONFIG(id)                    GET_LOG (id)->clearConfig();
#define LOG_CLEAR_ALL_CONFIGS                   Log::recordMgr.clearAllConfigs();

/* Normally, the dump method in the manager contains only the instance ids that are currently active, but we can use a
 * lambda function to provide more information about each instance. For example, here we can dump the level and the sink
 * type of each instance
*/
#define LOG_MGR_DUMP                            Log::recordMgr.dump (std::cout,                                         \
                                                [](Admin::NonTemplateBase* val, std::ostream& ost) {                    \
                                                Log::Record* c_record =                                                 \
                                                static_cast <Log::Record*> (val);                                       \
                                                ost << TAB_L4   << "level : "   << c_record->getLevel()    << "\n";     \
                                                ost << TAB_L4   << "sink : "    << c_record->getSink()     << "\n";     \
                                                })

/* Logging methods
*/
#define LOG_INFO(c_record)                      LOG(c_record, Log::INFO, true)
#define LOG_WARNING(c_record)                   LOG(c_record, Log::WARNING, true)
#define LOG_ERROR(c_record)                     LOG(c_record, Log::ERROR, true)
/* Lightweight logging method with header disabled
*/
#define LOG_LITE(c_record)                      LOG(c_record, Log::INFO, false)      
/* Under the hood
*/   
#define GET_LOG(id)                             static_cast <Log::Record*>                                              \
                                                (Log::recordMgr.getInstance (id))
#define LOG_GET_FILE                            __FILE__
#define LOG_GET_FUNCTION                        __FUNCTION__
#define LOG_GET_LINE                            __LINE__
#define LOG(c_record, level, header)            if (! (c_record-> filterLevel (level))) { ; }                           \
                                                else                                                                    \
                                                    c_record-> getReference() <<                                        \
                                                    c_record-> getHeader (level,                                        \
                                                                          LOG_GET_FUNCTION,                             \
                                                                          LOG_GET_LINE,                                 \
                                                                          header)
#endif  // LOG_H