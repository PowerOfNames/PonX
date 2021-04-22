#include "pxpch.h"
#include "Povox/Scene/Scene.h"

#include "Povox/Scene/Components.h"
#include "Povox/Renderer/Renderer2D.h"
#include <glm/glm.hpp>

#include "Povox/Scene/Entity.h"

namespace Povox {

	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = Entity(m_Registry.create(), this);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>(name);
		tag.Tag = name.empty() ? "Unnamed Entity" : name;

		return entity;
	}

	void Scene::OnUpdate(Timestep ts)
	{
		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
			
			Renderer2D::DrawQuad(transform, sprite.Color);
		}
	}
}