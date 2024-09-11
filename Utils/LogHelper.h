#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <vector>
#include "../Core/VKEnum.h"

namespace Utils {
    /* Split strings into smaller strings using delimiter. For example, supported flags of a queue family represented by
     * "VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT" will be split into individual flags for better
     * readability in log files
    */
    std::vector <std::string> getSplitString (std::string inputString, const std::string& delimiter) {
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

    const char* getBoolString (bool val) {
        return val == true ? "TRUE": "FALSE";
    }

    const char* getImageTypeString (Core::e_imageType type) {
        switch (type)
        {
            case Core::VOID_IMAGE:          return "VOID_IMAGE";
            case Core::SWAPCHAIN_IMAGE:     return "SWAPCHAIN_IMAGE";
            case Core::TEXTURE_IMAGE:       return "TEXTURE_IMAGE";
            case Core::DEPTH_IMAGE:         return "DEPTH_IMAGE";
            case Core::MULTISAMPLE_IMAGE:   return "MULTISAMPLE_IMAGE";
            default:                        return "Unhandled e_imageType";
        }
    }

    const char* getBufferTypeString (Core::e_bufferType type) {
        switch (type)
        {
            case Core::VOID_BUFFER:         return "VOID_BUFFER";
            case Core::STAGING_BUFFER:      return "STAGING_BUFFER";
            case Core::STAGING_BUFFER_TEX:  return "STAGING_BUFFER_TEX";
            case Core::VERTEX_BUFFER:       return "VERTEX_BUFFER";
            case Core::INDEX_BUFFER:        return "INDEX_BUFFER";
            case Core::UNIFORM_BUFFER:      return "UNIFORM_BUFFER";
            case Core::STORAGE_BUFFER:      return "STORAGE_BUFFER";
            default:                        return "Unhandled e_bufferType";
        }
    }

    const char* getSyncTypeString (Core::e_syncType type) {
        switch (type)
        {
            case Core::FEN_TRANSFER_DONE:   return "FEN_TRANSFER_DONE";
            case Core::FEN_BLIT_DONE:       return "FEN_BLIT_DONE";
            case Core::FEN_IN_FLIGHT:       return "FEN_IN_FLIGHT";
            case Core::SEM_IMAGE_AVAILABLE: return "SEM_IMAGE_AVAILABLE";
            case Core::SEM_RENDER_DONE:     return "SEM_RENDER_DONE";
            default:                        return "Unhandled e_syncType";
        }
    }
}   // namespace Utils
#endif  // LOG_HELPER_H