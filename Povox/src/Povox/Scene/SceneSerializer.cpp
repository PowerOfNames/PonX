#include "pxpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}

	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.a);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.a = node[3].as<float>();
			return true;
		}

	};
}
namespace Povox {
		

	YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec3& values)
	{
		emitter << YAML::Flow;
		emitter << YAML::BeginSeq << values.x << values.y << values.z << YAML::EndSeq;
		return emitter;
	}

	YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec4& values)
	{
		emitter << YAML::Flow;
		emitter << YAML::BeginSeq << values.x << values.y << values.z << values.a << YAML::EndSeq;
		return emitter;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}


	template<typename T, typename UIFunction>
	void SerializeComponent(YAML::Emitter& out, const std::string& name, Entity entity, UIFunction uiFunction)
	{

		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			out << YAML::Key << name;
			out << YAML::BeginMap; // Componentname

			uiFunction(component);

			out << YAML::EndMap;
		}
	}

	template<typename T, typename UIFunction>
	void DeserializeComponent(const YAML::Node& entityNode, const std::string& name, Entity entity, UIFunction uiFunction)
	{
		auto componentNode = entityNode[name];
		if (componentNode)
		{
			auto& component = entity.AddComponent<T>();
			uiFunction(componentNode, component);
		}
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << 63725789201658; // TODO: entity ID goes here

		// TODO: later implement some sort of global component list to iterate through 
		SerializeComponent<TagComponent>(out, "TagComponent", entity, [&](auto& tagComp)
		{
			out << YAML::Key << "Tag" << YAML::Value << tagComp.Tag;
		});
		SerializeComponent<TransformComponent>(out, "TransformComponent", entity, [&](auto& transfComp)
		{
			out << YAML::Key << "Translation" << YAML::Value << transfComp.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transfComp.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transfComp.Scale;
		});
		SerializeComponent<CameraComponent>(out, "CameraComponent", entity, [&](auto& camComp)
		{
			auto& camera = camComp.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "Projection Type" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "Perspective Vertical FOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "Perspective Near Clip" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "Perspective Far Clip" << YAML::Value << camera.GetPerspectiveFarClip();

			out << YAML::Key << "Orthographic Size" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "Orthographic Near Clip" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "Orthographic Far Clip" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera					   

			out << YAML::Key << "Primary" << YAML::Value << camComp.Primary;
			out << YAML::Key << "Fixed Aspect Ratio" << YAML::Value << camComp.FixedAspectRatio;
		});
		SerializeComponent<SpriteRendererComponent>(out, "SpriteRendererComponent", entity, [&](auto& spriteRenComp)
		{
			out << YAML::Key << "Color" << YAML::Value << spriteRenComp.Color;
		});

		out << YAML::EndMap; // Entity
	}

	static void DeserializeEntity(YAML::Node& entityNode, Entity deserializedEntity)
	{
		auto transformComponent = entityNode["TransformComponent"];
		if (transformComponent)
		{
			// Entities always have transforms
			auto& tc = deserializedEntity.GetComponent<TransformComponent>();
			tc.Translation = transformComponent["Translation"].as<glm::vec3>();
			tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
			tc.Scale = transformComponent["Scale"].as<glm::vec3>();
		}
		DeserializeComponent<CameraComponent>(entityNode, "CameraComponent", deserializedEntity, [&](auto& componentNode, auto& cc)
			{
				auto& cameraProps = componentNode["Camera"];

				cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["Projection Type"].as<int>());

				cc.Camera.SetPerspectiveVerticalFOV(cameraProps["Perspective Vertical FOV"].as<float>());
				cc.Camera.SetPerspectiveNearClip(cameraProps["Perspective Near Clip"].as<float>());
				cc.Camera.SetPerspectiveFarClip(cameraProps["Perspective Far Clip"].as<float>());

				cc.Camera.SetOrthographicSize(cameraProps["Orthographic Size"].as<float>());
				cc.Camera.SetOrthographicNearClip(cameraProps["Orthographic Near Clip"].as<float>());
				cc.Camera.SetOrthographicFarClip(cameraProps["Orthographic Far Clip"].as<float>());

				cc.Primary = componentNode["Primary"].as<bool>();
				cc.FixedAspectRatio = componentNode["Fixed Aspect Ratio"].as<bool>();
			});
		DeserializeComponent<SpriteRendererComponent>(entityNode, "SpriteRendererComponent", deserializedEntity, [&](auto& componentNode, auto& src)
			{
				src.Color = componentNode["Color"].as<glm::vec4>();
			});
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Noname";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Scene.get() };

			if (!entity)
				return;

			SerializeEntity(out, entity);
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream buffer;
		PX_CORE_ASSERT(stream, "SceneSerializer::Deserialize - empty file");
		buffer << stream.rdbuf();
		stream.close();

		YAML::Node data = YAML::Load(buffer.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		PX_CORE_TRACE("Deserializing Scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entityNode : entities)
			{
				uint64_t uuid = entityNode["Entity"].as<uint64_t>(); // TODO: universal unique identifier;

				std::string name;
				auto tagComponent = entityNode["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				PX_CORE_TRACE("Deserializing entity with ID '{0}' and name '{1}'", uuid, name);


				Entity deserializedEntity = m_Scene->CreateEntity(name);
				DeserializeEntity(entityNode, deserializedEntity);
			}
		}


		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// not yet implemented
		PX_CORE_ASSERT(false, "Deserializer no yet implemented");
		return false;
	}

}