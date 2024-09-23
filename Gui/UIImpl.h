#ifndef UI_IMPL_H
#define UI_IMPL_H
/* Imgui is a bloat-free graphical user interface library for C++. It outputs vertex buffers and a small list of
 * commands that you can easily render in your application. Because imgui doesn't know or touch graphics state directly
 * you can call its functions anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your
 * own rendering process)
*/
#include <imgui.h>
/* Note that, common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
 * You will use those if you want to use this rendering backend in your engine/app. Whereas, helper ImGui_ImplVulkanH_XXX
 * functions and structures are only used by example application in repo and by the backend itself (imgui_impl_vulkan.cpp),
 * but should probably not be used by your own engine/app code
*/
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "../Core/RenderPass/VKRenderPassMgr.h"
#include "../Core/Scene/VKSceneMgr.h"
#include "UIInput.h"

namespace Gui {
    class UIImpl: protected virtual Core::VKRenderPassMgr,
                  protected virtual Core::VKSceneMgr,
                  protected UIInput {
        private:
            static Log::Record* m_UIImplLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            static void errorHandlerCallBack (VkResult result) {
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_UIImplLog) << "Failed to ready ui "
                                            << "[" << string_VkResult (result) << "]"
                                            << std::endl;
                    throw std::runtime_error ("Failed to ready ui");
                }
            }

        public:
            UIImpl (void) {
                m_UIImplLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~UIImpl (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyUI (uint32_t deviceInfoId,
                          uint32_t uiRenderPassInfoId,
                          uint32_t uiSceneInfoId) {

                auto deviceInfo       = getDeviceInfo     (deviceInfoId);
                auto uiRenderPassInfo = getRenderPassInfo (uiRenderPassInfoId);
                auto uiSceneInfo      = getSceneInfo      (uiSceneInfoId);

                /* Note that, the application callbacks are initialized before initializing the imgui backend. However,
                 * there may be cases where callbacks would need to be initialized after, in such cases use imgui
                 * _RestoreCallbacks and _InstallCallbacks() methods to reinstall callbacks
                */
                readyKeyCallBack (deviceInfoId);
                /* Setup imgui context and style
                */
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();

                auto io         = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   /* Enable Keyboard Controls */
                /* The .ini file is used to save persistent window configuration between launches of the application
                 * (such as positions of windows that you have moved, or panels that have been opened/closed, etc). By
                 * default, imgui.ini is saved into the working directory of the program, but you change io.IniFilename
                 * to point to your own filename (pointer data owned by you)
                */
                io.IniFilename  = g_styleSettings.iniSaveFilePath;

                ImGui::StyleColorsDark();
                /* [ X ] Setup custom style here
                */

                /* Setup platform/renderer backends
                 * ImGui_ImplGlfw_InitForVulkan() lets imgui interact with GLFW in a non intrusive way. For example,
                 * imgui will have access to the keyboard and mouse events but will still run the user registered
                 * callbacks afterwards (chaining glfw callbacks). To let the library help you with that, we need to
                 * set install_callbacks to true and provide it with the GLFW window
                */
                ImGui_ImplGlfw_InitForVulkan (deviceInfo->resource.window, true);
                /* ImGui_ImplVulkan_InitInfo struct is a big bridge between the graphics engine and imgui as a lot of
                 * vulkan internals are exposed here
                */
                ImGui_ImplVulkan_InitInfo info{};
                info.Instance        = deviceInfo->resource.instance;
                /* In vulkan, most operations are done through command buffers that are submitted to a queue. A queue
                 * comes from a queue family that allows a limited subset of operations. For example, a family could only
                 * allow compute operations or transfer operations. Imgui is looking for a graphics queue and the
                 * related family
                */
                info.QueueFamily     = deviceInfo->meta.graphicsFamilyIndex.value();
                info.Queue           = deviceInfo->resource.graphicsQueue;
                info.PhysicalDevice  = deviceInfo->resource.phyDevice;
                info.Device          = deviceInfo->resource.logDevice;
                info.ImageCount      = deviceInfo->params.swapChainSize;
                info.MinImageCount   = deviceInfo->params.minSwapChainImageCount;
                info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
                info.RenderPass      = uiRenderPassInfo->resource.renderPass;
                info.Subpass         = 0;
                info.PipelineCache   = VK_NULL_HANDLE;
                info.DescriptorPool  = uiSceneInfo->resource.descriptorPool;
                /* The Allocator field can be used to pass a specific memory allocator to the vulkan functions called
                 * by imgui. You can pass nullptr if you don’t have any
                */
                info.Allocator       = nullptr;
                info.CheckVkResultFn = errorHandlerCallBack;

                ImGui_ImplVulkan_Init (&info);

                /* Note that, if no fonts are loaded, imgui will use the default font. You can also load multiple fonts
                 * and use push and pop font methods to select them. If the font file cannot be loaded, the function will
                 * return a nullptr. The fonts will be rasterized at a given size (w/ oversampling) and stored into a
                 * texture in ImGui_ImplXXXX_NewFrame call
                */
                /* [ X ] Add fonts here
                */
            }

            /* In order to differentiate between mouse/keyboard usage in imgui window vs the application window, we can
             * use the boolean flags provided by imgui. When WantCaptureMouse is set, you need to discard/hide the mouse
             * inputs from your underlying application, similarly, when WantCaptureKeyboard is set, you need to discard/
             * hide the keyboard inputs from your underlying application. Generally you may always pass all inputs to
             * imgui, and hide them from your application based on these flags
            */
            bool isMouseCapturedByUI (void) {
                auto io = ImGui::GetIO();
                return io.WantCaptureMouse;
            }

            bool isKeyBoardCapturedByUI (void) {
                auto io = ImGui::GetIO();
                return io.WantCaptureKeyboard;
            }

            void createUIFrame (void) {
                /* Start the imgui frame
                */
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                /* [ X ] Add window here instead of demo window
                */
                bool showDemo = true;
                ImGui::ShowDemoWindow (&showDemo);

                /* Prepare the data for rendering so you can call GetDrawData()
                */
                ImGui::Render();
            }

            void drawUIFrame (VkCommandBuffer commandBuffer) {
                /* Record imgui primitives into command buffer
                */
                ImGui_ImplVulkan_RenderDrawData (ImGui::GetDrawData(), commandBuffer);
            }

            void cleanUp (uint32_t deviceInfoId) {
                UIInput::cleanUp (deviceInfoId);

                ImGui_ImplVulkan_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
            }
    };

    Log::Record* UIImpl::m_UIImplLog;
}   // namespace Gui
#endif  // UI_IMPL_H