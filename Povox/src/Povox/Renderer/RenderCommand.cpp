#include "pxpch.h"
#include "Povox/Renderer/RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Povox {

	// TODO: should be propagated to the right context upon startup
	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;
}