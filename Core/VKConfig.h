#ifndef VK_CONFIG_H
#define VK_CONFIG_H

#include <stdlib.h>

namespace Renderer {
/* Disable validation layers and logging
*/
#define DEBUG_DISABLE               (false)
/* Manually inject vertex attributes and indices instead of populating them from imported model, usefult for testing
*/
#define OVERRIDE_MODEL_IMPORT       (false)
/* Window settings (resolution is in screen coordinates)
*/
struct windowSettings {
    const int width   = 800;
    const int height  = 600;
    const char* title = "WINDOW_";
} g_windowSettings;
/* File/directory path settings
*/
struct pathSettings {
    const char* logSaveDir            = "Build/Log/";
    const char* model                 = "SandBox/Models/Model_3.obj";
    const char* mtlFileDir            = "SandBox/Models/";
    const char* defaultDiffuseTexture = "SandBox/Textures/tex_10x10_#D91EFF.png";
    const char* vertexShaderBinary    = "Build/Bin/vert.spv";
    const char* fragmentShaderBinary  = "Build/Bin/frag.spv"; 
} g_pathSettings;
/* Frames in flight
 * As of now, we are required to wait on the previous frame to finish before we can start rendering the next which 
 * results in unnecessary idling of the host. The way to fix this is to allow multiple frames to be in-flight at once, 
 * that is to say, allow the rendering of one frame to not interfere with the recording of the next. Any resource that 
 * is accessed and modified during rendering must be duplicated. Thus, we need multiple command buffers, semaphores, and 
 * fences. First, defines how many frames should be processed concurrently
 * 
 * We choose the number 2 because we don't want the CPU to get too far ahead of the GPU. With 2 frames in flight, the 
 * CPU and the GPU can be working on their own tasks at the same time. If the CPU finishes early, it will wait till the 
 * GPU finishes rendering before submitting more work. With 3 or more frames in flight, the CPU could get ahead of the 
 * GPU, adding frames of latency as shown in the scenario below:
 * 
 * What happens if frames in flight > swap chain size?
 * If they were, it could result in clashes over resource usage. In a case with 3 images and 6 frames, Frame 1 may be 
 * tied to Image 1, and Frame 4 could also be tied to Image 1. While Frame 1 is presenting, Frame 4 could begin drawing 
 * in theory. But in practise would cause delays in execution because no image can be acquired from the swap chain yet 
*/
const size_t g_maxFramesInFlight = 2;
/* Statically allocate max number of unique device resources in device mgr ahead of time. The actual number of unique 
 * device resources used will be set by user
*/
const size_t g_maxDeviceResourcesCount = 1;
/* Keep track of instance ids for Collections. Note that, we are not taking into account of reserved ids used by
 * Collections which may result in collision
*/
size_t g_collectionsId = 0;
}   // namespace Renderer
#endif  // VK_CONFIG_H