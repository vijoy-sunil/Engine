#ifndef UI_PLOT_H
#define UI_PLOT_H
/* Implot is an immediate mode, GPU accelerated plotting library for imgui. Implot is well suited for visualizing program
 * data in real-time or creating interactive plots, and requires minimal code to integrate. Just like imgui, it does not
 * burden the end user with GUI state management, avoids STL containers and C++ headers, and has no external dependencies
 * except for imgui itself
*/
#include <implot.h>
#include <implot_internal.h>
#include "../UIConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Gui {
    class UIPlot {
        private:
            struct PlotDataInfo {
                struct Meta {
                    const char* label;
                    uint32_t insertIdx;

                    float history;
                    float xMin;
                    float xMax;
                    float yMin;
                    float yMax;

                    float sum;
                    float average;
                    /* Note that buffer size refers to current number of items in the buffer, whereas buffer capacity
                     * refers to max number of items that can fit in. This is used for computing the rolling average
                    */
                    size_t bufferSize;
                } meta;

                struct State {
                    bool plotVsTime;
                } state;

                struct Resource {
                    std::vector <std::pair <float, float>> buffer;
                } resource;

                struct Parameters {
                    ImPlotFlags plotFlags;
                    ImPlotAxisFlags plotAxisFlags;
                    ImPlotLineFlags plotLineFlags;
                } params;
            };
            std::unordered_map <uint32_t, PlotDataInfo> m_plotDataInfoPool;

            Log::Record* m_UIPlotLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deletePlotDataInfo (uint32_t plotDataInfoId) {
                if (m_plotDataInfoPool.find (plotDataInfoId) != m_plotDataInfoPool.end()) {
                    m_plotDataInfoPool.erase (plotDataInfoId);
                    return;
                }

                LOG_ERROR (m_UIPlotLog) << "Failed to delete plot data info "
                                        << "[" << plotDataInfoId << "]"
                                        << std::endl;
                throw std::runtime_error ("Failed to delete plot data info");
            }

            PlotDataInfo* getPlotDataInfo (uint32_t plotDataInfoId) {
                if (m_plotDataInfoPool.find (plotDataInfoId) != m_plotDataInfoPool.end())
                    return &m_plotDataInfoPool[plotDataInfoId];

                LOG_ERROR (m_UIPlotLog) << "Failed to find plot data info "
                                        << "[" << plotDataInfoId << "]"
                                        << std::endl;
                throw std::runtime_error ("Failed to find plot data info");
            }

        public:
            UIPlot (void) {
                m_UIPlotLog  = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~UIPlot (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyPlotDataInfo (uint32_t plotDataInfoId,
                                    const char* label,
                                    float history,
                                    float xMin, float xMax,
                                    float yMin, float yMax,
                                    bool plotVsTime,
                                    size_t bufferCapacity,
                                    ImPlotFlags plotFlags,
                                    ImPlotAxisFlags plotAxisFlags,
                                    ImPlotLineFlags plotLineFlags) {

                if (m_plotDataInfoPool.find (plotDataInfoId) != m_plotDataInfoPool.end()) {
                    LOG_ERROR (m_UIPlotLog) << "Plot data info id already exists "
                                            << "[" << plotDataInfoId << "]"
                                            << std::endl;
                    throw std::runtime_error ("Plot data info id already exists");
                }

                PlotDataInfo info{};
                info.meta.label           = label;
                info.meta.insertIdx       = 0;

                info.meta.history         = history;
                info.meta.xMin            = xMin;
                info.meta.xMax            = xMax;
                info.meta.yMin            = yMin;
                info.meta.yMax            = yMax;

                info.meta.sum             = 0.0f;
                info.meta.average         = 0.0f;
                info.meta.bufferSize      = 0;

                info.state.plotVsTime     = plotVsTime;
                info.resource.buffer.resize (bufferCapacity);

                info.params.plotFlags     = plotFlags;
                info.params.plotAxisFlags = plotAxisFlags;
                info.params.plotLineFlags = plotLineFlags;

                m_plotDataInfoPool[plotDataInfoId] = info;
            }

            void createPlotTable (const std::vector <uint32_t>& plotDataInfoIds,
                                  const std::vector <std::pair <float, float>>& dataPoints,
                                  ImGuiTableFlags tableFlags,
                                  ImPlotColormap colorMap) {

                ImPlot::PushColormap  (colorMap);
                if (ImGui::BeginTable ("##table", 2, tableFlags, ImVec2 (0.0f, 0.0f))) {

                    uint32_t rowIdx = 0;
                    for (auto const& infoId: plotDataInfoIds) {
                        auto plotDataInfo = getPlotDataInfo (infoId);
                        std::string label = "\n%0.3f";
                        label             = plotDataInfo->meta.label + label;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex (0);
                        ImGui::Text                (label.c_str(), plotDataInfo->meta.average);

                        ImGui::TableSetColumnIndex (1);
                        ImGui::PushID              (rowIdx);
                        ImPlot::SetNextLineStyle   (ImPlot::GetColormapColor (rowIdx));
                        createPlotData             (infoId,
                                                    dataPoints[rowIdx],
                                                    g_plotSettings.size);
                        ImGui::PopID();

                        rowIdx++;
                    }
                    ImGui::EndTable();
                }
                ImPlot::PopColormap();
            }

            void createPlotData (uint32_t plotDataInfoId,
                                 std::pair <float, float> dataPoint,
                                 ImVec2 plotSize) {

                auto plotDataInfo = getPlotDataInfo (plotDataInfoId);
                /* Roll over insert index
                */
                if (plotDataInfo->meta.insertIdx == plotDataInfo->resource.buffer.capacity())
                    plotDataInfo->meta.insertIdx =  0;
                /* Compute rolling sum
                */
                plotDataInfo->meta.sum -= plotDataInfo->resource.buffer[plotDataInfo->meta.insertIdx].second;
                plotDataInfo->meta.sum += dataPoint.second;
                /* Add to buffer
                */
                plotDataInfo->resource.buffer[plotDataInfo->meta.insertIdx++] = dataPoint;
                /* Compute rolling average
                */
                plotDataInfo->meta.bufferSize = plotDataInfo->meta.bufferSize <
                                                plotDataInfo->resource.buffer.capacity() ?
                                                plotDataInfo->meta.bufferSize + 1:
                                                plotDataInfo->resource.buffer.capacity();

                plotDataInfo->meta.average    = plotDataInfo->meta.sum/plotDataInfo->meta.bufferSize;


                if (ImPlot::BeginPlot        ("##plot", plotSize, plotDataInfo->params.plotFlags)) {

                    ImPlot::SetupAxes        (nullptr,
                                              nullptr,
                                              plotDataInfo->params.plotAxisFlags,
                                              plotDataInfo->params.plotAxisFlags);

                    if (plotDataInfo->state.plotVsTime)
                    ImPlot::SetupAxisLimits  (ImAxis_X1,
                                              dataPoint.first - plotDataInfo->meta.history,
                                              dataPoint.first,
                                              ImGuiCond_Always);
                    else
                    ImPlot::SetupAxisLimits  (ImAxis_X1,
                                              plotDataInfo->meta.xMin,
                                              plotDataInfo->meta.xMax,
                                              ImGuiCond_Always);

                    ImPlot::SetupAxisLimits  (ImAxis_Y1,
                                              plotDataInfo->meta.yMin,
                                              plotDataInfo->meta.yMax,
                                              ImGuiCond_Always);

                    ImPlot::PlotLine         ("##plotLine",
                                              &plotDataInfo->resource.buffer[0].first,
                                              &plotDataInfo->resource.buffer[0].second,
                                              plotDataInfo->resource.buffer.capacity(),
                                              plotDataInfo->params.plotLineFlags,
                                              plotDataInfo->meta.insertIdx,
                                              2 * sizeof (float));
                    ImPlot::EndPlot();
                }
            }

            void dumpPlotDataInfoPool (void) {
                LOG_INFO (m_UIPlotLog) << "Dumping plot data info pool"
                                       << std::endl;

                for (auto const& [key, val]: m_plotDataInfoPool) {
                    LOG_INFO (m_UIPlotLog) << "Plot data info id "
                                           << "[" << key << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Label "
                                           << "[" << val.meta.label << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Insert idx "
                                           << "[" << val.meta.insertIdx << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "History "
                                           << "[" << val.meta.history << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "X minimum "
                                           << "[" << val.meta.xMin << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "X maximum "
                                           << "[" << val.meta.xMax << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Y minimum "
                                           << "[" << val.meta.yMin << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Y maximum "
                                           << "[" << val.meta.yMax << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Sum "
                                           << "[" << val.meta.sum << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Average "
                                           << "[" << val.meta.average << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Buffer size "
                                           << "[" << val.meta.bufferSize << "]"
                                           << std::endl;

                    std::string boolString = val.state.plotVsTime == true ? "TRUE": "FALSE";
                    LOG_INFO (m_UIPlotLog) << "Plot vs time state "
                                           << "[" << boolString << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Buffer capacity "
                                           << "[" << val.resource.buffer.capacity() << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Plot flags "
                                           << "[" << val.params.plotFlags << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Plot axis flags "
                                           << "[" << val.params.plotAxisFlags << "]"
                                           << std::endl;

                    LOG_INFO (m_UIPlotLog) << "Plot line flags "
                                           << "[" << val.params.plotLineFlags << "]"
                                           << std::endl;
                }
            }

            void cleanUp (uint32_t plotDataInfoId) {
                deletePlotDataInfo (plotDataInfoId);
            }
    };
}   // namespace Gui
#endif  // UI_PLOT_H