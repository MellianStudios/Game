#include "Hierarchy.h"

#include <Game-Lib/Util/ServiceLocator.h>
#include <Game-Lib/Application/EnttRegistries.h>
#include <Game-Lib/ECS/Components/Name.h>
#include <Game-Lib/ECS/Components/AABB.h>
#include <Game-Lib/ECS/Util/CameraUtil.h>
#include <Game-Lib/Util/ImguiUtil.h>
#include <Game-Lib/Util/ImGui/FakeScrollingArea.h>
#include <Game-Lib/Editor/Inspector.h>

#include <entt/entt.hpp>
#include <imgui/imgui.h>

namespace Editor
{
    Hierarchy::Hierarchy()
        : BaseEditor(GetName(), BaseEditorFlags_DefaultVisible | BaseEditorFlags_EditorOnly)
    {

    }

    void Hierarchy::SetInspector(Inspector* inspector)
    {
        _inspector = inspector;
    }

    i32 GetSelectedIndex(const entt::registry& registry, entt::entity entityToFind) 
    {
        i32 index = 0;
        for (auto entity : registry.view<ECS::Components::Name>()) 
        {
            if (entity == entityToFind) 
            {
                return index;
            }
            ++index;
        }

        return -1;
    }

    void Hierarchy::OnModeUpdate(bool mode)
    {
        SetIsVisible(mode);
    }

    void Hierarchy::DrawImGui()
    {
        entt::registry* registry = ServiceLocator::GetEnttRegistries()->gameRegistry;

        if (ImGui::Begin(GetName(), &IsVisible()))
        {
            Util::Imgui::ItemRowsBackground();
            auto view = registry->view<ECS::Components::Name>();

            vec2 itemSize = vec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing());
            if (_scrollToSelected)
            {
                i32 selectedIndex = GetSelectedIndex(*registry, _selectedEntity);
                if (selectedIndex != -1)
                {
                    f32 scrollPos = itemSize.y * selectedIndex;
                    ImGui::SetScrollY(scrollPos);
                }
                _scrollToSelected = false;
            }

            FakeScrollingArea scrollingArea(itemSize, static_cast<i32>(view.size()));
            if (scrollingArea.Begin())
            {
                i32 firstVisibleItem = scrollingArea.GetFirstVisibleItem();
                i32 lastVisibleItem = scrollingArea.GetLastVisibleItem();

                size_t index = 0;
                view.each([&](entt::entity entity, ECS::Components::Name& name)
                {
                    if (index < firstVisibleItem || index > lastVisibleItem)
                    {
                        index++;
                        return; // Skip non-visible items
                    }

                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

                    if (entity == _selectedEntity)
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;

                        ImGui::PushStyleColor(ImGuiCol_Header, vec4(0.4f, 0.4f, 0.4f, 1.0f));
                    }

                    u32 i = entt::to_integral(entity);
                    if (ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", name.name.c_str()))
                    {
                        ImGui::TreePop();
                    }

                    if (entity == _selectedEntity)
                    {
                        ImGui::PopStyleColor();
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        OnDoubleClicked(registry, entity);
                    }
                    else if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen())
                    {
                        _selectedEntity = entity;
                        _inspector->SelectEntity(entity);
                    }

                    index++;
                });
                scrollingArea.End();
            }
        }
        ImGui::End();
    }

    void Hierarchy::SelectEntity(entt::entity entity)
    {
        _selectedEntity = entity;
        _scrollToSelected = entity != entt::null;
    }

    void Hierarchy::OnDoubleClicked(entt::registry* registry, entt::entity entity)
    {
        if (!registry->all_of<ECS::Components::WorldAABB>(entity))
            return;

        auto& worldAABB = registry->get<ECS::Components::WorldAABB>(entity);

        vec3 position = (worldAABB.min + worldAABB.max) * 0.5f;	
        f32 radius = glm::distance(worldAABB.min, worldAABB.max) * 0.5f;

        ECS::Util::CameraUtil::CenterOnObject(position, radius);
    }
}
