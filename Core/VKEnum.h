#ifndef VK_ENUM_H
#define VK_ENUM_H

namespace Renderer {
    typedef enum {
        VOID_IMAGE          = 0,
        SWAPCHAIN_IMAGE     = 1,
        TEXTURE_IMAGE       = 2,
        DEPTH_IMAGE         = 3,
        MULTISAMPLE_IMAGE   = 4
    } e_imageType;

    typedef enum {
        VOID_BUFFER         = 0,
        STAGING_BUFFER      = 1,
        STAGING_BUFFER_TEX  = 2,
        VERTEX_BUFFER       = 3,
        INDEX_BUFFER        = 4,
        UNIFORM_BUFFER      = 5
    } e_bufferType;

    typedef enum {
        FEN_TRANSFER_DONE   = 0,
        FEN_BLIT_DONE       = 1,
        FEN_IN_FLIGHT       = 2,
        SEM_IMAGE_AVAILABLE = 3,
        SEM_RENDER_DONE     = 4
    } e_syncType;
}   // namespace Renderer
#endif  // VK_ENUM_H