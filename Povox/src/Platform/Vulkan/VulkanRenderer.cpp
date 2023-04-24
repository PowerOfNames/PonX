#include "pxpch.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"

#include "Povox/Renderer/TextureSystem.h"

#include <vulkan/vulkan.h>


#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	Scope<VulkanImGui> VulkanRenderer::m_ImGui = nullptr;
	static constexpr uint32_t MAX_OBJECTS = 10000;

	VulkanRenderer::VulkanRenderer(const RendererSpecification& specs)
		:m_Specification(specs)
	{
		Init();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		PX_CORE_TRACE("Vulkan Renderer API delete!");
	}

	// Core
	void VulkanRenderer::Init()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VulkanRenderer::Init: Started initializing...");

		m_Device = VulkanContext::GetDevice()->GetVulkanDevice();
		PX_CORE_ASSERT(m_Device, "No VulkanDevice was set!");
		m_Swapchain = Application::Get()->GetWindow().GetSwapchain();
		PX_CORE_ASSERT(m_Device, "No VulkanSwapchain was set!");

		InitCommandControl();
		InitPerformanceQueryPools();
		InitFrameData();

		PX_CORE_INFO("Creating ShaderLibrary...");

		m_ShaderLibrary = CreateRef<ShaderLibrary>();
		

		PX_CORE_INFO("Completed ShaderLibrary creation.");


		PX_CORE_INFO("Creating TextureSystem...");

		m_TextureSystem = CreateRef<TextureSystem>();

		PX_CORE_INFO("Completed TextureSystem creation.");

		CreateSamplers();
		CreateDescriptors();

		m_ImGui = CreateScope<VulkanImGui>(Application::Get()->GetWindow().GetSwapchain()->GetImageFormat(), (uint8_t)m_Specification.MaxFramesInFlight);
		PX_CORE_TRACE("VulkanRenderer:: Created VKImGui!");
		

		
		m_Specification.State.IsInitialized = true;

		m_Statistics.State = &m_Specification.State;
		PX_CORE_INFO("VulkanRenderer::Init: Completed initialization.");
	}

	void VulkanRenderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VulkanRenderer::Shutdown: Starting...");

		vkDeviceWaitIdle(m_Device);

		PX_CORE_INFO("Starting ShaderLibrary shutdown...");

		m_ShaderLibrary->Shutdown();

		PX_CORE_INFO("Completed ShaderLibrary shutdown.");

		PX_CORE_INFO("Starting TextureSystem shutdown...");

		m_TextureSystem->Shutdown();

		PX_CORE_INFO("Completed TextureSystem shutdown.");

		PX_CORE_INFO("Started destruction of performance pools...");

		for (uint32_t i = 0; i < m_Specification.MaxFramesInFlight; i++)
		{
			vkDestroyQueryPool(m_Device, m_TimestampQueryPools[i], nullptr);
		}
		m_TimestampQueryPools.clear();
		vkDestroyQueryPool(m_Device, m_PipelineStatisticsQueryPool, nullptr);
		m_PipelineStatisticsQueryPool = VK_NULL_HANDLE;

		PX_CORE_INFO("Started destruction of performance pools...");
		PX_CORE_INFO("Started destruction of FrameObjects (Synch, Commands, UBOs) for {0} frames...", m_Frames.size());

		for (size_t i = 0; i < m_Frames.size(); i++)
		{
			vkDestroySemaphore(m_Device, m_Frames[i].Semaphores.PresentSemaphore, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].Semaphores.RenderSemaphore, nullptr);

			vkDestroyFence(m_Device, m_Frames[i].Fence, nullptr);

			vkDestroyCommandPool(m_Device, m_Frames[i].Commands.Pool, nullptr);

			vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Frames[i].CamUniformBuffer.Buffer, m_Frames[i].CamUniformBuffer.Allocation);
			vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Frames[i].ObjectBuffer.Buffer, m_Frames[i].ObjectBuffer.Allocation);
		}
		m_Frames.clear();

		PX_CORE_INFO("Completed FrameObjects (Synch, Commands, UBOs) destruction for {0} frames...", m_Frames.size());
		PX_CORE_WARN("Started destruction of leftovers and other things...");

		//TODO:: Move UploadContext cleanup to respective place!!!
		vkDestroyFence(m_Device, m_UploadContext->Fence, nullptr);

		vmaDestroyBuffer(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Buffer, m_SceneParameterBuffer.Allocation);

		//Layouts get destroyed by LayoutCache

		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolGfx, nullptr);
		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolTrsf, nullptr);

		m_ImGui->Destroy();

		m_FinalImage->Free();


		vkDestroySampler(m_Device, m_TextureSampler, nullptr);

		

		PX_CORE_WARN("Completed leftover and other things' destruction.");
		PX_CORE_INFO("VulkanRenderer::Shutdown: Completed.");
	}
	
	void VulkanRenderer::WaitForDeviceFinished()
	{
		vkDeviceWaitIdle(m_Device);
	}

	//FrameData
	bool VulkanRenderer::BeginFrame()
	{
		PX_PROFILE_FUNCTION();


		GetQueryResults();

		//VulkanContext::FreeFrameResources(m_CurrentFrameIndex);


		vkWaitForFences(m_Device, 1, &GetCurrentFrame().Fence, VK_TRUE, UINT64_MAX);
		vkResetCommandPool(m_Device, GetCurrentFrame().Commands.Pool, 0);
		m_SwapchainFrame = m_Swapchain->AcquireNextImageIndex(GetCurrentFrame().Semaphores.PresentSemaphore);
		if (!m_SwapchainFrame)
			return false;
		vkResetFences(m_Device, 1, &GetCurrentFrame().Fence);

		m_Specification.State.CurrentSwapchainImageIndex = m_CurrentSwapchainImageIndex = m_SwapchainFrame->CurrentImageIndex;
		m_SwapchainFrame->CurrentFence = GetCurrentFrame().Fence;
		m_SwapchainFrame->PresentSemaphore = GetCurrentFrame().Semaphores.PresentSemaphore;
		m_SwapchainFrame->RenderSemaphore = GetCurrentFrame().Semaphores.RenderSemaphore;
		

		return true;
	}
	void VulkanRenderer::EndFrame()
	{
		m_Specification.State.LastFrameIndex = m_LastFrameIndex = m_CurrentFrameIndex;
		m_Specification.State.CurrentFrameIndex = m_CurrentFrameIndex = (++m_CurrentFrameIndex) % m_Specification.MaxFramesInFlight;
		m_Specification.State.TotalFrames++;
	}
	
	void VulkanRenderer::Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount)
	{
		PX_PROFILE_FUNCTION();


		Ref<VulkanShader> vkShader = std::dynamic_pointer_cast<VulkanShader>(material->GetShader());


		uint32_t frameIndex = m_CurrentFrameIndex % m_Specification.MaxFramesInFlight;
		uint32_t camUniformOffset = PadUniformBuffer(sizeof(SceneUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;

		uint32_t height = m_Swapchain->GetProperties().Height;
		uint32_t width = m_Swapchain->GetProperties().Width;
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(height);
		viewport.width = static_cast<float>(width);
		viewport.height = -static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };
		vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);


		vkCmdBindDescriptorSets(
			m_ActiveCommandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_ActivePipeline->GetLayout(), 
			0, 
			1, 
			&GetCurrentFrame().GlobalDescriptorSet, 
			1, 
			&camUniformOffset
		);

		/* Instead of using memcpy here, we are doing a different trick. It is possible to cast the void* from mapping the buffer into another type, and write into it normally.
		* This will work completely fine, and makes it easier to write complex types into a buffer.
		**/
		//mapping of model data into the SSBO object buffer
		uint32_t objectUniformOffset = PadUniformBuffer(sizeof(ObjectUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;

		ObjectUniform objectUniform;
		//objectUniform.ModelMatrix = renderable.ModelMatrix;
		objectUniform.TexID = 0;
		objectUniform.TilingFactor = 1.0f;

		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().ObjectBuffer.Allocation, &data);
		memcpy(data, &objectUniform, sizeof(ObjectUniform));
		vmaUnmapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().ObjectBuffer.Allocation);

		vkCmdBindDescriptorSets(
			m_ActiveCommandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_ActivePipeline->GetLayout(), 
			1, 
			1, 
			&GetCurrentFrame().ObjectDescriptorSet, 
			0, 
			&objectUniformOffset);
		
		
		VkWriteDescriptorSet samplerWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		samplerWrite.dstBinding = 0;
		samplerWrite.dstArrayElement = 0;
		samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerWrite.descriptorCount = 1;
		samplerWrite.dstSet = GetCurrentFrame().TextureDescriptorSet;

		VkDescriptorImageInfo samplerInfo{};
		samplerInfo.sampler = m_TextureSampler;
		samplerWrite.pImageInfo = &samplerInfo;

		auto& activeTextures = m_TextureSystem->GetActiveTextures();
		//TODO: Get maxImageSlots from config as const
		std::array<VkDescriptorImageInfo, 32> imageInfos;
		for (uint32_t i = 0; i < activeTextures.size(); i++)
		{
			imageInfos[i] = std::dynamic_pointer_cast<VulkanImage2D>(activeTextures[i]->GetImage())->GetImageInfo();
		}
		VkWriteDescriptorSet setWrites{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		setWrites.dstBinding = 1;
		setWrites.dstArrayElement = 0;
		setWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		setWrites.descriptorCount = imageInfos.size();
		setWrites.pBufferInfo = 0;
		setWrites.dstSet = GetCurrentFrame().TextureDescriptorSet;
		setWrites.pImageInfo = imageInfos.data();


		VkWriteDescriptorSet writes[2] = {samplerWrite, setWrites};

		vkUpdateDescriptorSets(m_Device, 2, writes, 0, nullptr);

		vkCmdBindDescriptorSets(
			m_ActiveCommandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_ActivePipeline->GetLayout(), 
			2, 
			1, 
			&GetCurrentFrame().TextureDescriptorSet, 
			0, 
			nullptr
		);
		

		VkBuffer vertexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(vertices)->GetAllocation().Buffer;
		VkBuffer indexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(indices)->GetAllocation().Buffer;
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(m_ActiveCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_ActiveCommandBuffer, indexCount, 1, 0, 0, 0);

		vkCmdEndQuery(m_ActiveCommandBuffer, m_PipelineStatisticsQueryPool, 0);
	}

	void VulkanRenderer::DrawRenderable(const Renderable& renderable)
	{
		PX_PROFILE_FUNCTION();


		Ref<VulkanShader> vkShader = std::dynamic_pointer_cast<VulkanShader>(renderable.Material.Shader);


		uint32_t frameIndex = m_CurrentFrameIndex % m_Specification.MaxFramesInFlight;
		uint32_t camUniformOffset = PadUniformBuffer(sizeof(SceneUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;

		uint32_t height = m_Swapchain->GetProperties().Height;
		uint32_t width = m_Swapchain->GetProperties().Width;
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(height);
		viewport.width = static_cast<float>(width);
		viewport.height = -static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };
		vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);


		vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 0, 1, &GetCurrentFrame().GlobalDescriptorSet, 1, &camUniformOffset);
				
		/* Instead of using memcpy here, we are doing a different trick. It is possible to cast the void* from mapping the buffer into another type, and write into it normally.
		* This will work completely fine, and makes it easier to write complex types into a buffer.
		**/
		//mapping of model data into the SSBO object buffer
		uint32_t objectUniformOffset = PadUniformBuffer(sizeof(ObjectUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;
		
		ObjectUniform objectUniform;
		//objectUniform.ModelMatrix = renderable.ModelMatrix;
		objectUniform.TexID = renderable.Material.TexID;
		objectUniform.TilingFactor = renderable.Material.TilingFactor;

		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().ObjectBuffer.Allocation, &data);
		memcpy(data, &objectUniform, sizeof(ObjectUniform));
		vmaUnmapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().ObjectBuffer.Allocation);
		
		vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &GetCurrentFrame().ObjectDescriptorSet, 0, &objectUniformOffset);

		if (renderable.Material.Texture)
		{		
			auto& activeTextures = m_TextureSystem->GetActiveTextures(); 
			//TODO: Get maxImageSlots form config as const
			std::array<VkDescriptorImageInfo, 32> imageInfos;
			for (uint32_t i = 0; i < activeTextures.size(); i++)
			{
				imageInfos[i] = std::dynamic_pointer_cast<VulkanImage2D>(activeTextures[i]->GetImage())->GetImageInfo(); 
			}
			VkWriteDescriptorSet setWrites{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			setWrites.dstBinding = 1;
			setWrites.dstArrayElement = 0;
			setWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			setWrites.descriptorCount = imageInfos.size();
			setWrites.pBufferInfo = 0;
			setWrites.dstSet = GetCurrentFrame().TextureDescriptorSet;
			setWrites.pImageInfo = imageInfos.data();

			vkUpdateDescriptorSets(m_Device, 1, &setWrites, 0, nullptr);
			vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 2, 1, &GetCurrentFrame().TextureDescriptorSet, 0, nullptr);
		}
		
		VkBuffer vertexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(renderable.MeshData.VertexBuffer)->GetAllocation().Buffer;
		VkBuffer indexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(renderable.MeshData.IndexBuffer)->GetAllocation().Buffer;
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(m_ActiveCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);


		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		float x = 0.0f + time;
		//glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(x, 0.0f, 1.0f));
		//PushConstants pushConstant;
		//pushConstant.ModelMatrix = model;
		//vkCmdPushConstants(cmd, m_GraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstant);
			
		vkCmdDrawIndexed(m_ActiveCommandBuffer, 6, 1, 0, 0, 0);
	}

	void VulkanRenderer::InitFrameData()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VulkanRenderer::InitFrameData: Starting initialization...");

		uint32_t maxFrames = m_Specification.MaxFramesInFlight;
		PX_CORE_INFO("MaxFramesInFlight: {0}", maxFrames);

		if (m_Frames.size() != maxFrames)
		{
			m_Frames.resize(maxFrames);
			PX_CORE_INFO("Creating synchronization objects for {0} frames...", maxFrames);
			
			//Semaphores
			VkSemaphoreCreateInfo createSemaphoreInfo{};
			createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for (size_t i = 0; i < maxFrames; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_Frames[i].Semaphores.RenderSemaphore), VK_SUCCESS, "Failed to create RenderSemaphore!");
				PX_CORE_VK_ASSERT(vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_Frames[i].Semaphores.PresentSemaphore), VK_SUCCESS, "Failed to create PresentSemaphore!");
			}
			//Fences
			VkFenceCreateInfo createFenceInfo{};
			createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (size_t i = 0; i < maxFrames; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_Frames[i].Fence), VK_SUCCESS, "Failed to create Fence!");
			}

			PX_CORE_INFO("Completed Synchronization objects creation for {0} frames.", maxFrames);
			PX_CORE_INFO("Creating Command objects for {0} frames...", maxFrames);

			//Commands
			VkCommandPoolCreateInfo poolci{};
			poolci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolci.pNext = nullptr;
			poolci.queueFamilyIndex = VulkanContext::GetDevice()->GetQueueFamilies().GraphicsFamilyIndex;
			poolci.flags = 0;

			VkCommandBufferAllocateInfo bufferci{};
			bufferci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			bufferci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			bufferci.commandBufferCount = 1;

			for (uint32_t i = 0; i < maxFrames; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateCommandPool(m_Device, &poolci, nullptr, &m_Frames[i].Commands.Pool), VK_SUCCESS, "Failed to create graphics command pool!");

				bufferci.commandPool = m_Frames[i].Commands.Pool;
				PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(m_Device, &bufferci, &m_Frames[i].Commands.Buffer), VK_SUCCESS, "Failed to create CommandBuffer!");
			}

			PX_CORE_INFO("Completed Command objects creation for {0} frames.", maxFrames);
			
			PX_CORE_INFO("Creating (global) UBOs for {0} frames...", maxFrames);

			const size_t align = VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			const size_t sceneUniformSize = maxFrames * PadUniformBuffer(sizeof(SceneUniform), align);

			m_SceneParameterBuffer = VulkanBuffer::CreateAllocation(sceneUniformSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			for (uint32_t i = 0; i < maxFrames; i++)
			{
				m_Frames[i].CamUniformBuffer = VulkanBuffer::CreateAllocation(sizeof(CameraUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
				
				//TODO: Track these buffers sizes
				m_Frames[i].ObjectBuffer = VulkanBuffer::CreateAllocation(sizeof(ObjectUniform) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			}
			
			PX_CORE_INFO("Completed (global) UBO creation for {0} frames.", maxFrames);

			m_FramebufferWidth = m_ViewportWidth = m_Swapchain->GetProperties().Width;
			m_FramebufferHeight = m_ViewportHeight = m_Swapchain->GetProperties().Height;
		}
		else
		{
			PX_CORE_WARN("VulkanRenderer::Init call after first initialization!");
		}


		PX_CORE_INFO("VulkanRenderer::InitFrameData: Completed initialization.");
	}

	void VulkanRenderer::CreateFinalImage(Ref<Image2D> sourceImage)
	{
		PX_PROFILE_FUNCTION();


		Ref<VulkanImage2D> sourceImageVK = std::dynamic_pointer_cast<VulkanImage2D>(sourceImage);
		if (!m_FinalImage)
		{
			InitFinalImage(m_ViewportWidth, m_ViewportHeight);
		}

		//Transition final image
		m_FinalImage->TransitionImageLayout(
			VK_IMAGE_LAYOUT_UNDEFINED,	
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_MEMORY_READ_BIT, 
			VK_ACCESS_TRANSFER_READ_BIT,	
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		//Transition source image
		sourceImageVK->TransitionImageLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
		
		//Image copying
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkImageCopy imageCopyRegion{};
				imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.srcSubresource.layerCount = 1;
				imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.dstSubresource.layerCount = 1;
				imageCopyRegion.extent.width = sourceImageVK->GetSpecification().Width;
				imageCopyRegion.extent.height = sourceImageVK->GetSpecification().Height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, sourceImageVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_FinalImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
			});
		//Transition swapchain image back into present
		m_FinalImage->TransitionImageLayout(
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}
	void VulkanRenderer::PrepareSwapchainImage(Ref<Image2D> sourceImage)
	{
		PX_PROFILE_FUNCTION();


		Ref<VulkanImage2D> sourceImageVK = std::dynamic_pointer_cast<VulkanImage2D>(sourceImage);

		//Transition swapchain image
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = m_SwapchainFrame->CurrentImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		//Transition final rendered image
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = sourceImageVK->GetImage();
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		//Image copying
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkImageCopy imageCopyRegion{};
				imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.srcSubresource.layerCount = 1;
				imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.dstSubresource.layerCount = 1;
				imageCopyRegion.extent.width = sourceImageVK->GetSpecification().Width;
				imageCopyRegion.extent.height = sourceImageVK->GetSpecification().Height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, sourceImageVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_SwapchainFrame->CurrentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
			});
		//Transition swapchain image back into present
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = m_SwapchainFrame->CurrentImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //memory_write
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
	}

	// State
	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		if (m_FramebufferResized)
			return;


		m_FramebufferResized = true;

		
		m_Specification.State.WindowWidth = m_FramebufferWidth = width;
		m_Specification.State.WindowHeight = m_FramebufferHeight = height;
	}

	void VulkanRenderer::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportResized)
			return;

		m_ViewportResized = true;


		m_Specification.State.ViewportWidth = m_ViewportWidth = width;
		m_Specification.State.ViewportHeight = m_ViewportHeight = height;

		InitFinalImage(m_ViewportWidth, m_ViewportHeight);

		m_ViewportResized = false;
	}

	void VulkanRenderer::UpdateCamera(const CameraUniform& cam)
	{
		PX_PROFILE_FUNCTION();


		CameraUniform camOut;
		camOut.ViewMatrix = cam.ViewMatrix;
		camOut.ProjectionMatrix = cam.ProjectionMatrix;
		camOut.ProjectionMatrix[1][1] *= -1; // either this or flip the viewport
		camOut.ViewProjMatrix = camOut.ProjectionMatrix * camOut.ViewMatrix;

		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().CamUniformBuffer.Allocation, &data);
		memcpy(data, &camOut, sizeof(CameraUniform));
		vmaUnmapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().CamUniformBuffer.Allocation);


		//m_SceneParameter.AmbientColor = glm::vec4(glm::cos(m_DebugInfo.TotalFrames /10.0f), 1.0f, glm::sin(m_DebugInfo.TotalFrames /10.0f), 0.5f);
		m_SceneParameter.AmbientColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);
		char* sceneData;
		vmaMapMemory(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Allocation, (void**)&sceneData);
		sceneData += PadUniformBuffer(sizeof(SceneUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * m_CurrentFrameIndex;
		memcpy(sceneData, &m_SceneParameter, sizeof(SceneUniform));
		vmaUnmapMemory(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Allocation);
		
	}

	// Commands
	void VulkanRenderer::BeginCommandBuffer(const void* commBuf)
	{
		PX_PROFILE_FUNCTION();


		m_ActiveCommandBuffer = (VkCommandBuffer)commBuf;
		m_SwapchainFrame->Commands.push_back(m_ActiveCommandBuffer);
		
		VkCommandBufferBeginInfo cmdBegin{};
		cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBegin.pNext = nullptr;
		cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(m_ActiveCommandBuffer, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");
		
		/*VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
		nameInfo.objectHandle = (uint64_t)m_ActiveCommandBuffer;
		nameInfo.pObjectName = "Frame CommandBuffer Frame";
		NameVkObject(m_Device, nameInfo);*/		
	}
	void VulkanRenderer::EndCommandBuffer()
	{

		PX_CORE_VK_ASSERT(vkEndCommandBuffer(m_ActiveCommandBuffer), VK_SUCCESS, "Failed to record graphics command buffer!");
		m_ActiveCommandBuffer = VK_NULL_HANDLE;
	}
	
	void VulkanRenderer::InitCommandControl()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VulkanRenderer::InitCommandControl: Starting...");

		m_CommandControl = CreateScope<VulkanCommandControl>();
		PX_CORE_ASSERT(m_CommandControl, "Failed to create CommandControl!");

		m_UploadContext = m_CommandControl->CreateUploadContext();
		PX_CORE_ASSERT(m_UploadContext, "Failed to get UploadContext!");

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_UploadContext->Fence), VK_SUCCESS, "Failed to create upload fence!");

		PX_CORE_INFO("VulkanRenderer::InitCommandControl: Completed.");
	}

	//Renderpass
	void VulkanRenderer::BeginRenderPass(Ref<RenderPass> renderPass)
	{
		PX_PROFILE_FUNCTION();


		//TEMP:
		ResetQuerys(m_ActiveCommandBuffer);

		m_ActiveRenderPass = std::dynamic_pointer_cast<VulkanRenderPass>(renderPass);
		Ref<VulkanFramebuffer> fb = std::dynamic_pointer_cast<VulkanFramebuffer>(m_ActiveRenderPass->GetSpecification().TargetFramebuffer);


		//TODO: Get from the fb
		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext = nullptr;
		info.renderPass = m_ActiveRenderPass->GetVulkanObj();
		info.framebuffer = fb->GetVulkanObj();
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = { fb->GetSpecification().Width, fb->GetSpecification().Height };

		info.clearValueCount = static_cast<uint32_t>(clearColor.size());
		info.pClearValues = clearColor.data();

		vkCmdBeginRenderPass(m_ActiveCommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}
	void VulkanRenderer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_ActiveCommandBuffer);
		m_ActiveRenderPass = nullptr;
	}

	// Pipeline
	void VulkanRenderer::BindPipeline(Ref<Pipeline> pipeline)
	{
		PX_PROFILE_FUNCTION();


		m_ActivePipeline = std::dynamic_pointer_cast<VulkanPipeline>(pipeline);

		if (pipeline->GetSpecification().DynamicViewAndScissors)
		{
			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height };

			VkViewport viewport;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)m_ActiveRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Width;
			viewport.height = (float)m_ActiveRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Height;

			vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);
			vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);
		}

		vkCmdBeginQuery(m_ActiveCommandBuffer, m_PipelineStatisticsQueryPool, 0, 0);
		vkCmdBindPipeline(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetVulkanObj());
	}

	// GUI
	void VulkanRenderer::BeginGUIRenderPass()
	{
		if (m_FramebufferResized)
		{
			VkExtent2D extent{ m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height };
			m_ImGui->OnSwapchainRecreate(Application::Get()->GetWindow().GetSwapchain()->GetImageViews(), extent);
			// TODO: Find right place for flag
			m_FramebufferResized = false;
		}
		m_ImGui->BeginRenderPass(m_ActiveCommandBuffer, m_CurrentSwapchainImageIndex, { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height });
	}
	void VulkanRenderer::EndGUIRenderPass()
	{
		m_ImGui->EndRenderPass();
	}
	
	void VulkanRenderer::DrawGUI()
	{
		PX_PROFILE_FUNCTION();


		m_ImGui->RenderDrawData();
	}
	
	// ImGui
	void VulkanRenderer::InitImGui()
	{
		auto& swapchain = Application::Get()->GetWindow().GetSwapchain();
		m_ImGui->Init(swapchain->GetImageViews(), swapchain->GetProperties().Width, swapchain->GetProperties().Height);
	}
	void VulkanRenderer::BeginImGuiFrame()
	{
		m_ImGui->BeginFrame();
	}
	void VulkanRenderer::EndImGuiFrame()
	{
		m_ImGui->EndFrame();
	}
	void VulkanRenderer::CreateDescriptors()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VulkanRenderer::CreateDescriptors: Starting...");
		PX_CORE_INFO("Creating (global) DescriptorLayouts... ");
		//TODO:: refactor global descriptor set (layout) creation to MaterialSystem?

	// Descriptors
		//Global DescriptorLayoutCreation
		VkDescriptorSetLayoutBinding camBinding{};
		VkDescriptorSetLayoutBinding sceneBinding{};
		{
			camBinding.binding = 0;
			camBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			camBinding.descriptorCount = 1;
			camBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			camBinding.pImmutableSamplers = nullptr;


			sceneBinding.binding = 1;
			sceneBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			sceneBinding.descriptorCount = 1;
			sceneBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			sceneBinding.pImmutableSamplers = nullptr;

			std::vector<VkDescriptorSetLayoutBinding> globalBindings = { camBinding, sceneBinding };


			VkDescriptorSetLayoutCreateInfo globalInfo{};
			globalInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			globalInfo.pNext = nullptr;

			globalInfo.flags = 0;
			globalInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
			globalInfo.pBindings = globalBindings.data();

			m_GlobalDescriptorSetLayout = VulkanContext::GetDescriptorLayoutCache()->CreateDescriptorLayout(&globalInfo);
		}
		for (uint32_t i = 0; i < m_Specification.MaxFramesInFlight; i++)
		{
			VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());
			VkDescriptorBufferInfo camInfo{};
			camInfo.buffer = m_Frames[i].CamUniformBuffer.Buffer;
			camInfo.offset = 0;
			camInfo.range = sizeof(CameraUniform);
			builder.BindBuffer(camBinding, &camInfo);


			VkDescriptorBufferInfo sceneInfo{};
			sceneInfo.buffer = m_SceneParameterBuffer.Buffer;
			sceneInfo.offset = 0;
			sceneInfo.range = sizeof(SceneUniform);
			builder.BindBuffer(sceneBinding, &sceneInfo);

			builder.Build(m_Frames[i].GlobalDescriptorSet, m_GlobalDescriptorSetLayout, "GlobalDS");
		}

		PX_CORE_INFO("Completed (global) DescriptorLayouts creation. ");
		PX_CORE_INFO("Creating Object DescriptorLayouts... ");
		
		VkDescriptorSetLayoutBinding objectBinding{};
		{
			objectBinding.binding = 0;
			objectBinding.descriptorCount = 1;
			objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			objectBinding.pImmutableSamplers = nullptr;
			objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		
			VkDescriptorSetLayoutCreateInfo objectInfo{};
			objectInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			objectInfo.pNext = nullptr;

			objectInfo.flags = 0;
			objectInfo.bindingCount = 0;
			objectInfo.pBindings = &objectBinding;

			m_ObjectDescriptorSetLayout = VulkanContext::GetDescriptorLayoutCache()->CreateDescriptorLayout(&objectInfo);
		}
		for (uint32_t i = 0; i < m_Specification.MaxFramesInFlight; i++)
		{
			VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());
			VkDescriptorBufferInfo objectInfo{};
			objectInfo.buffer = m_Frames[i].ObjectBuffer.Buffer;
			objectInfo.offset = 0;
			objectInfo.range = sizeof(ObjectUniform) * MAX_OBJECTS;
			builder.BindBuffer(objectBinding, &objectInfo);

			builder.Build(m_Frames[i].ObjectDescriptorSet, m_ObjectDescriptorSetLayout, "ObjectDS");
		}


		PX_CORE_INFO("Completed Object DescriptorLayouts creation. ");
		PX_CORE_INFO("Creating TextureArray DescriptorLayouts... ");

		//TextureDescriptorLayoutCreation
		VkDescriptorSetLayoutBinding samplerBinding{};
		VkDescriptorSetLayoutBinding textureArrayBinding{};
		{
			samplerBinding.binding = 0;
			samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			samplerBinding.descriptorCount = 1;
			samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			samplerBinding.pImmutableSamplers = nullptr;
		

			textureArrayBinding.binding = 1;
			textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			textureArrayBinding.descriptorCount = m_TextureSystem->GetConfig().MaxTextureSlots;
			textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			textureArrayBinding.pImmutableSamplers = nullptr;

			std::vector<VkDescriptorSetLayoutBinding> textureBindings = { samplerBinding, textureArrayBinding };

			VkDescriptorSetLayoutCreateInfo textureInfo{};
			textureInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			textureInfo.pNext = nullptr;

			textureInfo.flags = 0;
			textureInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
			textureInfo.pBindings = textureBindings.data();

			m_TextureDescriptorSetLayout = VulkanContext::GetDescriptorLayoutCache()->CreateDescriptorLayout(&textureInfo);
		}

		auto& activeTextures = m_TextureSystem->GetActiveTextures();
		//TODO: Get maxImageSlots from config as const
		std::vector<VkDescriptorImageInfo> imageInfos;	
		imageInfos.resize(m_TextureSystem->GetConfig().MaxTextureSlots);
		for (uint32_t i = 0; i < imageInfos.size(); i++)
		{
			Ref<Image2D> image = activeTextures[i]->GetImage();
			//TODO: Define BindImages for arrays of image infos!
			Ref<VulkanImage2D> vkimage = std::dynamic_pointer_cast<VulkanImage2D>(activeTextures[i]->GetImage());
			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].imageView = vkimage->GetImageView();
			imageInfos[i].sampler = nullptr;
		}

		for (uint32_t i = 0; i < m_Specification.MaxFramesInFlight; i++)
		{	
			VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());

			VkDescriptorImageInfo samplerInfo{};
			samplerInfo.sampler = m_TextureSampler;

			builder.BindImage(samplerBinding, &samplerInfo);
			builder.BindImage(textureArrayBinding, imageInfos.data());

			builder.Build(m_Frames[i].TextureDescriptorSet, m_TextureDescriptorSetLayout, "TextureArrayDS");
		}




		PX_CORE_INFO("Completed textureArray DescriptorLayouts creation. ");
		PX_CORE_INFO("VulkanRenderer::CreateDescriptors: Completed.");
	}

	void* VulkanRenderer::GetGUIDescriptorSet(Ref<Image2D> image)
	{
		Ref<VulkanImage2D> vkImage = std::dynamic_pointer_cast<VulkanImage2D>(image);

		if ((VkDescriptorSet)vkImage->GetDescriptorSet())
			m_ImGui->FreeImGuiDescriptorSet((VkDescriptorSet)vkImage->GetDescriptorSet());

		vkImage->TransitionImageLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_MEMORY_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
		
		vkImage->SetDescriptorSet(m_ImGui->GetImGUIDescriptorSet(vkImage->GetImageView(), vkImage->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		return vkImage->GetDescriptorSet();
	}

	
	//TEMP
	// Samplers
	void VulkanRenderer::CreateSamplers()
	{
		PX_PROFILE_FUNCTION();


		// Texture Sampler
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(VulkanContext::GetDevice()->GetPhysicalDevice(), &properties);
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_TRUE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxLod = 0.0f;
			samplerInfo.minLod = 0.0f;
			PX_CORE_VK_ASSERT(vkCreateSampler(VulkanContext::GetDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_TextureSampler), VK_SUCCESS, "Failed to create texture sampler!");

#ifdef PX_DEBUG
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
			nameInfo.objectHandle = (uint64_t)m_TextureSampler;
			nameInfo.pObjectName = "TextureSampler";
			NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG
		}	
	}
	// Descriptors
	size_t VulkanRenderer::PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment)
	{
		size_t alignedSize = inputSize;
		if (minGPUBufferOffsetAlignment > 0)
		{
			alignedSize = (alignedSize + minGPUBufferOffsetAlignment - 1) & ~(minGPUBufferOffsetAlignment - 1);
		}
		return alignedSize;
	}
			

	// Debugging and Performance
	void VulkanRenderer::StartTimestampQuery(const std::string& name)
	{
		if (m_TimestampQueries.find(name) == m_TimestampQueries.end())
		{
			PX_CORE_ERROR("VulkanRenderer::StartTimestampQuery: Could not start query {0}", name);
			return;
		}

		vkCmdWriteTimestamp(m_ActiveCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_TimestampQueryPools[m_CurrentFrameIndex], m_TimestampQueries[name]);
	}
	void VulkanRenderer::StopTimestampQuery(const std::string& name)
	{
		if (m_TimestampQueries.find(name) == m_TimestampQueries.end())
		{
			PX_CORE_ERROR("VulkanRenderer::StartTimestampQuery: Could not start query {0}", name);
			return;
		}

		vkCmdWriteTimestamp(m_ActiveCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_TimestampQueryPools[m_CurrentFrameIndex], m_TimestampQueries[name] + 1);
	}

	void VulkanRenderer::InitPerformanceQueryPools()
	{
		PX_PROFILE_FUNCTION();

		Ref<VulkanDevice> device = VulkanContext::GetDevice();

		PX_CORE_INFO("VulkanRenderer::InitPerfromanceQueryPools: Starting...");
		PX_CORE_INFO("Starting TimestampPool creation...");
		{
			uint32_t framesInFlight = m_Specification.MaxFramesInFlight;
			m_TimestampQueries["ImguiRenderpass"] = 0;
			m_Statistics.TimestampResults["ImguiRenderpass"] = 0;

			VkQueryPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.queryType = VK_QUERY_TYPE_TIMESTAMP;
			info.queryCount = m_TimestampQueries.size() * 2;
			info.pipelineStatistics = 0;

			m_TimestampQueryPools.resize(framesInFlight);
			m_Timestamps.resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateQueryPool(m_Device, &info, nullptr, &m_TimestampQueryPools[i]), VK_SUCCESS, "Failed to create TimeStamp pool!");
				m_Timestamps[i].resize(m_TimestampQueries.size() * 2);
				for (uint32_t j = 0; j < m_Timestamps[i].size(); j++)
				{
					m_Timestamps[i][j] = 0;
				}
			}
		}
		PX_CORE_INFO("Completed TimestampPool creation.");
		// TODO: Check if PieplienStatistics feature has been successfully enabled during device creation
		PX_CORE_INFO("Starting PipelineStaticsticsPool creation...");
		{
			VkQueryPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
			info.queryCount = 7;
			info.pipelineStatistics =
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

			PX_CORE_VK_ASSERT(vkCreateQueryPool(m_Device, &info, nullptr, &m_PipelineStatisticsQueryPool), VK_SUCCESS, "Failed to create Pipeline stats pool!");


			m_Statistics.PipelineStatNames = { "Input assembly vertex count (indices) ",
									"Input assembly primitives count ",
									"Vertex shader invocations ",
									"Clipping stage - primitives reached ",
									"Clipping stage - primitives passed ",
									"Fragment shader invocations ",
									"Compute shader shader invocations ", };
			m_Statistics.PipelineStats.resize(m_Statistics.PipelineStatNames.size());

		}
		PX_CORE_INFO("Completed PipelineStaticsticsPool creation.");
		PX_CORE_INFO("VulkanRenderer::InitPerfromanceQueryPools: Completed.");
	}

	void VulkanRenderer::GetQueryResults()
	{
		//Pipeline stats
		{
			if (!m_PipelineStatisticsQueryPool)
				return;

			uint32_t count = static_cast<uint32_t>(m_Statistics.PipelineStats.size());
			vkGetQueryPoolResults(m_Device, m_PipelineStatisticsQueryPool, 0, 1, count * sizeof(uint64_t), m_Statistics.PipelineStats.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		}

		//Timestamps last frame
		{
			uint32_t lastFrameIdx = (m_CurrentFrameIndex - 1) % m_Specification.MaxFramesInFlight;
			if (!m_TimestampQueryPools[lastFrameIdx])
				return;

			uint32_t count = static_cast<uint32_t>(m_Timestamps[lastFrameIdx].size());
			vkGetQueryPoolResults(m_Device, m_TimestampQueryPools[lastFrameIdx], 0, count, count * sizeof(uint64_t), m_Timestamps[lastFrameIdx].data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);


			m_Statistics.TimestampResults["ImguiRenderpass"] = (uint64_t)(((m_Timestamps[lastFrameIdx][1] - m_Timestamps[lastFrameIdx][0])) *
				VulkanContext::GetDevice()->GetLimits().TimestampPeriod);
		}
	}
	void VulkanRenderer::ResetQuerys(VkCommandBuffer cmd)
	{
		vkCmdResetQueryPool(cmd, m_PipelineStatisticsQueryPool, 0, static_cast<uint32_t>(m_Statistics.PipelineStats.size()));
		vkResetQueryPool(m_Device, m_TimestampQueryPools[m_CurrentFrameIndex], 0, 2);
	}

	// Resource Creation and Getters
	void VulkanRenderer::InitFinalImage(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();

		if (width <= 0 || height <= 0)
		{
			PX_CORE_WARN("VulkanRenderer::InitFinalImage: Cannot create image with extent {}, {}", width, height);
			return;
		}

		if (m_FinalImage)
		{
			m_FinalImage->Free();
			m_FinalImage = nullptr;
		}

		ImageSpecification imageSpec{};
		imageSpec.DebugName = "Renderer::FinalImage";
		imageSpec.Width = width;
		imageSpec.Height = height;
		imageSpec.ChannelCount = 4;
		imageSpec.Format = ImageFormat::RGBA8;
		imageSpec.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		imageSpec.MipLevels = 1;
		imageSpec.Tiling = ImageTiling::LINEAR;
		imageSpec.Usages = { ImageUsage::SAMPLED, ImageUsage::COLOR_ATTACHMENT, ImageUsage::COPY_DST };
		imageSpec.DedicatedSampler = true;
		imageSpec.CreateDescriptorOnInit = false;
		m_FinalImage = CreateRef<VulkanImage2D>(imageSpec);
	}

	FrameData& VulkanRenderer::GetFrame(uint32_t index)
	{
		PX_CORE_ASSERT(index < m_Frames.size(), "Index too big!");
		return m_Frames[index];
	}

}
