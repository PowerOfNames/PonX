#include "pxpch.h"
#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	//TODO: Make this swappable!
	RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;

}