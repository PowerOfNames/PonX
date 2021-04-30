#include "SceneHierarchyPanel.h"

#include "Povox/Scene/Components.h"

#include <glm/gtc/type_ptr.hpp>
#include <ImGui/imgui.h>

namespace Povox {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");
		if (ImGui::TreeNode("Entities"))
		{
			m_Context->m_Registry.each([&](auto entityID)
			{
				Entity entity{ entityID, m_Context.get() };
				DrawEntityNode(entity);
			});

			ImGui::TreePop();
		}

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
		{
			m_SelectionContext = {};
		}
		ImGui::End();

		ImGui::Begin("Properties");

		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		if (opened)
		{
			ImGui::Text("%s", tag.c_str());
			ImGui::TreePop();
		}

	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));

			ImGui::Text("Name");
			ImGui::SameLine();
			if (ImGui::InputTextWithHint("", tag.c_str(), buffer, IM_ARRAYSIZE(buffer)))
			{
				tag = std::string(buffer);
			}
			ImGui::Separator();
		}

		if (entity.HasComponent<TransformComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
			{
				auto& transform = entity.GetComponent<TransformComponent>().Transform;

				ImGui::Text("Position");
				ImGui::SameLine();
				ImGui::DragFloat3("", glm::value_ptr(transform[3]), 0.5);


				ImGui::Separator();
				ImGui::TreePop();	
			}
		}
	}
}