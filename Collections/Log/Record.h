#ifndef RECORD_H
#define RECORD_H

#include <fstream>
#include <sstream>
#include "../Buffer/Buffer.h"

namespace Collections {
namespace Log {
    typedef enum {
        NONE    = 0,
        INFO    = 1,
        WARNING = 2,
        ERROR   = 4,
        VERBOSE = 7
    } e_level;

    typedef enum {
        TO_NONE                 = 0,
        TO_FILE_IMMEDIATE       = 1,
        TO_CONSOLE              = 2,
        TO_FILE_BUFFER_CIRCULAR = 4
    } e_sink;

    inline e_level operator | (e_level a, e_level b) {
        return static_cast <e_level> (static_cast <int> (a) | static_cast <int> (b));
    }

    inline e_sink operator | (e_sink a, e_sink b) {
        return static_cast <e_sink> (static_cast <int> (a) | static_cast <int> (b));
    }

    class Record: public Admin::NonTemplateBase {
        private:
            size_t m_instanceId;
            std::string m_callingFile;
            std::string m_saveDir;
            size_t m_bufferCapacity;
            const char* m_format;
            std::map <e_level, e_sink> m_levelConfig;
            std::fstream m_saveFileImmediate;
            std::fstream m_saveFileBuffered; 
            /* std::endl is a template function, and this is the signature of that function
            */
            using endl_type = std::ostream& (std::ostream&); 
            /* Variable to hold entry in buffered sink
            */
            std::string m_bufferedSinkHolder;
            /* File paths
            */
            std::string m_saveFilePathImmediate;
            std::string m_saveFilePathBuffered;
            /* Current sink for the filtered level
            */
            e_sink m_activeSink;
            bool m_fileImmediateReady;
            bool m_fileBufferedReady;

            const char* levelToString (e_level level) {
                switch (level) {
                    case INFO:
                        return "INFO";
                    case WARNING:
                        return "WARN";
                    case ERROR:
                        return "ERRO";
                    default:
                        return "UNDF";
                }
            }

            std::string getLocalTimestamp (void) {
                /* The string stream associates a string object with a string. Using this we can read from string as if it
                 * were a stream like cin
                */
                std::stringstream stream;
                
                std::chrono::time_point <std::chrono::system_clock> now = std::chrono::system_clock::now();
                std::time_t t_c = std::chrono::system_clock::to_time_t (now);
                /* https://en.cppreference.com/w/cpp/io/manip/put_time
                */
                stream << std::put_time (std::localtime (&t_c), "%F %T");
                return stream.str();
            }

            /* These 2 methods helps us to convert everything to string type
            */
            std::string to_string (const std::string& r) const { 
                return r; 
            }

            template <typename T>
            typename std::enable_if <!std::is_convertible <T, std::string>::value, 
                                      std::string>::type to_string (T r) const { 
                return std::to_string (r); 
            }

            void deleteEmptyFile (std::fstream& file, const char* filePath) {
                /* Check if file is empty
                */
                if (file.peek() == std::ifstream::traits_type::eof()) {
                    if (remove (filePath) != 0)
                        throw std::runtime_error ("Unable to delete file");
                }
            }

        public:
            Record (size_t instanceId, 
                    std::string callingFile,
                    std::string saveDir,
                    size_t bufferCapacity,
                    const char* format) {

                m_instanceId     = instanceId;
                m_callingFile    = callingFile;
                m_saveDir        = saveDir;
                m_bufferCapacity = bufferCapacity;
                m_format         = format;

                m_levelConfig.insert ({INFO,    TO_NONE}); 
                m_levelConfig.insert ({WARNING, TO_NONE}); 
                m_levelConfig.insert ({ERROR,   TO_NONE}); 

                m_activeSink         = TO_NONE;
                m_fileImmediateReady = false;
                m_fileBufferedReady  = false;

                /* Strip file path and file extension to get just its name
                */
                size_t strip_start = m_callingFile.find_last_of ("\\/") + 1;
                size_t strip_end   = m_callingFile.find_last_of ('.');
                m_callingFile      = m_callingFile.substr (strip_start, strip_end - strip_start);
            }

            ~Record (void) { 
                clearConfig();
            }

            void addConfig (e_level level, e_sink sink) {
                m_levelConfig[level] = sink;
                /* Open file, note that for this sink we are in append mode
                */
                if (!m_fileImmediateReady && (sink & TO_FILE_IMMEDIATE)) { 
                    m_saveFilePathImmediate = m_saveDir + "i_" + 
                                              std::to_string (m_instanceId) + "_" +
                                              m_callingFile + 
                                              m_format;

                    m_saveFileImmediate.open (m_saveFilePathImmediate, 
                                              std::ios_base::app | std::ios_base::out);

                    if (!m_saveFileImmediate.is_open())
                        throw std::runtime_error ("Unable to open file for TO_FILE_IMMEDIATE sink");
                    
                    m_fileImmediateReady = true;
                }

                if (!m_fileBufferedReady && (sink & TO_FILE_BUFFER_CIRCULAR)) {
                    if (m_bufferCapacity == 0)
                        throw std::runtime_error ("Buffer capacity invalid for TO_FILE_BUFFER_CIRCULAR sink");    

                    BUFFER_INIT (RESERVED_ID_LOG_SINK + m_instanceId, 
                                 Buffer::WITH_OVERFLOW, 
                                 std::string, 
                                 m_bufferCapacity);   

                    m_saveFilePathBuffered  = m_saveDir + "b_" + 
                                              std::to_string (m_instanceId) + "_" +
                                              m_callingFile +
                                              m_format;

                    m_saveFileBuffered.open (m_saveFilePathBuffered, 
                                              std::ios_base::out);
                                              
                    if (!m_saveFileBuffered.is_open())
                        throw std::runtime_error ("Unable to open file for TO_FILE_BUFFER_CIRCULAR sink");

                    m_fileBufferedReady = true;
                }
            }

            /* Clear config method should be used to overwrite an existing configuration. Since the overwrite may include
             * buffered/immediate sinks, we need to handle that by flushing file contents and closing the file
            */
            void clearConfig (void) {
                e_sink allSinks = getSink();
                if (allSinks & TO_FILE_BUFFER_CIRCULAR) {
                    /* Flush buffered sink before closing
                    */
                    flushBufferToFile();
                    m_saveFileBuffered.close();
                    /* Open the file in read mode and delete if empty
                    */
                    m_saveFileBuffered.open (m_saveFilePathBuffered, std::ios_base::in);
                    deleteEmptyFile (m_saveFileBuffered, m_saveFilePathBuffered.c_str());
                }

                if (allSinks & TO_FILE_IMMEDIATE) {
                    m_saveFileImmediate.close();
                    m_saveFileImmediate.open (m_saveFilePathImmediate, std::ios_base::in);
                    deleteEmptyFile (m_saveFileImmediate, m_saveFilePathImmediate.c_str());
                }

                m_levelConfig[INFO]    = TO_NONE;
                m_levelConfig[WARNING] = TO_NONE;
                m_levelConfig[ERROR]   = TO_NONE;

                m_fileImmediateReady   = false;
                m_fileBufferedReady    = false;
            }

            inline Record& getReference (void) {
                return *this;
            }

            e_sink getSink (void) {
                e_sink allSinks = TO_NONE;
                for (auto const& [level, sink]: m_levelConfig)
                    allSinks = allSinks | sink;

                return allSinks;
            }

            e_level getLevel (void) {
                e_level allLevels = NONE;
                for (auto const& [level, sink]: m_levelConfig) {
                    if (sink != TO_NONE)
                        allLevels = allLevels | level;
                }

                return allLevels;
            }

            std::string getHeader (e_level level,
                                   const char* callingFunction, 
                                   const size_t line,
                                   bool enHeader) {
                /* Skip header for lightweight logging
                */
                if (enHeader == false)
                    return "";

                std::string instanceId = std::to_string (m_instanceId);
                /* Pad m_instanceId string for single digit ids
                */
                if (m_instanceId < 10)
                    instanceId.insert (0, 1, '0');

                std::string header = "[ " + instanceId + " ]" + 
                                     " " +
                                     getLocalTimestamp() + 
                                     " " +
                                     "[ " + levelToString (level) + " ]" 
                                     + " " +
                                     callingFunction + 
                                     " " +
                                     std::to_string (line) +  
                                     " ";

                return header;
            }

            /* Write buffered data to file, only used when sink is a buffered sink
            */
            void flushBufferToFile (void) {
                if (m_saveFileBuffered.is_open()) {
                    auto logBuffer = GET_BUFFER (RESERVED_ID_LOG_SINK + m_instanceId, std::string);
                    logBuffer-> BUFFER_FLUSH (m_saveFileBuffered);
                }
            }

            bool filterLevel (e_level level) {
                /* Set active sink, this will decide where the logging will output to for this level
                */
                m_activeSink = m_levelConfig[level];
                return m_activeSink == TO_NONE ? false: true;
            }

            /* Overload for std::endl
            */
            Record& operator << (endl_type endl){
                if (m_activeSink & TO_FILE_IMMEDIATE && m_saveFileImmediate.is_open())
                    m_saveFileImmediate << endl;

                if (m_activeSink & TO_CONSOLE)
                    std::cout << endl;
                
                /* For buffered sink, instead of inserting a new line we push the log entry into the buffer
                */
                if (m_activeSink & TO_FILE_BUFFER_CIRCULAR) {
                    auto logBuffer = GET_BUFFER (RESERVED_ID_LOG_SINK + m_instanceId, std::string);
                    logBuffer-> BUFFER_PUSH (m_bufferedSinkHolder);
                    /* Clear after flush
                    */
                    m_bufferedSinkHolder = "";
                }

                return *this;
            }

            /* Templated overload operator, 
             * reference: https://stackoverflow.com/questions/17595957/operator-overloading-in-c-for-logging-purposes
            */
            template <typename T>
            Record& operator << (const T& data) {
                if (m_activeSink & TO_FILE_IMMEDIATE && m_saveFileImmediate.is_open())
                    m_saveFileImmediate << data;

                if (m_activeSink & TO_CONSOLE)
                    std::cout << data;

                /* Hold till we flush it upon std::endl
                */
                if (m_activeSink & TO_FILE_BUFFER_CIRCULAR)
                    m_bufferedSinkHolder += to_string (data);
            
                return *this;
            }
    };
}   // namespace Log
}   // namespace Collections
#endif  // RECORD_H