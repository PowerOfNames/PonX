#include "pxpch.h"
#include "Platform/Vulkan/VulkanShaderResourceSystem.h"

#include "Povox/Core/Application.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

namespace Povox {




	VulkanShaderResourceSystem::VulkanShaderResourceSystem()
	{
		Init();
	}

	VulkanShaderResourceSystem::~VulkanShaderResourceSystem()
	{

	}

	bool VulkanShaderResourceSystem::Init()
	{
		m_Device = VulkanContext::GetDevice();


		m_DescriptorLayoutCache = CreateRef<VulkanDescriptorLayoutCache>();
		PX_CORE_ASSERT(m_DescriptorLayoutCache, "Failed to create DescriptorLayoutCache!");

		uint32_t maxFrames = Application::Get()->GetSpecification().MaxFramesInFlight;


		return true;
	}

	void VulkanShaderResourceSystem::OnUpdate()
	{

	}

	void VulkanShaderResourceSystem::Shutdown()
	{

	}

}