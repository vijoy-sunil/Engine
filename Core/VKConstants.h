#ifndef VK_CONSTANTS_H
#define VK_CONSTANTS_H

namespace Renderer {

/* Resolution in screen coordinates
*/
#define WINDOW_WIDTH                800
#define WINDOW_HEIGHT               600
#define WINDOW_TITLE                "VULKAN WINDOW"
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
#define MAX_FRAMES_IN_FLIGHT        2
/* Specify the maximum number of command buffers that will be submitted to the transfer queue
*/
#define MAX_TRANSFERS_IN_QUEUE      2
/* Path to shader files binary
*/
#define VERTEX_SHADER_BINARY        "Build/Bin/vert.spv"
#define FRAGMENT_SHADER_BINARY      "Build/Bin/frag.spv"

#define APPLICATION_NAME            "VULKAN APPLICATION"

}   // namespace Renderer
#endif  // VK_CONSTANTS_H