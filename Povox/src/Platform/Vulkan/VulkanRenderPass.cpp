#include "pxpch.h"
#include "VulkanRenderPass.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanCommands.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanImage2D.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanShader.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Log.h"

namespace Povox {

	VulkanPass::VulkanPass()
	{

	}

	void VulkanPass::CreateDescriptorSets(const std::map<uint32_t, VkDescriptorSetLayout>& layoutMap)
	{
		const std::unordered_map<uint32_t, std::string> setNames = {
			{0, "GlobalUBOs"},
			{1, "GlobalSSBOs"},
			{2, "Textures"},
			{3, "ObjectMaterials"}
		};

		auto& layoutCache = VulkanContext::GetDescriptorLayoutCache();
		VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());
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

	void VulkanPass::BindInput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Inputs.push_back(name);
	}

	void VulkanPass::BindOutput(const std::string& name, Ref<ShaderResource> resource)
	{
		if (m_BoundResources.find(name) != m_BoundResources.end())
		{
			PX_CORE_WARN("Renderpass {}, ShaderResource {} already bound to map!", m_DebugName, name);
			return;
		}

		m_BoundResources[name] = resource;
		m_Outputs.push_back(name);
	}

	void VulkanPass::Bake()
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

			VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.pNext = nullptr;
			write.descriptorCount = description->Count;
			write.dstBinding = description->Binding;

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
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

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
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

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
										
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

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
										
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

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

	void VulkanPass::Validate()
	{

	}

	void VulkanPass::UpdateDescriptor(const std::string& name)
	{
		PX_CORE_ASSERT(m_AllShaderResourceDescs.find(name) != m_AllShaderResourceDescs.end(), "Descriptor not contained in Renderpass");
		PX_CORE_ASSERT(m_InvalidResources.find(name) != m_InvalidResources.end(), "Descriptor not contained in Invalid Resources!");

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		uint32_t framesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;


		Ref<ShaderResourceDescription> description = m_AllShaderResourceDescs[name];

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.pNext = nullptr;
		write.descriptorCount = description->Count;
		write.dstBinding = description->Binding;

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
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

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
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

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
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

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

				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

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

	std::vector<uint32_t> VulkanPass::GetDynamicOffsets(uint32_t currentFrameIndex)
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
			}
		}
		return offsets;
	}


	const std::vector<std::string> VulkanPass::GetBoundResourceNames()
	{
		std::vector<std::string> descriptorNames;
		descriptorNames.reserve(m_BoundResources.size());
		for (const auto& [name, descr] : m_BoundResources)
		{
			descriptorNames.push_back(name);
		}
		return descriptorNames;
	}

// RenderPass

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		PX_PROFILE_FUNCTION();
		
		m_DebugName = spec.DebugName;
		m_Type = PassType::GRAPHICS;

		PX_CORE_ASSERT(m_Specification.Pipeline, "There was no Pipeline attached to this RenderPass!");
		if (!m_Specification.TargetFramebuffer)
		{
			m_Specification.TargetFramebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		}
		m_RenderPass = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)->GetRenderPass();
		

		auto& shaderResources = m_Specification.Pipeline->GetResourceDescriptions();
		m_AllShaderResourceDescs.insert(shaderResources.begin(), shaderResources.end());

		auto& layoutMap = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Pipeline->GetSpecification().Shader)->GetDescriptorSetLayouts();
		CreateDescriptorSets(layoutMap);
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

	void VulkanRenderPass::UpdateResourceOwnership(uint32_t frameIndex)
	{
		if (!m_PredecessorPass)
		{
			PX_CORE_INFO("VulkanRenderPass::UpdateResourceOwnership: This Graphics pass has no previous compute. No ownership transfer needed!");
			return;
		}

		if (m_PredecessorPass->GetPassType() == PassType::GRAPHICS)
			return;

		Ref<VulkanComputePass> computePass = std::dynamic_pointer_cast<VulkanComputePass>(m_PredecessorPass);

		uint32_t graphicsQueue = VulkanContext::GetDevice()->GetQueueFamilies().GraphicsFamilyIndex;
		uint32_t computeQueue = VulkanContext::GetDevice()->GetQueueFamilies().ComputeFamilyIndex;

		//Now find all resources that are used in the previous compute and this graphics pass. Ownership from Compute to Graphics queue

		std::vector<std::string> prevDescriptorNames = computePass->GetBoundResourceNames();
		
		VkBuffer buffer = nullptr;
		size_t offset = 0;
		size_t range = 0;
		for (const std::string& prevName : prevDescriptorNames)
		{
			if (m_BoundResources.find(prevName) == m_BoundResources.end())
				continue;
			Ref<ShaderResource> sharedResource = m_BoundResources.at(prevName);
			
			switch (sharedResource->GetType())
			{
				case ShaderResourceType::UNIFORM_BUFFER:
				{
					break;
				}
				case ShaderResourceType::STORAGE_BUFFER:
				{	
					//Ref<VulkanBuffer> vkBuffer = std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(m_BoundResources.at(prevName))->GetBuffer(frameIndex));
					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
				{
					break;
				}
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
				{
					Ref<StorageBufferDynamic> storageBuf = std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(prevName));
					auto& descriptorInfo = storageBuf->GetDescriptorInfo(prevName);

					if (!descriptorInfo.Suballocation)
					{
						PX_CORE_WARN("This Dynamic storage buffer does not contain any suballocations");
						break;
					}

					buffer = std::dynamic_pointer_cast<VulkanBuffer>(descriptorInfo.Suballocation->Buffer)->GetAllocation().Buffer;
					offset = descriptorInfo.Suballocation->Offset;
					range = descriptorInfo.Suballocation->Range;

					VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_GRAPHICS
						, [=](VkCommandBuffer releaseCmd)
						{
							VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
							bufBarrier.pNext = nullptr;
							bufBarrier.srcQueueFamilyIndex = computeQueue;
							bufBarrier.dstQueueFamilyIndex = graphicsQueue;

							bufBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
							bufBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

							bufBarrier.buffer = buffer;
							bufBarrier.offset = offset;
							bufBarrier.size = range;

							VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
							dependency.pNext = nullptr;
							dependency.bufferMemoryBarrierCount = 1;
							dependency.pBufferMemoryBarriers = &bufBarrier;
							vkCmdPipelineBarrier2(releaseCmd, &dependency);
						}
						, [=](VkCommandBuffer acquireCmd)
							{
								VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
								bufBarrier.pNext = nullptr;
								bufBarrier.srcQueueFamilyIndex = computeQueue;
								bufBarrier.dstQueueFamilyIndex = graphicsQueue;

								bufBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
								bufBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;

								bufBarrier.buffer = buffer;
								bufBarrier.offset = offset;
								bufBarrier.size = range;

								VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
								dependency.pNext = nullptr;
								dependency.bufferMemoryBarrierCount = 1;
								dependency.pBufferMemoryBarriers = &bufBarrier;
								vkCmdPipelineBarrier2(acquireCmd, &dependency);
							});
					break;
				}
			}
		}			
	}



// Compute

	VulkanComputePass::VulkanComputePass(const ComputePassSpecification& spec)
		: m_Specification(spec)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(m_Specification.Pipeline, "There was no Pipeline attached to this RenderPass!");

		m_DebugName = spec.DebugName;
		m_Type = PassType::COMPUTE;

		auto& shaderResources = m_Specification.Pipeline->GetResourceDescriptions();
		m_AllShaderResourceDescs.insert(shaderResources.begin(), shaderResources.end());

		auto& layoutMap = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Pipeline->GetSpecification().Shader)->GetDescriptorSetLayouts();
		CreateDescriptorSets(layoutMap);
	}
	VulkanComputePass::~VulkanComputePass()
	{
	}
	
	void VulkanComputePass::Recreate(uint32_t width, uint32_t height)
	{

	}

	void VulkanComputePass::UpdateResourceOwnership(uint32_t frameIndex)
	{
		if (!m_PredecessorPass)
		{
			PX_CORE_INFO("VulkanRenderPass::UpdateResourceOwnership: This Graphics pass has no previous compute. No ownership transfer needed!");
			return;
		}

		if (m_PredecessorPass->GetPassType() == PassType::COMPUTE)
			return;

		Ref<VulkanRenderPass> graphicsPass = std::dynamic_pointer_cast<VulkanRenderPass>(m_PredecessorPass);

		uint32_t graphicsQueue = VulkanContext::GetDevice()->GetQueueFamilies().GraphicsFamilyIndex;
		uint32_t computeQueue = VulkanContext::GetDevice()->GetQueueFamilies().ComputeFamilyIndex;

		//Now find all resources that are used in the previous compute and this graphics pass. Ownership from Compute to Graphics queue

		std::vector<std::string> prevDescriptorNames = graphicsPass->GetBoundResourceNames();

		VkBuffer buffer = nullptr;
		size_t offset = 0;
		size_t range = 0;
		for (const std::string& prevName : prevDescriptorNames)
		{
			if (m_BoundResources.find(prevName) == m_BoundResources.end())
				continue;
			Ref<ShaderResource> sharedResource = m_BoundResources.at(prevName);

			switch (sharedResource->GetType())
			{
				case ShaderResourceType::UNIFORM_BUFFER:
				{
					break;
				}
				case ShaderResourceType::STORAGE_BUFFER:
				{
					//Ref<VulkanBuffer> vkBuffer = std::dynamic_pointer_cast<VulkanBuffer>(std::dynamic_pointer_cast<StorageBuffer>(m_BoundResources.at(prevName))->GetBuffer(frameIndex));
					break;
				}
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC:
				{
					break;
				}
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC:
				{
					Ref<StorageBufferDynamic> storageBuf = std::dynamic_pointer_cast<StorageBufferDynamic>(m_BoundResources.at(prevName));
					auto& descriptorInfo = storageBuf->GetDescriptorInfo(prevName);

					if (!descriptorInfo.Suballocation)
					{
						PX_CORE_WARN("This Dynamic storage buffer does not contain any suballocations");
						break;
					}

					buffer = std::dynamic_pointer_cast<VulkanBuffer>(descriptorInfo.Suballocation->Buffer)->GetAllocation().Buffer;
					offset = descriptorInfo.Suballocation->Offset;
					range = descriptorInfo.Suballocation->Range;

					VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_COMPUTE
						, [=](VkCommandBuffer releaseCmd)
						{
							VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
							bufBarrier.pNext = nullptr;
							bufBarrier.srcQueueFamilyIndex = graphicsQueue;
							bufBarrier.dstQueueFamilyIndex = computeQueue;

							bufBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
							bufBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

							bufBarrier.buffer = buffer;
							bufBarrier.offset = offset;
							bufBarrier.size = range;

							VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
							dependency.pNext = nullptr;
							dependency.bufferMemoryBarrierCount = 1;
							dependency.pBufferMemoryBarriers = &bufBarrier;
							vkCmdPipelineBarrier2(releaseCmd, &dependency);
						}
						, [=](VkCommandBuffer acquireCmd)
							{
								VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
								bufBarrier.pNext = nullptr;
								bufBarrier.srcQueueFamilyIndex = graphicsQueue;
								bufBarrier.dstQueueFamilyIndex = computeQueue;

								bufBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
								bufBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

								bufBarrier.buffer = buffer;
								bufBarrier.offset = offset;
								bufBarrier.size = range;

								VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
								dependency.pNext = nullptr;
								dependency.bufferMemoryBarrierCount = 1;
								dependency.pBufferMemoryBarriers = &bufBarrier;
								vkCmdPipelineBarrier2(acquireCmd, &dependency);
							});
					break;
				}
			}
		}
	}

}
