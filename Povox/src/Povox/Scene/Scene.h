#pragma once

#include "Povox/Core/Timestep.h"
#include "Povox/Core/UUID.h"

#include "Povox/Renderer/EditorCamera.h"
#include "Povox/Renderer/Renderer2D.h"


#include <entt.hpp>

namespace Povox {

	class Entity;
	class Renderer2D;
	class Renderer2DStatistics;

	class Scene
	{
	public:
		Scene(uint32_t width, uint32_t height);
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntity(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);


		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		Entity GetPrimaryCameraEntity();

		// TODO: temp
		inline const Renderer2DStatistics& GetStats() const { return m_Renderer2D->GetStatistics(); }
		inline void ResetStats() { m_Renderer2D->ResetStats(); }
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		Ref<Renderer2D> m_Renderer2D = nullptr;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
	};
}
