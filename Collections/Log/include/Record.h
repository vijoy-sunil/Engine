#ifndef RECORD_H
#define RECORD_H

#include "../../InstanceMgr.h"
// import buffer from collections to use buffered sink
#include "../../Buffer/include/Buffer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace Collections {
namespace Log {
    typedef enum {
        NONE = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 4,
        VERBOSE = 7
    }e_level;

    typedef enum {
        TO_FILE_IMMEDIATE = 1,
        TO_CONSOLE = 2,
        TO_FILE_BUFFER_CIRCULAR = 4
    }e_sink;

    // define bit operators for enum
    inline e_level operator | (e_level a, e_level b) {
        return static_cast <e_level> (static_cast <int> (a) | static_cast <int> (b));
    }

    inline e_sink operator | (e_sink a, e_sink b) {
        return static_cast <e_sink> (static_cast <int> (a) | static_cast <int> (b));
    }

    // record objects (identified by instance ids) are maintained by the mgr
    class Record: public Admin::NonTemplateBase {
        private:
            size_t m_instanceId;
            e_level m_level;
            e_sink m_sink;
            std::ofstream m_saveFile_immediate;
            std::ofstream m_saveFile_buffered; 

            // std::endl is a template function, and this is the signature of that function
            using endl_type = std::ostream& (std::ostream&); 

            // variable to hold entry in buffered sink
            std::string m_bufferedSinkHolder;

            // file names
            std::string m_saveFileName_immediate;
            std::string m_saveFileName_buffered;

            std::string levelToString (e_level level) {
                std::string result;
                switch (level) {
                    case INFO:
                        result = "INFO";
                        break;

                    case WARNING:
                        result = "WARN";
                        break;

                    case ERROR:
                        result = "ERRO";
                        break;

                    default:
                        result = "UNDF";
                        break;
                }
                return result;
            }

            std::string getLocalTimestamp (void) {
                /* the string stream associates a string object with a string. Using this we can read from string as if it were a 
                * stream like cin
                */
                std::stringstream stream;
                
                std::chrono::time_point <std::chrono::system_clock> now = std::chrono::system_clock::now();
                std::time_t t_c = std::chrono::system_clock::to_time_t (now);
                // https://en.cppreference.com/w/cpp/io/manip/put_time
                stream << std::put_time (std::localtime (&t_c), "%F %T");
                return stream.str();
            }

            // these 2 methods helps us to convert everything to string type
            std::string to_string (const std::string& r) const { 
                return r; 
            }

            template <typename T>
            typename std::enable_if <!std::is_convertible <T, std::string>::value, 
                                      std::string>::type to_string (T r) const { 
                return std::to_string (r); 
            }

        public:
            Record (size_t instanceId, 
                    e_level level, 
                    e_sink sink, 
                    std::string saveDir,
                    size_t bufferCapacity,
                    std::string format) {

                // save instance id, this is used to differentiate between logs
                m_instanceId = instanceId;
                m_level = level;
                m_sink = sink;

                // create dir
                if ((m_sink & TO_FILE_IMMEDIATE) || (m_sink & TO_FILE_BUFFER_CIRCULAR)) {
                    std::string dirHierarchy = saveDir;
                    std::filesystem::create_directories (dirHierarchy);

                    m_saveFileName_immediate = dirHierarchy + "immediate_log_" + 
                                               std::to_string (m_instanceId) + format;

                    m_saveFileName_buffered  = dirHierarchy + "buffered_log_" + 
                                               std::to_string (m_instanceId) + format;
                }

                // open file, note that for this sink we are in append mode
                if (m_sink & TO_FILE_IMMEDIATE) { 
                    m_saveFile_immediate.open (m_saveFileName_immediate, 
                                               std::ios_base::app | std::ios_base::out);

                    if (!m_saveFile_immediate.is_open())
                        throw std::runtime_error ("Unable to open file for TO_FILE_IMMEDIATE sink");
                }

                if (m_sink & TO_FILE_BUFFER_CIRCULAR) {
                    // if capacity is invalid
                    if (bufferCapacity == 0)
                        throw std::runtime_error ("Buffer capacity invalid for TO_FILE_BUFFER_CIRCULAR sink");    

                    // buffer instance id will be offset from log instance id
                    BUFFER_INIT (RESERVED_0 + m_instanceId, 
                                 Buffer::WITH_OVERFLOW, 
                                 std::string, 
                                 bufferCapacity);   
                    
                    // open file
                    m_saveFile_buffered.open (m_saveFileName_buffered, 
                                              std::ios_base::out);
                    if (!m_saveFile_buffered.is_open())
                        throw std::runtime_error ("Unable to open file for TO_FILE_BUFFER_CIRCULAR sink");
                }
            }

            ~Record (void) { 
                // close all files
                if (m_sink & TO_FILE_IMMEDIATE)
                    m_saveFile_immediate.close();

                if (m_sink & TO_FILE_BUFFER_CIRCULAR)
                    m_saveFile_buffered.close();
            }

            inline Record& getReference (void) {
                return *this;
            }

            inline e_sink getSink (void) {
                return m_sink;
            }

            inline e_level getLevel (void) {
                return m_level;
            }

            std::string getHeader (e_level level,
                                   const char* file, 
                                   const char* function, 
                                   const size_t line) {
                std::string instanceId = std::to_string (m_instanceId);
                /* Pad m_instanceId string for single digit ids
                */
                if (m_instanceId < 10)
                    instanceId.insert (0, 1, '0');

                std::string header = "[ " + instanceId + " ]" + " " +
                                     getLocalTimestamp() + " " +
                                     "[ " + levelToString (level) + " ]" + " " +
                                     file + " " +
                                     function + " " +
                                     std::to_string (line) +  " ";

                return header;
            }

            inline bool filterLevel (e_level level) {
                return m_level & level;
            }

            // write buffered data to file, only used when sink is a buffered sink
            void flushBufferToFile (void) {
                if (m_saveFile_buffered.is_open()) {
                    auto logBuffer = GET_BUFFER (RESERVED_0 + m_instanceId, std::string);
                    logBuffer-> BUFFER_FLUSH (m_saveFile_buffered);
                }
            }

            // overload for std::endl
            Record& operator << (endl_type endl){
                if (m_sink & TO_FILE_IMMEDIATE && m_saveFile_immediate.is_open())
                    m_saveFile_immediate << endl;

                if (m_sink & TO_CONSOLE)
                    std::cout << endl;
                
                // for buffered sink, instead of inserting a new line we push the log entry into the buffer
                if (m_sink & TO_FILE_BUFFER_CIRCULAR) {
                    auto logBuffer = GET_BUFFER (RESERVED_0 + m_instanceId, std::string);
                    logBuffer-> BUFFER_PUSH (m_bufferedSinkHolder);
                    // clear after flush
                    m_bufferedSinkHolder = "";
                }

                return *this;
            }

            /* templated overload operator, 
             * reference: https://stackoverflow.com/questions/17595957/operator-overloading-in-c-for-logging-purposes
            */
            template<typename T>
            Record& operator << (const T& data) {
                // multiple sink support
                if (m_sink & TO_FILE_IMMEDIATE && m_saveFile_immediate.is_open())
                    m_saveFile_immediate << data;

                if (m_sink & TO_CONSOLE)
                    std::cout << data;

                // hold till we flush it upon std::endl
                if (m_sink & TO_FILE_BUFFER_CIRCULAR)
                    m_bufferedSinkHolder += to_string (data);
            
                return *this;
            }
    };
}   // namespace Log
}   // namespace Collections
#endif  // RECORD_H