#pragma once

#include <glm/glm.hpp>
#include "Povox/Scene/SceneCamera.h"
#include "Povox/Scene/ScriptableEntity.h"

namespace Povox {

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::mat4 Transform = glm::mat4(1.0f);

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::mat4& transform) 
			: Transform(transform) {}

		operator glm::mat4& () { return Transform; }
		operator const glm::mat4& () const { return Transform; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color) 
			: Color(color) {}
	};

	struct CameraComponent
	{
		Povox::SceneCamera Camera;
		bool Primary = true; //the scene may be responsible to know which the active camera is instead of the cameracomponent itself
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);
		
		template <typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity * >(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) {delete nsc->Instance; nsc->Instance = nullptr; };			
		}
	};

	/*
	From old native script version without virtual functions -> as refeence to me on how to properly write lambda-templates
		std::function<void(ScriptableEntity*)> OnCreateFunction;
		std::function<void(ScriptableEntity*)> OnDestroyFunction;
		std::function<void(ScriptableEntity*, Timestep)> OnUpdateFunction;

		 OnCreateFunction = [](ScriptableEntity* instance) { ((T*)instance)->OnCreate(); };
		OnDestroyFunction = [](ScriptableEntity* instance) { ((T*)instance)->OnDestroy(); };
		OnUpdateFunction = [](ScriptableEntity* instance, Timestep deltatime) { ((T*)instance)->OnUpdate(deltatime); };
	*/

}