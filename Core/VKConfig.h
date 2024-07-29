#ifndef VK_CONFIG_H
#define VK_CONFIG_H

#include <vector>

namespace Renderer {
    #define ENABLE_LOGGING              (true)
    #define ENABLE_MODEL_IMPORT         (true)
    #define ENABLE_CYCLE_TEXTURES       (true)
    #define ENABLE_IDLE_ROTATION        (true)

    struct windowSettings {
        const int width   = 800;
        const int height  = 600;
        const char* title = "WINDOW_";
    } g_windowSettings;

    struct pathSettings {
        const char* logSaveDir            = "Build/Log/";
        const std::vector <const char*> models = {
            "SandBox/Models/Model_1.obj",
            "SandBox/Models/Model_2.obj",
            "SandBox/Models/Model_3.obj",
            "SandBox/Models/Model_4.obj"
        };
        const char* mtlFileDir            = "SandBox/Models/";
        const char* defaultDiffuseTexture = "SandBox/Textures/tex_16x16_empty.png";
        const std::vector <const char*> cycleTextures = {
            "SandBox/Textures/tex_512x512_n0.png",
            "SandBox/Textures/tex_512x512_n1.png",
            "SandBox/Textures/tex_512x512_n2.png",
            "SandBox/Textures/tex_512x512_n3.png"
        };
        const char* vertexShaderBinary    = "Build/Bin/vert.spv";
        const char* fragmentShaderBinary  = "Build/Bin/frag.spv"; 
    } g_pathSettings;

    /* As of now, we are required to wait on the previous frame to finish before we can start rendering the next which 
     * results in unnecessary idling of the host. The way to fix this is to allow multiple frames to be in-flight at once, 
     * that is to say, allow the rendering of one frame to not interfere with the recording of the next. Any resource that 
     * is accessed and modified during rendering must be duplicated. Thus, we need multiple command buffers, semaphores, 
     * and fences. First, defines how many frames should be processed concurrently
     * 
     * We choose the number 2 because we don't want the CPU to get too far ahead of the GPU. With 2 frames in flight, the 
     * CPU and the GPU can be working on their own tasks at the same time. If the CPU finishes early, it will wait till 
     * the GPU finishes rendering before submitting more work. With 3 or more frames in flight, the CPU could get ahead 
     * of the GPU, adding frames of latency as shown in the scenario below:
     * 
     * What happens if frames in flight > swap chain size?
     * If they were, it could result in clashes over resource usage. In a case with 3 images and 6 frames, Frame 1 may be 
     * tied to Image 1, and Frame 4 could also be tied to Image 1. While Frame 1 is presenting, Frame 4 could begin 
     * drawing in theory. But in practise would cause delays in execution because no image can be acquired from the swap 
     * chain yet 
    */
    const uint32_t g_maxFramesInFlight = 2;
    /* Statically allocate max number of unique device resources in device mgr ahead of time. The actual number of unique 
     * device resources used will be set by user
    */
    const uint32_t g_maxDeviceResourcesCount = 1;
    /* This frame count marker is used to cycle textures at specific intervals. For example, the default texture can be  
     * replaced by a group of textures which will be cycled every 'X' frames
    */
    const uint32_t g_framesPerCycleTexture = 24;
    /* Keep track of instance ids for Collections. Note that, we are not taking into account of reserved ids used by
     * Collections which may result in collision
    */
    uint32_t g_collectionsId = 0;
}   // namespace Renderer
#endif  // VK_CONFIG_H