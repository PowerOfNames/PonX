#include "pxpch.h"
#include "VulkanRenderPass.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanImage2D.h"
#include "Platform/Vulkan/VulkanPipeline.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Log.h"

namespace Povox {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		PX_PROFILE_FUNCTION();
		

		PX_CORE_ASSERT(m_Specification.Pipeline, "There was no Pipeline attached to this RenderPass!");
		if (!m_Specification.TargetFramebuffer)
		{
			m_Specification.TargetFramebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		}
		m_RenderPass = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)->GetRenderPass();
		

		auto& shaderResources = m_Specification.Pipeline->GetResourceDescriptions();
		m_AllShaderResourceDescs.insert(shaderResources.begin(), shaderResources.end());

		CreateDescriptorSets();
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}

	void VulkanRenderPass::Recreate(uint32_t width, uint32_t height)
	{
		if (m_RenderPass)
			m_RenderPass = VK_NULL_HANDLE;

		m_Specification.TargetFramebuffer->Recreate(width, height);
		
		m_Specification.Pipeline->Recreate();
			
		m_RenderPass = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)->GetRenderPass();

		PX_CORE_INFO("VulkanRenderpass::Recreate: Recreated Renderpass with AttachmentExtent of '{0}, {1}'", width, height);		
	}

	void VulkanRenderPass::BindResource(const std::string& name, Ref<UniformBuffer> resource)
	{
		m_UniformBuffers[name] = resource;
	}
	void VulkanRenderPass::BindResource(const std::string& name, Ref<StorageBuffer> resource)
	{
		m_StorageBuffers[name] = resource;
	}

	void VulkanRenderPass::Bake()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(m_AllShaderResourceDescs.size());

		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;

		for (const auto& [name, description] : m_AllShaderResourceDescs)
		{
			ShaderResourceType type = description->ResourceType;

			switch (type)
			{
				case ShaderResourceType::NONE:
				{

					break;
				}
				case ShaderResourceType::SAMPLER:
				{

					break;
				}
				case ShaderResourceType::COMBINED_IMAGE_SAMPLER:
				{

					break;
				}
				case ShaderResourceType::SAMPLED_IMAGE:
				{

					break;
				}
				case ShaderResourceType::STORAGE_IMAGE:
				{

					break;
				}
				case ShaderResourceType::UNIFORM_TEXEL_BUFFER:
				{

					break;
				}
				case ShaderResourceType::STORAGE_TEXEL_BUFFER:
				{

					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER:
				{
					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = 1; // for arrays
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.dstBinding = description->Binding;
					PX_CORE_TRACE("Baking: ShaderResourceType::Uniform_Buffer Named {}", name);
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						write.pBufferInfo = &(std::dynamic_pointer_cast<VulkanBuffer>(m_UniformBuffers.at(name)->GetBuffer(frame)))->GetBufferInfo();
						write.dstSet = m_DescriptorSets.at(description->Set).Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER:
				{
					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = 1;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = description->Binding;
					PX_CORE_TRACE("Baking: ShaderResourceType::Storage_Buffer Named {}", name);

					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						write.pBufferInfo = &(std::dynamic_pointer_cast<VulkanBuffer>(m_StorageBuffers.at(name)->GetBuffer(frame)))->GetBufferInfo();
						write.dstSet = m_DescriptorSets.at(description->Set).Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
				{

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
				{

					break;
				}
				case ShaderResourceType::INPUT_ATTACHMENT:
				{

					break;
				}
				case ShaderResourceType::ACCELERATION_STRUCTURE:
				{

					break;
				}
				case ShaderResourceType::PUSH_CONSTANT:
				{

					break;
				}
				default:
				{

					break;
				}
			}
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	void VulkanRenderPass::CreateDescriptorSets()
	{
		const std::unordered_map<uint32_t, std::string> setNames = {
			{0, "GlobalUBOs"},
			{1, "GlobalSSBOs"},
			{2, "ObjectMaterials"},
			{3, "Textures"}
		};

		auto& layoutCache = VulkanContext::GetDescriptorLayoutCache();
		VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());
		auto& layoutMap = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Pipeline->GetSpecification().Shader)->GetDescriptorSetLayouts();
		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;
		
		// Description:
		/*
		* For each unique set(layout) create a descriptorSet vector (per FrameInFlight) and add it to this renderpass.
		*/
		
		for (const auto& [setNumber, layout] : layoutMap)
		{
			DescriptorSet set{};
			// TODO: Safety check if number is in set
			set.Name = setNames.at(setNumber);
			set.SetNumber = setNumber;
			set.Bindings = 0; // TODO:
			set.Layout = layout;
			set.Sets.resize(framesInFlight);
			for (uint32_t frame = 0; frame < framesInFlight; frame++)
			{
				builder.BuildNoWrites(set.Sets[frame], layout, setNames.at(setNumber));
			}
			m_DescriptorSets[setNumber] = std::move(set);
		}
	}


	// Compute

	VulkanComputePass::VulkanComputePass(const ComputePassSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanComputePass::~VulkanComputePass()
	{
	}

	void VulkanComputePass::Recreate()
	{
	}

	

}
