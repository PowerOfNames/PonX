#pragma once
#include "Povox/Core/Core.h"

#include <glm/glm.hpp>
#include <vector>

namespace Povox {

	class Shader;
	class Texture;
	struct SimpleMaterial
	{
		glm::vec3 Color{ 1.0f, 1.0f, 1.0f };

		Ref<Shader> Shader = nullptr;
		Ref<Texture> Texture = nullptr;
	};

	struct VertexData
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		float TexID;
		float TilingFactor;

		//Editor only
		int EntityID;
	};

	class Buffer;
	struct Mesh
	{
		Ref<Buffer> VertexBuffer;
		Ref<Buffer> IndexBuffer;
	};

	struct Renderable
	{
		Mesh MeshData;

		SimpleMaterial Material;
	};

}
