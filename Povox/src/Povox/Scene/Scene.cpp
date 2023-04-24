#include "pxpch.h"
#include "Povox/Scene/Scene.h"

#include "Povox/Scene/Components.h"
#include "Povox/Renderer/Renderer2D.h"
#include <glm/glm.hpp>

#include "Povox/Scene/Entity.h"
#include "Povox/Scene/ScriptableEntity.h"

namespace Povox {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntity(UUID(), name);
	}

	Entity Scene::CreateEntity(UUID uuid, const std::string& name)
	{
		Entity entity = Entity(m_Registry.create(), this);
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>(name);
		tag.Tag = name.empty() ? "Unnamed Entity" : name;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateRuntime(Timestep deltatime)
	{
		//Update scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					// TODO: Move to Scene::OnScenePlay()
					if (!nsc.Instance)
					{
						nsc.Instance = nsc.InstantiateScript();
						nsc.Instance->m_Entity = Entity{ entity, this };
						nsc.Instance->OnCreate();
					}

					nsc.Instance->OnUpdate(deltatime);
				});
		}

		//Render sprites
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto group = m_Registry.group<CameraComponent, TransformComponent>();
			for (auto entity : group)
			{
				auto [camera, transform] = group.get<CameraComponent, TransformComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if(mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);
			auto group = m_Registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [spriteComp, transformComp] = group.get<SpriteRendererComponent, TransformComponent>(entity);

				Renderer2D::DrawQuad(transformComp.GetTransform(), spriteComp.Color);
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(Timestep deltatime, EditorCamera& editorCamera)
	{
		//Renderer2D::BeginScene(editorCamera);

		auto group = m_Registry.group<SpriteRendererComponent>(entt::get<IDComponent, TransformComponent>);
		for (auto entity : group)
		{
			auto [spriteComp, uuidComp, transformComp] = group.get<SpriteRendererComponent, IDComponent, TransformComponent>(entity);
			
			Renderer2D::DrawSprite(transformComp.GetTransform(), spriteComp, uuidComp.ID);
		}
		//Renderer2D::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);

			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
			{
				return Entity{ entity, this};
			}
		}
		return {};
	}


// OnComponentAdded
	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{

	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{

	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{

	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if(m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{

	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{

	}
}
