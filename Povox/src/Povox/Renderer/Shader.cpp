#include "pxpch.h"
#include "Povox/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Povox/Renderer/Renderer.h"


namespace Povox {

	Shader* Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::OpenGL:
			{
				return new OpenGLShader(vertexSrc, fragmentSrc);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}