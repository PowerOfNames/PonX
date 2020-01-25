#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {



	void Renderer::BeginScene()
	{
	}

	void Renderer::EndScene()
	{
	}

	// TODO: later should submit every call into a render command queue
	void Renderer::Submit(const std::shared_ptr<VertexArray>& vertexArray)
	{
		RenderCommand::DrawIndexed(vertexArray);
	}

}