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
		m_SelectionContext = {};
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


	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		//TODO: Good looks stuff in here

		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen, name.c_str());
			
			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, IM_ARRAYSIZE(buffer)))
			{
				tag = std::string(buffer);
			}
			ImGui::Separator();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component) 
		{
			auto& transform = component.Transform;
			ImGui::Text("Position");
			ImGui::SameLine();
			ImGui::DragFloat3("", glm::value_ptr(transform[3]), 0.5);

			ImGui::Separator();
		});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];

			//TODO: Needs to set all other cameras to not being primary!
			ImGui::Checkbox("Primary", &component.Primary);

			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (ImGui::DragFloat("Vertical FOV", &verticalFOV))
					camera.SetOrthographicSize(glm::radians(verticalFOV));

				float perspectiveNearClip = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near Clip", &perspectiveNearClip))
					camera.SetPerspectiveNearClip(perspectiveNearClip);

				float perspectiveFarClip = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far Clip", &perspectiveFarClip))
					camera.SetPerspectiveFarClip(perspectiveFarClip);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNearClip = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near Clip", &orthoNearClip))
					camera.SetOrthographicNearClip(orthoNearClip);

				float orthoFarClip = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far Clip", &orthoFarClip))
					camera.SetOrthographicFarClip(orthoFarClip);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}

			ImGui::Separator();
		});

		DrawComponent <SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component)
		{
				auto& color = component.Color;

				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit4("", glm::value_ptr(color));

				ImGui::Separator();
		});
	}
}