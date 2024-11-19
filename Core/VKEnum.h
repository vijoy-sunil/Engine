#ifndef VK_ENUM_H
#define VK_ENUM_H

namespace Core {
    typedef enum {
        VOID_IMAGE          = 0,
        SWAP_CHAIN_IMAGE    = 1,
        TEXTURE_IMAGE       = 2,
        DEPTH_IMAGE         = 3,
        MULTI_SAMPLE_IMAGE  = 4
    } e_imageType;

    typedef enum {
        VOID_BUFFER         = 0,
        STAGING_BUFFER      = 1,
        VERTEX_BUFFER       = 2,
        INDEX_BUFFER        = 3,
        UNIFORM_BUFFER      = 4,
        STORAGE_BUFFER      = 5
    } e_bufferType;

    typedef enum {
        FEN_ONE_TIME_OPS    = 0,
        FEN_IN_FLIGHT       = 1,
        SEM_IMAGE_AVAILABLE = 2,
        SEM_RENDER_DONE     = 3
    } e_syncType;

    typedef enum {
        PER_FRAME_SET       = 0,
        COMMON_SET          = 1
    } e_descriptorSetType;
}   // namespace Core
#endif  // VK_ENUM_H