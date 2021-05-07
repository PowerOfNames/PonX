#pragma once
#include "Scene.h"


namespace Povox {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);
		~SceneSerializer() = default;

		void Serialize(const std::string& filepath);		//to yaml text
		void SerializeRuntime(const std::string& filepath);	//to binary

		bool Deserialize(const std::string& filepath);		//to yaml text
		bool DeserializeRuntime(const std::string& filepath);	//to binary


	private:
		Ref<Scene> m_Scene;
	};

}