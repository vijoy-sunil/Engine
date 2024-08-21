#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <vector>
#include "../Core/VKEnum.h"

using namespace Core;

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

    const char* getImageTypeString (e_imageType type) {
        switch (type)
        {
            case VOID_IMAGE:
                return "VOID_IMAGE";
            case SWAPCHAIN_IMAGE:
                return "SWAPCHAIN_IMAGE";
            case TEXTURE_IMAGE:
                return "TEXTURE_IMAGE";
            case DEPTH_IMAGE:
                return "DEPTH_IMAGE";
            case MULTISAMPLE_IMAGE:
                return "MULTISAMPLE_IMAGE";
            default:
                return "Unhandled e_imageType";
        }
    }

    const char* getBufferTypeString (e_bufferType type) {
        switch (type)
        {
            case VOID_BUFFER:
                return "VOID_BUFFER";
            case STAGING_BUFFER:
                return "STAGING_BUFFER";
            case STAGING_BUFFER_TEX:
                return "STAGING_BUFFER_TEX";
            case VERTEX_BUFFER:
                return "VERTEX_BUFFER";
            case INDEX_BUFFER:
                return "INDEX_BUFFER";
            case UNIFORM_BUFFER:
                return "UNIFORM_BUFFER";
            case STORAGE_BUFFER:
                return "STORAGE_BUFFER";
            default:
                return "Unhandled e_bufferType";
        }
    }

    const char* getSyncTypeString (e_syncType type) {
        switch (type)
        {
            case FEN_TRANSFER_DONE:
                return "FEN_TRANSFER_DONE";
            case FEN_BLIT_DONE:
                return "FEN_BLIT_DONE";
            case FEN_IN_FLIGHT:
                return "FEN_IN_FLIGHT";
            case SEM_IMAGE_AVAILABLE:
                return "SEM_IMAGE_AVAILABLE";
            case SEM_RENDER_DONE:
                return "SEM_RENDER_DONE";
            default:
                return "Unhandled e_syncType";
        }
    }
}   // namespace Utils
#endif  // LOG_HELPER_H