#ifndef VK_UTILS_H
#define VK_UTILS_H

#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKUtils {
        private:
            /* Handle to the log object
            */
            static Log::Record* m_VKUtilsLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 10;
                 
        public:
            VKUtils (void) {
                m_VKUtilsLog = LOG_INIT (m_instanceId, 
                                         Log::VERBOSE, 
                                         Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                         "./Build/Log/");
                LOG_INFO (m_VKUtilsLog) << "Constructor called" << std::endl; 
            }

            ~VKUtils (void) {
                LOG_INFO (m_VKUtilsLog) << "Destructor called" << std::endl; 
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* Read all of the bytes from the specified file and return them in a byte array managed by std::vector. This 
             * function is used to read shader binary files
            */
            std::vector <char> readFile (const std::string& filename) {
                /* ate: Start reading at the end of the file
                 * binary: Read the file as binary file (avoid text transformations)
                 *
                 * The advantage of starting to read at the end of the file is that we can use the read position to 
                 * determine the size of the file and allocate a buffer
                */
                std::ifstream file (filename, std::ios::ate | std::ios::binary);
                if (!file.is_open()) {
                    LOG_WARNING (m_VKUtilsLog) << "Failed to open file" << " " << filename << std::endl;
                    return {};
                }

                size_t fileSize = (size_t) file.tellg();
                std::vector <char> buffer(fileSize);
                /* seek back to the beginning of the file and read all of the bytes at once
                */
                file.seekg(0);
                file.read (buffer.data(), fileSize);

                file.close();
                return buffer;
            }
    };

    Log::Record* VKUtils::m_VKUtilsLog;
}   // namesapce Renderer
#endif  // VK_UTILS_H