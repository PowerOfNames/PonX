#include "pxpch.h"
#include "VulkanRenderPass.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanImage2D.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
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

	void VulkanRenderPass::BindInput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_Specification.DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Inputs.push_back(name);
	}

	void VulkanRenderPass::BindOutput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_Specification.DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Outputs.push_back(name);
	}	

	void VulkanRenderPass::UpdateDescriptor(const std::string& name)
	{
		PX_CORE_ASSERT(m_AllShaderResourceDescs.find(name) != m_AllShaderResourceDescs.end(), "Descriptor not contained in Renderpass");
		PX_CORE_ASSERT(m_InvalidResources.find(name) != m_InvalidResources.end(), "Descriptor not contained in Invalid Resources!");

		Ref<ShaderResourceDescription> description = m_AllShaderResourceDescs[name];
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;


		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.pNext = nullptr;

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
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.dstBinding = description->Binding;

				for (uint32_t frame = 0; frame < framesInFlight; frame++)
				{
					write.pImageInfo = &((std::dynamic_pointer_cast<VulkanImage2D>(std::dynamic_pointer_cast<StorageImage>(m_BoundResources.at(name))->GetImage(frame)))->GetImageInfo());
					write.dstSet = m_DescriptorSets.at(description->Set).Sets[frame];
				}


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
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.dstBinding = description->Binding;

				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<UniformBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo());
					write.dstSet = sets.Sets[frame];
				}

				break;
			}
			case ShaderResourceType::STORAGE_BUFFER:
			{
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				write.dstBinding = description->Binding;

				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo());
					write.dstSet = sets.Sets[frame];
				}

				break;
			}
			case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
			{

				break;
			}
			case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
			{
				Ref<StorageBufferDynamic> resource = std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(name));
				auto& descriptorInfo = resource->GetDescriptorInfo(name);

				if (!descriptorInfo.Suballocation)
				{
					PX_CORE_ERROR("Resource {} still not ready to use!");
					break;
				}
					
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				write.dstBinding = description->Binding;
								
				write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(resource->GetBuffer(name)))
					->GetBufferInfo(0, descriptorInfo.Suballocation->Range));
				
				//TODO: remove framed
				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.dstSet = sets.Sets[frame];
				}
				m_InvalidResources.erase(name);
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

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanRenderPass::Bake()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;

		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;
		writes.reserve(m_AllShaderResourceDescs.size() * framesInFlight);
		bufferInfos.reserve(m_AllShaderResourceDescs.size() * framesInFlight);
		imageInfos.reserve(m_AllShaderResourceDescs.size() * framesInFlight);


		for (const auto [name, description] : m_AllShaderResourceDescs)
		{
			ShaderResourceType type = description->ResourceType;


			switch (type)
			{
				case ShaderResourceType::NONE:
				{

				}
				case ShaderResourceType::SAMPLER:
				{

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
					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					write.dstBinding = description->Binding;

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorImageInfo tempInfo = (std::dynamic_pointer_cast<VulkanImage2D>(std::dynamic_pointer_cast<StorageImage>(m_BoundResources.at(name))->GetImage(frame)))->GetImageInfo();
						imageInfos.push_back(std::move(tempInfo));
						write.pImageInfo = &(tempInfo);

						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}


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
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.dstBinding = description->Binding;

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorBufferInfo& info = bufferInfos.emplace_back((std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<UniformBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo());
						write.pBufferInfo = &info;

						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
				{
					m_DynamicDescriptors[description->Set][description->Binding] = name;

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER:
				{

					Ref<ShaderResource> resource = m_BoundResources.at(name);
					if (resource == nullptr)
						break;

					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = description->Binding;

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorBufferInfo tempInfo = (std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(resource)->GetBuffer(frame)))->GetBufferInfo();
						bufferInfos.push_back(std::move(tempInfo));
						write.pBufferInfo = &(tempInfo);

						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
				{
					m_DynamicDescriptors[description->Set][description->Binding] = name;

					auto& descInfo = (std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(name)))->GetDescriptorInfo(name);
					if (!descInfo.Suballocation)
					{
						m_InvalidResources[name] = m_BoundResources.at(name);
						break;
					}
					Ref<BufferSuballocation> suballocation = descInfo.Suballocation;
					Ref<VulkanBuffer> buffer = std::dynamic_pointer_cast<VulkanBuffer>(suballocation->Buffer);

					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					write.dstBinding = description->Binding;

					VkDescriptorBufferInfo tempInfo = buffer->GetBufferInfo(0, suballocation->Range);
					bufferInfos.push_back(std::move(tempInfo));
					write.pBufferInfo = &(tempInfo);

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

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

	std::vector<uint32_t> VulkanRenderPass::GetDynamicOffsets(uint32_t currentFrameIndex)
	{
		if (m_DynamicDescriptors.empty())
		{
			PX_CORE_WARN("There are no dynamic descriptrs registered to this renderpass!");
			return {};
		}

		std::vector<uint32_t> offsets;
		for (auto& [set, bindings] : m_DynamicDescriptors)
		{
			for (auto& [binding, name] : bindings)
			{
				Ref<ShaderResource> resource = m_BoundResources.at(name);
				switch (resource->GetType())
				{
					case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
					{
						//auto& descOffsets = std::dynamic_pointer_cast<UniformBufferDynamic>(resource)->GetOffsets(currentFrameIndex);
						//std::copy(descOffsets.begin(), descOffsets.end(), std::back_inserter(offsets));
						break;
					}
					case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
					{
						uint32_t offset = std::dynamic_pointer_cast<StorageBufferDynamic>(resource)->GetOffset(name, currentFrameIndex);
						offsets.push_back(offset);
						break;
					}
					default:
					{
						PX_CORE_ERROR("An offset request to a non-dynamic shader resource has been made!");
						break;
					}
				}
				PX_CORE_TRACE("Set: {}, Binding {} Name {}", set, binding, name);
			}
		}
		return offsets;
	}

	void VulkanRenderPass::CreateDescriptorSets()
	{
		const std::unordered_map<uint32_t, std::string> setNames = {
			{0, "GlobalUBOs"},
			{1, "GlobalSSBOs"},
			{2, "Textures"},
			{3, "ObjectMaterials"}
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
			set.Bindings = 0; // TODO: if needed
			set.Layout = layout;
			if (setNumber != 1)
				set.Sets.resize(framesInFlight);
			else
				set.Sets.resize(1);

			for (uint32_t frame = 0; frame < set.Sets.size(); frame++)
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
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(m_Specification.Pipeline, "There was no Pipeline attached to this RenderPass!");
		/*if (!m_Specification.TargetFramebuffer)
		{
			m_Specification.TargetFramebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		}
		m_RenderPass = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)->GetRenderPass();*/


		auto& shaderResources = m_Specification.Pipeline->GetResourceDescriptions();
		m_AllShaderResourceDescs.insert(shaderResources.begin(), shaderResources.end());

		CreateDescriptorSets();
	}
	VulkanComputePass::~VulkanComputePass()
	{
	}

	void VulkanComputePass::Recreate()
	{
	}
	

	void VulkanComputePass::BindInput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_Specification.DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Inputs.push_back(name);
	}

	void VulkanComputePass::BindOutput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_Specification.DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Outputs.push_back(name);
	}

	void VulkanComputePass::UpdateDescriptor(const std::string& name)
	{
		PX_CORE_ASSERT(m_AllShaderResourceDescs.find(name) != m_AllShaderResourceDescs.end(), "Descriptor not contained in Renderpass");
		PX_CORE_ASSERT(m_InvalidResources.find(name) != m_InvalidResources.end(), "Descriptor not contained in Invalid Resources!");


		Ref<ShaderResourceDescription> description = m_AllShaderResourceDescs[name];
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;
		

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.pNext = nullptr;

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
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.dstBinding = description->Binding;

				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.pImageInfo = &((std::dynamic_pointer_cast<VulkanImage2D>(std::dynamic_pointer_cast<StorageImage>(m_BoundResources.at(name))->GetImage(frame)))->GetImageInfo());
					write.dstSet = sets.Sets[frame];
				}


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
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.dstBinding = description->Binding;

				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<UniformBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo());
					write.dstSet = sets.Sets[frame];
				}

				break;
			}
			case ShaderResourceType::STORAGE_BUFFER:
			{
				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				write.dstBinding = description->Binding;

				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo());
					write.dstSet = sets.Sets[frame];
				}

				break;
			}
			case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
			{

				break;
			}
			case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
			{
				Ref<StorageBufferDynamic> resource = std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(name));
				auto& descriptorInfo = resource->GetDescriptorInfo(name);

				if (!descriptorInfo.Suballocation)
				{
					PX_CORE_ERROR("Resource {} still not ready to use!");
					break;
				}

				write.descriptorCount = description->Count;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				write.dstBinding = description->Binding;

				write.pBufferInfo = &((std::dynamic_pointer_cast<VulkanBuffer>(resource->GetBuffer(name)))
					->GetBufferInfo(0, descriptorInfo.Suballocation->Range));

				//TODO: remove framed
				auto& sets = m_DescriptorSets.at(description->Set);
				for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
				{
					write.dstSet = sets.Sets[frame];
				}
				m_InvalidResources.erase(name);
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

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanComputePass::Bake()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;
		
		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;
		writes.reserve(m_AllShaderResourceDescs.size() * framesInFlight);
		bufferInfos.reserve(m_AllShaderResourceDescs.size() * framesInFlight);
		imageInfos.reserve(m_AllShaderResourceDescs.size() * framesInFlight);


		for (const auto [name, description] : m_AllShaderResourceDescs)
		{
			ShaderResourceType type = description->ResourceType;
			

			switch (type)
			{
				case ShaderResourceType::NONE:
				{

				}
				case ShaderResourceType::SAMPLER:
				{
					
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
					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					write.dstBinding = description->Binding;
					
					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorImageInfo tempInfo = (std::dynamic_pointer_cast<VulkanImage2D>(std::dynamic_pointer_cast<StorageImage>(m_BoundResources.at(name))->GetImage(frame)))->GetImageInfo();
						imageInfos.push_back(std::move(tempInfo));
						write.pImageInfo = &(tempInfo);
						
						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}


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
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.dstBinding = description->Binding;

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorBufferInfo temp = (std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<UniformBuffer>(m_BoundResources.at(name))->GetBuffer(frame)))->GetBufferInfo();
						VkDescriptorBufferInfo& info = bufferInfos.emplace_back(temp);
						write.pBufferInfo = &info;

						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
				{
					

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER:
				{

					Ref<ShaderResource> resource = m_BoundResources.at(name);
					if (resource == nullptr)
						break;

					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = description->Binding;

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{
						VkDescriptorBufferInfo tempInfo = (std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(resource)->GetBuffer(frame)))->GetBufferInfo();
						bufferInfos.push_back(std::move(tempInfo));
						write.pBufferInfo = &(tempInfo);

						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

					break;
				}
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
				{
					m_DynamicDescriptors[description->Set][description->Binding] = name;

					auto& descInfo = (std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(name)))->GetDescriptorInfo(name);
					if (!descInfo.Suballocation)
					{
						m_InvalidResources[name] = m_BoundResources.at(name);
						break;
					}
					Ref<BufferSuballocation> suballocation = descInfo.Suballocation;
					Ref<VulkanBuffer> buffer = std::dynamic_pointer_cast<VulkanBuffer>(suballocation->Buffer);

					VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.pNext = nullptr;
					write.descriptorCount = description->Count;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					write.dstBinding = description->Binding;

					VkDescriptorBufferInfo tempInfo = buffer->GetBufferInfo(0, suballocation->Range);
					bufferInfos.push_back(std::move(tempInfo));
					write.pBufferInfo = &(tempInfo);

					auto& sets = m_DescriptorSets.at(description->Set);
					for (uint32_t frame = 0; frame < sets.Sets.size(); frame++)
					{						
						write.dstSet = sets.Sets[frame];
						writes.push_back(write);
					}

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

// 	const std::unordered_map<uint32_t, DescriptorSet>& VulkanComputePass::GetDescriptorSets()
// 	{
// 		Validate();
// 
// 
// 	}

	std::vector<uint32_t> VulkanComputePass::GetDynamicOffsets(uint32_t currentFrameIndex) 
	{
		if (m_DynamicDescriptors.empty())
		{
			PX_CORE_WARN("There are no dynamic descriptrs registered to this renderpass!");
			return {};
		}

		std::vector<uint32_t> offsets;
		for (auto& [set, bindings] : m_DynamicDescriptors)
		{
			for (auto& [binding, name] : bindings)
			{
				Ref<ShaderResource> resource = m_BoundResources.at(name);
				switch (resource->GetType())
				{
					case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
					{
						//auto& descOffsets = std::dynamic_pointer_cast<UniformBufferDynamic>(resource)->GetOffsets(currentFrameIndex);
						//std::copy(descOffsets.begin(), descOffsets.end(), std::back_inserter(offsets));
						break;
					}
					case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
					{
						uint32_t offset = std::dynamic_pointer_cast<StorageBufferDynamic>(resource)->GetOffset(name, currentFrameIndex);
						offsets.push_back(offset);
						break;
					}
					default:
					{
						PX_CORE_ERROR("An offset request to a non-dynamic shader resource has been made!");
						break;
					}
				}
				PX_CORE_TRACE("Set: {}, Binding {} Name {}", set, binding, name);
			}
		}
		return offsets;
	}

	void VulkanComputePass::CreateDescriptorSets()
	{
		const std::unordered_map<uint32_t, std::string> setNames = {
			{0, "GlobalUBOs"},
			{1, "GlobalSSBOs"},
			{2, "Textures"},
			{3, "ObjectMaterials"}
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
			if (setNumber != 1)
				set.Sets.resize(framesInFlight);
			else
				set.Sets.resize(1);
		
			for (uint32_t frame = 0; frame < set.Sets.size(); frame++)
			{
				builder.BuildNoWrites(set.Sets[frame], layout, setNames.at(setNumber));
			}
			m_DescriptorSets[setNumber] = std::move(set);
		}
	}

	void VulkanComputePass::Validate()
	{

	}

}
