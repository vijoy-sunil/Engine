#ifndef UI_IMPL_H
#define UI_IMPL_H
/* Imgui is a bloat-free graphical user interface library for C++. It outputs vertex buffers and a small list of
 * commands that you can easily render in your application. Because imgui doesn't know or touch graphics state directly
 * you can call its functions anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your
 * own rendering process)
*/
#include <imgui_impl_glfw.h>
#include "../Core/RenderPass/VKRenderPassMgr.h"
#include "UIWindow.h"

namespace Gui {
    class UIImpl: protected virtual Core::VKRenderPassMgr,
                  protected UIWindow {
        private:
            bool m_showWorldCollectionWindow;
            bool m_showPropertyEditorWindow;
            bool m_showMetricsOverlay;
            bool m_showBoundingBox;
            bool m_showShadow;

            e_overlayLocation m_metricsOverlayLocation;

            uint32_t m_frameDeltaPlotDataInfoId;
            uint32_t m_fpsPlotDataInfoId;

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
                m_showWorldCollectionWindow = g_defaultStateSettings.showWindow.worldCollection;
                m_showPropertyEditorWindow  = g_defaultStateSettings.showWindow.propertyEditor;
                m_showMetricsOverlay        = g_defaultStateSettings.showWindow.metricsOverlay;
                m_showBoundingBox           = g_defaultStateSettings.showWindow.boundingBox;
                m_showShadow                = g_defaultStateSettings.showWindow.shadow;

                m_metricsOverlayLocation    = g_defaultStateSettings.overlayLocation.metrics;

                m_frameDeltaPlotDataInfoId  = 0;
                m_fpsPlotDataInfoId         = 1;

                m_UIImplLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~UIImpl (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyUI (uint32_t deviceInfoId,
                          const std::vector <uint32_t>& modelInfoIds,
                          uint32_t uiRenderPassInfoId,
                          const std::vector <uint32_t>& cameraInfoIds,
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

                auto& io        = ImGui::GetIO();
                /* Enable keyboard controls
                */
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                /* The .ini file is used to save persistent window configuration between launches of the application
                 * (such as positions of windows that you have moved, or panels that have been opened/closed, etc). By
                 * default, imgui.ini is saved into the working directory of the program, but you change io.IniFilename
                 * to point to your own filename (pointer data owned by you)
                */
                io.IniFilename                                   = g_styleSettings.iniSaveFilePath;
                io.ConfigWindowsMoveFromTitleBarOnly             = true;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG UI - STYLE                                                                              |
                 * |------------------------------------------------------------------------------------------------|
                */
                auto& style                                      = ImGui::GetStyle();
                style.Alpha                                      = g_styleSettings.alpha.global;
                style.DisabledAlpha                              = g_styleSettings.alpha.disabled;
                style.HoverDelayShort                            = g_styleSettings.hoverDelay;
                /* Padding
                */
                style.WindowPadding                              = g_styleSettings.padding.window;
                style.FramePadding                               = g_styleSettings.padding.frame;
                style.CellPadding                                = g_styleSettings.padding.cell;
                /* Rounding
                */
                style.WindowRounding                             = g_styleSettings.rounding.window;
                style.FrameRounding                              = g_styleSettings.rounding.frame;
                style.ChildRounding                              = g_styleSettings.rounding.child;
                style.PopupRounding                              = g_styleSettings.rounding.popUp;
                style.ScrollbarRounding                          = g_styleSettings.rounding.scrollBar;
                /* Border size
                */
                style.WindowBorderSize                           = g_styleSettings.borderSize.window;
                style.FrameBorderSize                            = g_styleSettings.borderSize.frame;
                style.ChildBorderSize                            = g_styleSettings.borderSize.child;
                style.PopupBorderSize                            = g_styleSettings.borderSize.popUp;
                /* Spacing
                */
                style.IndentSpacing                              = g_styleSettings.spacing.intend;
                style.ItemSpacing                                = g_styleSettings.spacing.item;
                style.ItemInnerSpacing                           = g_styleSettings.spacing.itemInner;
                /* Size
                */
                style.ScrollbarSize                              = g_styleSettings.size.scrollBar;
                /* Alignment
                */
                style.WindowTitleAlign                           = g_styleSettings.alignment.windowTitle;
                style.ButtonTextAlign                            = g_styleSettings.alignment.buttonText;
                /* Color
                */
                style.Colors[ImGuiCol_WindowBg]                  = g_styleSettings.color.windowBackground;

                style.Colors[ImGuiCol_TitleBgActive]             = g_styleSettings.color.titleBackgroundActive;
                style.Colors[ImGuiCol_TitleBg]                   = g_styleSettings.color.titleBackgroundInactive;
                style.Colors[ImGuiCol_TitleBgCollapsed]          = g_styleSettings.color.titleBackgroundCollapsed;

                style.Colors[ImGuiCol_HeaderHovered]             = g_styleSettings.color.headerHovered;
                style.Colors[ImGuiCol_HeaderActive]              = g_styleSettings.color.headerActive;
                style.Colors[ImGuiCol_Header]                    = g_styleSettings.color.header;

                style.Colors[ImGuiCol_FrameBgHovered]            = g_styleSettings.color.frameBackgroundHovered;
                style.Colors[ImGuiCol_FrameBgActive]             = g_styleSettings.color.frameBackgroundActive;
                style.Colors[ImGuiCol_FrameBg]                   = g_styleSettings.color.frameBackground;

                style.Colors[ImGuiCol_ChildBg]                   = g_styleSettings.color.childBackground;
                style.Colors[ImGuiCol_PopupBg]                   = g_styleSettings.color.popUpBackground;

                style.Colors[ImGuiCol_Text]                      = g_styleSettings.color.text;
                style.Colors[ImGuiCol_TextSelectedBg]            = g_styleSettings.color.textSelectedBackground;
                style.Colors[ImGuiCol_CheckMark]                 = g_styleSettings.color.checkMark;

                style.Colors[ImGuiCol_SeparatorHovered]          = g_styleSettings.color.separatorHovered;
                style.Colors[ImGuiCol_SeparatorActive]           = g_styleSettings.color.separatorActive;
                style.Colors[ImGuiCol_Separator]                 = g_styleSettings.color.separator;

                style.Colors[ImGuiCol_ScrollbarGrabHovered]      = g_styleSettings.color.scrollBarGrabHovered;
                style.Colors[ImGuiCol_ScrollbarGrabActive]       = g_styleSettings.color.scrollBarGrabActive;
                style.Colors[ImGuiCol_ScrollbarGrab]             = g_styleSettings.color.scrollBarGrab;
                style.Colors[ImGuiCol_ScrollbarBg]               = g_styleSettings.color.scrollBarBackground;

                style.Colors[ImGuiCol_ButtonHovered]             = g_styleSettings.color.buttonHovered;
                style.Colors[ImGuiCol_ButtonActive]              = g_styleSettings.color.buttonActive;
                style.Colors[ImGuiCol_Button]                    = g_styleSettings.color.button;

                style.Colors[ImGuiCol_ResizeGripHovered]         = g_styleSettings.color.resizeGripHovered;
                style.Colors[ImGuiCol_ResizeGripActive]          = g_styleSettings.color.resizeGripActive;
                style.Colors[ImGuiCol_ResizeGrip]                = g_styleSettings.color.resizeGrip;

                style.Colors[ImGuiCol_TableRowBg]                = g_styleSettings.color.tableRowBackground;
                style.Colors[ImGuiCol_TableRowBgAlt]             = g_styleSettings.color.tableRowBackgroundAlt;
                style.Colors[ImGuiCol_TableBorderStrong]         = g_styleSettings.color.tableBorder;
                style.Colors[ImGuiCol_TableBorderLight]          = g_styleSettings.color.tableBorder;

                style.Colors[ImGuiCol_Border]                    = g_styleSettings.color.border;
                /* Unused colors
                */
                style.Colors[ImGuiCol_TextDisabled]              = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_BorderShadow]              = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_MenuBarBg]                 = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_SliderGrab]                = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_SliderGrabActive]          = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabHovered]                = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_Tab]                       = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabSelected]               = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabSelectedOverline]       = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabDimmed]                 = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabDimmedSelected]         = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TabDimmedSelectedOverline] = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_PlotLines]                 = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_PlotLinesHovered]          = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_PlotHistogram]             = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_PlotHistogramHovered]      = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TableHeaderBg]             = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_TextLink]                  = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_DragDropTarget]            = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_NavHighlight]              = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_NavWindowingHighlight]     = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_NavWindowingDimBg]         = g_styleSettings.color.unused;
                style.Colors[ImGuiCol_ModalWindowDimBg]          = g_styleSettings.color.unused;
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG UI - BACKEND                                                                            |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* ImGui_ImplGlfw_InitForVulkan() lets imgui interact with GLFW in a non intrusive way. For example,
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
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG UI - FONT AND ICON                                                                      |
                 * |------------------------------------------------------------------------------------------------|
                */
                /* Note that, if no fonts are loaded, imgui will use the default font. You can also load multiple fonts
                 * and use push and pop font methods to select them. The fonts will be rasterized at a given size (with
                 * oversampling) and stored into a texture in ImGui_ImplXXXX_NewFrame call
                */
                io.Fonts->AddFontFromFileTTF (g_styleSettings.font.filePath,
                                              g_styleSettings.font.size,
                                              nullptr,
                                              io.Fonts->GetGlyphRangesDefault());
                /* Merge icons into previous font
                */
                ImFontConfig config{};
                config.MergeMode        = true;
                config.PixelSnapH       = true;
                config.GlyphMinAdvanceX = g_styleSettings.icon.size;

                static const ImWchar glyphRanges[] = {
                    ICON_MIN_FA,
                    ICON_MAX_16_FA,
                    0
                };
                io.Fonts->AddFontFromFileTTF (g_styleSettings.icon.filePath,
                                              g_styleSettings.icon.size,
                                              &config,
                                              glyphRanges);
                /* |------------------------------------------------------------------------------------------------|
                 * | CONFIG UI WINDOW                                                                               |
                 * |------------------------------------------------------------------------------------------------|
                */
                readyUIWindow (modelInfoIds,
                               cameraInfoIds,
                               uiSceneInfoId,
                               m_frameDeltaPlotDataInfoId,
                               m_fpsPlotDataInfoId);
            }

            void createUIFrame (float frameDelta) {
                /* Start the imgui frame
                */
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

#if SHOW_DEMO_IMGUI
                ImGui::ShowDemoWindow();
#endif  // SHOW_DEMO_IMGUI
#if SHOW_DEMO_IMPLOT
                ImPlot::ShowDemoWindow();
#endif  // SHOW_DEMO_IMPLOT

                if (m_showWorldCollectionWindow) createWorldCollection (m_showWorldCollectionWindow);
                if (m_showPropertyEditorWindow)  createPropertyEditor  (m_showPropertyEditorWindow,
                                                                        m_showMetricsOverlay,
                                                                        m_showBoundingBox,
                                                                        m_showShadow);
                if (m_showMetricsOverlay)        createOverlay         ("##metrics",
                                                                        "Mertics",
                                                                        g_styleSettings.padding.overlay,
                                                                        m_metricsOverlayLocation,
                [&](void) {
                {
                    auto plotDataInfoIds     = std::vector <uint32_t> {
                        m_frameDeltaPlotDataInfoId,
                        m_fpsPlotDataInfoId
                    };
                    /* Keep track of elapsed time in order to plot data vs time
                    */
                    static float elapsedTime = 0.0f;
                    elapsedTime             += ImGui::GetIO().DeltaTime;
                    float fps                = frameDelta == 0.0f ? 0.0f: (1.0f / frameDelta);
                    auto dataPoints          = std::vector <std::pair <float, float>> {
                        {elapsedTime, frameDelta},
                        {elapsedTime, fps}
                    };
                    auto tableFlags          = ImGuiTableFlags_BordersOuter |
                                               ImGuiTableFlags_BordersV     |
                                               ImGuiTableFlags_RowBg;

                    createPlotTable (plotDataInfoIds,
                                     dataPoints,
                                     tableFlags,
                                     ImPlotColormap_Plasma);
                }
                });
                if (m_showBoundingBox)          {/* [ X ] Pending implementation */}
                if (m_showShadow)               {/* [ X ] Pending implementation */}
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

                auto plotDataInfoIds = std::vector <uint32_t> {
                    m_frameDeltaPlotDataInfoId,
                    m_fpsPlotDataInfoId
                };
                UIWindow::cleanUp (plotDataInfoIds);

                ImGui_ImplVulkan_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
            }
    };

    Log::Record* UIImpl::m_UIImplLog;
}   // namespace Gui
#endif  // UI_IMPL_H