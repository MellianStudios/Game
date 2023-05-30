#include "PerformanceDiagnostics.h"

#include <Base/Util/CPUInfo.h>
#include <Base/CVarSystem/CVarSystemPrivate.h>

#include <Game/Util/ServiceLocator.h>
#include <Game/ECS/Singletons/EngineStats.h>
#include <Game/Rendering/GameRenderer.h>
#include <Game/Rendering/Terrain/TerrainRenderer.h>
#include <Game/Application/EnttRegistries.h>
#include <Game/Util/ImguiUtil.h>

#include <entt/entt.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/implot.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <string>

namespace Editor
{
    PerformanceDiagnostics::PerformanceDiagnostics()
        : BaseEditor(GetName(), true)
	{

	}

	void PerformanceDiagnostics::DrawImGui()
	{
        GameRenderer* gameRenderer = ServiceLocator::GetGameRenderer();

        entt::registry* registry = ServiceLocator::GetEnttRegistries()->gameRegistry;
        ECS::Singletons::EngineStats& stats = registry->ctx().at<ECS::Singletons::EngineStats>();

        ECS::Singletons::EngineStats::Frame average = stats.AverageFrame(240);

        TerrainRenderer* terrainRenderer = gameRenderer->GetTerrainRenderer();

        // Draw hardware info
        CPUInfo cpuInfo = CPUInfo::Get();
        Renderer::Renderer* renderer = gameRenderer->GetRenderer();

        const std::string& cpuName = cpuInfo.GetPrettyName();
        const std::string& gpuName = gameRenderer->GetGPUName();

        if (ImGui::Begin(GetName()))
        {
            ImGui::Text("CPU: %s", cpuName.c_str());
            ImGui::Text("GPU: %s", gpuName.c_str());

            const std::string rightHeaderText = "Survived / Total (%%)";

            f32 textWidth = ImGui::CalcTextSize(rightHeaderText.c_str()).x;
            f32 windowWidth = ImGui::GetWindowContentRegionWidth();

            f32 textPos = windowWidth - textWidth;

            u32 numCascades = 0;// *CVarSystem::Get()->GetIntCVar("shadows.cascade.num");

            // Surviving Drawcalls
            {
                ImGui::Spacing();

                ImGui::Checkbox("Only for main view", &_drawCallStatsOnlyForMainView);
                bool showDrawCalls = ImGui::CollapsingHeader("Surviving Drawcalls");

                // If we are not collapsed, add a header that explains the values
                if (showDrawCalls)
                {
                    ImGui::SameLine(textPos);
                    ImGui::Text(rightHeaderText.c_str());
                    ImGui::Separator();
                }

                u32 totalDrawCalls = 0;
                u32 totalDrawCallsSurvived = 0;

                // Main view
                DrawCullingDrawcallStatsView(0, textPos, totalDrawCalls, totalDrawCallsSurvived, showDrawCalls);

                if (!_drawCallStatsOnlyForMainView)
                {
                    for (u32 i = 1; i < numCascades + 1; i++)
                    {
                        DrawCullingDrawcallStatsView(i, textPos, totalDrawCalls, totalDrawCallsSurvived, showDrawCalls);
                    }
                }

                // Always draw Total, if we are collapsed it will go on the collapsable header
                DrawCullingStatsEntry("Total", totalDrawCalls, totalDrawCallsSurvived, !showDrawCalls);
                ImGui::Spacing();
            }

            // Surviving Triangles
            {
                ImGui::Spacing();
                bool showTriangles = ImGui::CollapsingHeader("Surviving Triangles");

                if (showTriangles)
                {
                    ImGui::SameLine(textPos);
                    ImGui::Text(rightHeaderText.c_str());
                    ImGui::Separator();
                }

                u32 totalTriangles = 0;
                u32 totalTrianglesSurvived = 0;

                // Main view
                DrawCullingTriangleStatsView(0, textPos, totalTriangles, totalTrianglesSurvived, showTriangles);

                if (!_drawCallStatsOnlyForMainView)
                {
                    for (u32 i = 1; i < numCascades + 1; i++)
                    {
                        DrawCullingTriangleStatsView(i, textPos, totalTriangles, totalTrianglesSurvived, showTriangles);
                    }
                }

                DrawCullingStatsEntry("Total", totalTriangles, totalTrianglesSurvived, !showTriangles);
                ImGui::Spacing();
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Text("Frametimes (ms)");
            ImGui::Separator();

            // Draw Timing Graph
            {
                static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit /*| ImGuiTableFlags_BordersOuter*/ | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_ContextMenuInBody;

                if (ImGui::BeginTable("frametimes", 2, flags))
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("Total");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", average.deltaTimeS * 1000);
                    ImGui::TableNextColumn();

                    ImGui::Text("Update");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", average.simulationFrameTimeS * 1000);
                    ImGui::TableNextColumn();

                    ImGui::Text("Render CPU");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", average.renderFrameTimeS * 1000);
                    ImGui::TableNextColumn();

                    ImGui::Text("CPU wait for GPU");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", average.renderWaitTimeS * 1000);
                    ImGui::TableNextColumn();

                    ImGui::Text("GPU", average.gpuFrameTimeMS);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", average.gpuFrameTimeMS);
                    ImGui::TableNextColumn();
                    
                    ImGui::EndTable();
                }

                // Read the frame buffer to gather timings for the histograms
                std::vector<float> totalTimes;
                totalTimes.reserve(stats.frameStats.size());

                std::vector<float> updateTimes;
                updateTimes.reserve(stats.frameStats.size());

                std::vector<float> renderTimes;
                renderTimes.reserve(stats.frameStats.size());

                std::vector<float> waitTimes;
                waitTimes.reserve(stats.frameStats.size());

                std::vector<float> gpuTimes;
                gpuTimes.reserve(stats.frameStats.size());

                for (int i = 0; i < stats.frameStats.size(); i++)
                {
                    totalTimes.push_back(stats.frameStats[i].deltaTimeS * 1000);
                    updateTimes.push_back(stats.frameStats[i].simulationFrameTimeS * 1000);
                    renderTimes.push_back(stats.frameStats[i].renderFrameTimeS * 1000);
                    waitTimes.push_back(stats.frameStats[i].renderWaitTimeS * 1000);
                    gpuTimes.push_back(stats.frameStats[i].gpuFrameTimeMS);
                }

                ImPlot::SetNextAxesLimits(0.0, 120.0, 0, 33.0);

                //lock minimum Y to 0 (cant have negative ms)
                //lock X completely as its fixed 120 frames
                if (ImPlot::BeginPlot("Timing", ImVec2(400, 300)))
                {
                    ImPlot::SetupAxis(ImAxis_X1, "frame", ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_LockMin); // ImPlotAxisFlags_LockMin

                    ImPlot::PlotLine("Total", totalTimes.data(), (int)totalTimes.size());
                    ImPlot::PlotLine("Update", updateTimes.data(), (int)updateTimes.size());
                    ImPlot::PlotLine("Render", renderTimes.data(), (int)renderTimes.size());
                    ImPlot::PlotLine("CPU wait For GPU", waitTimes.data(), (int)waitTimes.size());
                    ImPlot::PlotLine("GPU", gpuTimes.data(), (int)gpuTimes.size());
                    ImPlot::EndPlot();
                }

                const std::vector<Renderer::TimeQueryID> frameTimeQueries = renderer->GetFrameTimeQueries();

                if (frameTimeQueries.size() > 0)
                {
                    if (ImGui::CollapsingHeader("Render Passes (GPU)"))
                    {
                        if (ImGui::BeginTable("passtimes", 2, flags))
                        {
                            for (u32 i = 0; i < frameTimeQueries.size(); i++)
                            {
                                const std::string& name = renderer->GetTimeQueryName(frameTimeQueries[i]);

                                f32 averageMS = 0.0f;
                                if (stats.AverageNamed(name, 240, averageMS))
                                {
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", name.c_str());
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%.3f", averageMS);
                                }
                            }

                            ImGui::EndTable();
                        }
                    }
                }
            }
        }
        ImGui::End();
	}

    void PerformanceDiagnostics::DrawCullingDrawcallStatsView(u32 viewID, f32 textPos, u32& totalDrawCalls, u32& totalSurvivingDrawcalls, bool showDrawcalls)
    {
        GameRenderer* gameRenderer = ServiceLocator::GetGameRenderer();
        TerrainRenderer* terrainRenderer = gameRenderer->GetTerrainRenderer();
        ModelRenderer* modelRenderer = gameRenderer->GetModelRenderer();
        
        const std::string rightHeaderText = "Survived / Total (%%)";

        std::string viewName = "Main View Drawcalls";
        if (viewID > 0)
        {
            viewName = "Shadow Cascade " + std::to_string(viewID - 1) + " Drawcalls";
        }

        bool showView = showDrawcalls;
        if (!_drawCallStatsOnlyForMainView)
        {
            if (showDrawcalls)
            {
                showView = ImGui::CollapsingHeader(viewName.c_str());
            }

            // If we are not collapsed, add a header that explains the values
            if (showView)
            {
                ImGui::SameLine(textPos);
                ImGui::Text(rightHeaderText.c_str());
                ImGui::Separator();
            }
        }
        
        u32 viewDrawCalls = 0;
        u32 viewDrawCallsSurvived = 0;

        bool viewSupportsOcclusionCulling = viewID == 0; // Only main view supports occlusion culling so far

        bool viewRendersTerrainCulling = true; // Only main view supports terrain culling so far
        bool viewRendersOpaqueModelsCulling = true;
        bool viewRendersTransparentModelsCulling = viewID == 0; // Only main view supports transparent cmodel culling so far
        bool viewRendersWaterCulling = viewID == 0; // Only main view supports mapObjectgs culling so far

        // Terrain
        if (viewRendersTerrainCulling)
        {
            u32 drawCalls = terrainRenderer->GetNumDrawCalls();
            viewDrawCalls += drawCalls;

            // Occluders
            if (viewSupportsOcclusionCulling)
            {
                u32 drawCallsSurvived = terrainRenderer->GetNumOccluderDrawCalls();

                if (showView)
                {
                    DrawCullingStatsEntry("Terrain Occluders", drawCalls, drawCallsSurvived, !showView);
                }
                viewDrawCallsSurvived += drawCallsSurvived;
            }

            // Geometry
            {
                u32 drawCallsSurvived = terrainRenderer->GetNumSurvivingDrawCalls(viewID);

                if (showView)
                {
                    DrawCullingStatsEntry("Terrain Geometry", drawCalls, drawCallsSurvived, !showView);
                }
                viewDrawCallsSurvived += drawCallsSurvived;
            };
        }

        // Opaque Models
        if (viewRendersOpaqueModelsCulling)
        {
            CullingResourcesBase& cullingResources = modelRenderer->GetOpaqueCullingResources();
            DrawCullingResourcesDrawcall("Model (O)", viewID, cullingResources, showView, viewSupportsOcclusionCulling, viewDrawCalls, viewDrawCallsSurvived);
        }

        // Transparent Models
        if (viewRendersTransparentModelsCulling)
        {
            CullingResourcesBase& cullingResources = modelRenderer->GetTransparentCullingResources();
            DrawCullingResourcesDrawcall("Model (T)", viewID, cullingResources, showView, viewSupportsOcclusionCulling, viewDrawCalls, viewDrawCallsSurvived);
        }

        // If showDrawcalls we always want to draw Total, if we are collapsed it will go on the collapsable header
        if (showDrawcalls)
        {
            DrawCullingStatsEntry("View Total", viewDrawCalls, viewDrawCallsSurvived, !showView);
        }

        totalDrawCalls += viewDrawCalls;
        totalSurvivingDrawcalls += viewDrawCallsSurvived;
    }

    void PerformanceDiagnostics::DrawCullingTriangleStatsView(u32 viewID, f32 textPos, u32& totalTriangles, u32& totalSurvivingTriangles, bool showTriangles)
    {
        GameRenderer* gameRenderer = ServiceLocator::GetGameRenderer();
        TerrainRenderer* terrainRenderer = gameRenderer->GetTerrainRenderer();
        ModelRenderer* modelRenderer = gameRenderer->GetModelRenderer();

        const std::string rightHeaderText = "Survived / Total (%%)";

        std::string viewName = "Main View Triangles";
        if (viewID > 0)
        {
            viewName = "Shadow Cascade " + std::to_string(viewID - 1) + " Triangles";
        }

        bool showView = showTriangles;
        if (!_drawCallStatsOnlyForMainView)
        {
            if (showTriangles)
            {
                showView = ImGui::CollapsingHeader(viewName.c_str());
            }

            // If we are not collapsed, add a header that explains the values
            if (showView)
            {
                ImGui::SameLine(textPos);
                ImGui::Text(rightHeaderText.c_str());
                ImGui::Separator();
            }
        }

        u32 viewTriangles = 0;
        u32 viewTrianglesSurvived = 0;

        bool viewSupportsOcclusionCulling = viewID == 0; // Only main view supports occlusion culling so far

        bool viewRendersTerrainCulling = true; // Only main view supports terrain culling so far
        bool viewRendersOpaqueModelsCulling = true;
        bool viewRendersTransparentCModelsCulling = viewID == 0; // Only main view supports transparent cmodel culling so far
        bool viewRendersWaterCulling = viewID == 0; // Only main view supports water culling so far

        // Terrain
        if (viewRendersTerrainCulling)
        {
            u32 triangles = terrainRenderer->GetNumTriangles();
            viewTriangles += triangles;

            // Occluders
            if (viewSupportsOcclusionCulling)
            {
                u32 trianglesSurvived = terrainRenderer->GetNumOccluderTriangles();

                if (showView)
                {
                    DrawCullingStatsEntry("Terrain Occluders", triangles, trianglesSurvived, !showView);
                }
                viewTrianglesSurvived += trianglesSurvived;
            }

            // Geometry
            {
                u32 trianglesSurvived = terrainRenderer->GetNumSurvivingGeometryTriangles(viewID);

                if (showView)
                {
                    DrawCullingStatsEntry("Terrain Geometry", triangles, trianglesSurvived, !showView);
                }
                viewTrianglesSurvived += trianglesSurvived;
            }
        }

        // Opaque Models
        if (viewRendersOpaqueModelsCulling)
        {
            CullingResourcesBase& cullingResources = modelRenderer->GetOpaqueCullingResources();
            DrawCullingResourcesDrawcall("Model (O)", viewID, cullingResources, showView, viewSupportsOcclusionCulling, viewTriangles, viewTrianglesSurvived);
        }

        // Opaque Models
        if (viewRendersOpaqueModelsCulling)
        {
            CullingResourcesBase& cullingResources = modelRenderer->GetTransparentCullingResources();
            DrawCullingResourcesDrawcall("Model (T)", viewID, cullingResources, showView, viewSupportsOcclusionCulling, viewTriangles, viewTrianglesSurvived);
        }

        // If showTriangles we always want to draw Total, if we are collapsed it will go on the collapsable header
        if (showTriangles)
        {
            DrawCullingStatsEntry("View Total", viewTriangles, viewTrianglesSurvived, !showView);
        }

        totalTriangles += viewTriangles;
        totalSurvivingTriangles += viewTrianglesSurvived;
    }

    void PerformanceDiagnostics::DrawCullingResourcesDrawcall(std::string prefix, u32 viewID, CullingResourcesBase& cullingResources, bool showView, bool viewSupportsOcclusionCulling, u32& viewDrawCalls, u32& viewDrawCallsSurvived)
    {
        u32 drawCalls = cullingResources.GetNumDrawCalls();
        viewDrawCalls += drawCalls;

        // Occluders
        if (viewSupportsOcclusionCulling && cullingResources.HasSupportForTwoStepCulling())
        {
            u32 drawCallsSurvived = cullingResources.GetNumSurvivingOccluderDrawCalls();

            if (showView)
            {
                DrawCullingStatsEntry(prefix + " Occluders", drawCalls, drawCallsSurvived, !showView);
            }
            viewDrawCallsSurvived += drawCallsSurvived;
        }

        // Geometry
        {
            u32 drawCallsSurvived = cullingResources.GetNumSurvivingDrawCalls(viewID);

            if (showView)
            {
                DrawCullingStatsEntry(prefix + " Geometry", drawCalls, drawCallsSurvived, !showView);
            }
            viewDrawCallsSurvived += drawCallsSurvived;
        };
    }

    void PerformanceDiagnostics::DrawCullingResourcesTriangle(std::string prefix, u32 viewID, CullingResourcesBase& cullingResources, bool showView, bool viewSupportsOcclusionCulling, u32& viewTriangles, u32& viewTrianglesSurvived)
    {
        u32 triangles = cullingResources.GetNumTriangles();
        viewTriangles += triangles;

        // Occluders
        if (viewSupportsOcclusionCulling && cullingResources.HasSupportForTwoStepCulling())
        {
            u32 trianglesSurvived = cullingResources.GetNumSurvivingOccluderTriangles();

            if (showView)
            {
                DrawCullingStatsEntry(prefix + " Occluders", triangles, trianglesSurvived, !showView);
            }
            viewTrianglesSurvived += trianglesSurvived;
        }

        // Geometry
        {
            u32 trianglesSurvived = cullingResources.GetNumSurvivingTriangles(viewID);

            if (showView)
            {
                DrawCullingStatsEntry(prefix + " Geometry", triangles, trianglesSurvived, !showView);
            }
            viewTrianglesSurvived += trianglesSurvived;
        }
    }

    void PerformanceDiagnostics::DrawCullingStatsEntry(std::string_view name, u32 drawCalls, u32 survivedDrawCalls, bool isCollapsed)
    {
        f32 percent = 0;
        if (drawCalls > 0) // Avoid division by 0 NaNs
        {
            percent = static_cast<f32>(survivedDrawCalls) / static_cast<f32>(drawCalls) * 100.0f;
        }

        char str[50];
        i32 strLength = StringUtils::FormatString(str, sizeof(str), "%s / %s (%.0f%%)", StringUtils::FormatThousandSeparator(survivedDrawCalls).c_str(), StringUtils::FormatThousandSeparator(drawCalls).c_str(), percent);

        f32 textWidth = ImGui::CalcTextSize(str).x;
        f32 windowWidth = ImGui::GetWindowContentRegionWidth();

        f32 textPos = windowWidth - textWidth;

        if (isCollapsed)
        {
            ImGui::SameLine(textPos);
            ImGui::Text("%s", str);
        }
        else
        {
            ImGui::Separator();
            ImGui::Text("%.*s:", name.length(), name.data());
            ImGui::SameLine(textPos);
            ImGui::Text("%s", str);
        }
    }
}