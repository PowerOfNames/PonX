#include "SciParticleInformationPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

namespace Povox {

	SciParticleInformationPanel::SciParticleInformationPanel(const Povox::Ref<SciParticleSet>& context)
	{
		SetContext(context);
	}

	void SciParticleInformationPanel::SetContext(const Povox::Ref<SciParticleSet>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SciParticleInformationPanel::OnImGuiRender()
	{
// 		ImGui::Begin("Particle Sets");
// 		m_Context->m_Registry.each([&](auto entityID)
// 			{
// 				Entity entity{ entityID, m_Context.get() };
// 				DrawEntityNode(entity);
// 			});
// 
// 		// Right click on blank space
// 		if (ImGui::BeginPopupContextWindow(0, 1, false))
// 		{
// 			if (ImGui::MenuItem("Create empty entity", NULL, false, true))
// 				m_Context->CreateEntity("New Entity");
// 
// 			ImGui::EndPopup();
// 		}
// 
// 		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
// 		{
// 			m_SelectionContext = {};
// 		}
// 		ImGui::End();
// 
// 		ImGui::Begin("Properties");
// 		if (m_SelectionContext)
// 		{
// 			DrawComponents(m_SelectionContext);
// 		}
// 
// 		ImGui::End();
	}

	void SciParticleInformationPanel::DrawParticleNode(const BufferLayout& particle)
	{
// 		auto& tag = entity.GetComponent<TagComponent>().Tag;
// 		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
// 
// 		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
// 		if (ImGui::IsItemClicked())
// 		{
// 			m_SelectionContext = entity;
// 		}
// 
// 		bool entityDeleted = false;
// 		if (ImGui::BeginPopupContextItem(0, 1))
// 		{
// 			//m_SelectionContext = entity;
// 			if (ImGui::MenuItem("Destroy Entity", NULL, false, true))
// 			{
// 				entityDeleted = true;
// 			}
// 
// 			ImGui::EndPopup();
// 		}
// 
// 		//TODO: child entities
// 		if (opened)
// 		{
// 			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
// 			if (ImGui::TreeNodeEx((void*)(uint64_t)6786987, flags, tag.c_str()))
// 			{
// 				ImGui::TreePop();
// 			}
// 			ImGui::TreePop();
// 		}
// 
// 
// 		//check at the end if entity was destroyed
// 		if (entityDeleted)
// 		{
// 			m_Context->DestroyEntity(entity);
// 			if (m_SelectionContext == entity)
// 				m_SelectionContext = {};
// 		}
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];


		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.5f, 0.0f, 0.0f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.1f, 0.1f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopStyleColor(2);
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.5f, 0.0f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.1f, 0.8f, 0.1f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopStyleColor(2);
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.5f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.1f, 0.1f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopStyleColor(2);
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);

		ImGui::PopID();
	}

}
