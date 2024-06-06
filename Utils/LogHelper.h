#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <vector>

namespace Renderer {
namespace Utils {
    /* Split strings into smaller strings using delimiter. For example, supported flags of a queue family represented by
     * "VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT" will be split into individual flags for better
     * readability in log files
    */
    std::vector <std::string> splitString (std::string inputString, const std::string& delimiter) {
        std::vector <std::string> outputStrings;
        size_t pos = 0;
        std::string token;

        while ((pos = inputString.find (delimiter)) != std::string::npos) {
            token = inputString.substr (0, pos);
            outputStrings.push_back (token);
            inputString.erase (0, pos + delimiter.length());
        }
        outputStrings.push_back (inputString);
        return outputStrings;
    }
}   // namespace Utils
}   // namespace Renderer
#endif  // LOG_HELPER_H