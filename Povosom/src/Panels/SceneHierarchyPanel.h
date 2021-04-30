#pragma once

#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"
#include "Povox/Scene/Scene.h"
#include "Povox/Scene/Entity.h"

namespace Povox {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);

		void SetContext(const Ref<Scene>& context);

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;

		Entity m_SelectionContext;
	};

}