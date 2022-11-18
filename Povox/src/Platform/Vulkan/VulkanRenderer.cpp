#include "pxpch.h"
#include "VulkanRenderer.h"

#include "VulkanDebug.h"
#include "VulkanFramebuffer.h"


#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


namespace Povox {

	Scope<VulkanImGui> VulkanRenderer::m_ImGui = nullptr;

	VulkanRenderer::VulkanRenderer(const RendererSpecification& specs)
		:m_Specification(specs)
	{
		Init();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		PX_CORE_TRACE("Vulkan Renderer API delete!");
	}

	void VulkanRenderer::Init()
	{
		PX_PROFILE_FUNCTION();


		m_Device = VulkanContext::GetDevice()->GetVulkanDevice();
		m_Swapchain = Application::Get()->GetWindow().GetSwapchain();

		InitCommands();

		m_DescriptorAllocator = CreateScope<VulkanDescriptorAllocator>();
		m_DescriptorCache = CreateScope<VulkanDescriptorLayoutCache>();

		m_ImGui = CreateScope<VulkanImGui>(Application::Get()->GetWindow().GetSwapchain()->GetImageFormat(), (uint8_t)m_Specification.MaxFramesInFlight);
		
		
		m_SceneObjects = new Renderable[m_Specification.MaxSceneObjects];


		
		
		PX_CORE_INFO("Finished initializing VulkanRenderer!");
	}

	void VulkanRenderer::Shutdown()
	{
		vkDeviceWaitIdle(m_Device);
		
		for (size_t i = 0; i < m_Frames.size(); i++)
		{
			vkDestroySemaphore(m_Device, m_Frames[i].Semaphores.PresentSemaphore, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].Semaphores.RenderSemaphore, nullptr);
			
			vkDestroyFence(m_Device, m_Frames[i].Fence, nullptr);
			
			vkDestroyCommandPool(m_Device, m_Frames[i].Commands.Pool, nullptr);
		}
		vkDestroyFence(m_Device, m_UploadContext->Fence, nullptr);

		vmaDestroyBuffer(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Buffer, m_SceneParameterBuffer.Allocation);

		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolGfx, nullptr);
		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolTrsf, nullptr);

		m_ImGui->Destroy();

		m_DescriptorCache->Cleanup();
		m_DescriptorAllocator->Cleanup();
	}

	void VulkanRenderer::BeginFrame()
	{
		PX_CORE_WARN("Start swap buffers!");


		//ATTANTION: Possible, that I need to render before the clearing, maybe, possibly
		auto& currentFreeQueue = VulkanContext::GetResourceFreeQueue()[m_CurrentImageIndex];
		for (auto& func : currentFreeQueue)
			func();
		currentFreeQueue.clear();
		vkWaitForFences(m_Device, 1, &GetCurrentFrame().Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &GetCurrentFrame().Fence);


		vkResetCommandPool(m_Device, GetCurrentFrame().Commands.Pool, 0);


		SwapchainFrame* swapchainFrame = m_Swapchain->AcquireNextImageIndex(GetCurrentFrame().Semaphores.PresentSemaphore);
		swapchainFrame->CurrentFence = GetCurrentFrame().Fence;
		swapchainFrame->PresentSemaphore = GetCurrentFrame().Semaphores.PresentSemaphore;
		swapchainFrame->RenderSemaphore = GetCurrentFrame().Semaphores.RenderSemaphore;

		//iterate over the different command buffers (if there are multiple ones)
		swapchainFrame->Commands.push_back(GetCurrentFrame().Commands.Buffer);
	}

	void VulkanRenderer::Draw(const std::vector<Renderable>& drawList)
	{
		PX_PROFILE_FUNCTION();

		uint32_t frameIndex = m_CurrentFrame % m_Specification.MaxFramesInFlight;
		uint32_t uniformOffset = PadUniformBuffer(sizeof(SceneUniformBufferD), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;
		// Bind command for descriptor-sets |needs (bindpoint), pipeline-layout, firstSet, setCount, setArray, dynOffsetCount, dynOffsets
		vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 0, 1, &GetCurrentFrame().GlobalDescriptorSet, 1, &uniformOffset);	
		
		std::vector<VkBuffer> vertexBuffers;
		vertexBuffers.resize(drawList.size());
		std::vector<VkBuffer> indexBuffers;
		indexBuffers.resize(drawList.size());
		

		VkDeviceSize offsets[] = { 0 };

		for (size_t i = 0; i < drawList.size(); i++)
		{
			vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &(std::dynamic_pointer_cast<VulkanBuffer>(drawList[i].MeshData.VertexBuffer)->GetAllocation().Buffer), offsets);		// Bind command for vertex buffer	|needs (first binding, binding count), buffer array and offset
			vkCmdBindIndexBuffer(m_ActiveCommandBuffer, std::dynamic_pointer_cast<VulkanBuffer>(drawList[i].MeshData.IndexBuffer)->GetAllocation().Buffer, 0, VK_INDEX_TYPE_UINT32);	// Bind command for index buffer	|needs indexbuffer, offset, type		


			vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &m_Frames[m_CurrentFrame].ObjectDescriptorSet, 0, nullptr);
		}

		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		float x = 0.0f + time;
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(x, 0.0f, 1.0f));
		//PushConstants pushConstant;
		//pushConstant.ModelMatrix = model;
		//vkCmdPushConstants(cmd, m_GraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstant);

		//mapping of model data into the SSBO object buffer
		/* Instead of using memcpy here, we are doing a different trick. It is possible to cast the void* from mapping the buffer into another type, and write into it normally.
		 * This will work completely fine, and makes it easier to write complex types into a buffer.
		 **/
		void* objData;
		vmaMapMemory(VulkanContext::GetAllocator(), m_Frames[m_CurrentFrame].ObjectBuffer.Allocation, &objData);
		GPUBufferObject* objectSSBO = (GPUBufferObject*)objData;
		int count = 5;
		for (int i = 0; i < count; i++)
		{
			objectSSBO[i].ModelMatrix = model;
		}
		vmaUnmapMemory(VulkanContext::GetAllocator(), m_Frames[m_CurrentFrame].ObjectBuffer.Allocation);
		vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &m_Frames[m_CurrentFrame].ObjectDescriptorSet, 0, nullptr);
		//the pipeline layout is later part of the obj material


		//CMD End

		//CopyOffscreenToViewportImage(m_OffscreenFramebuffers[frameIndex].GetColorAttachment(0).Image.Image);


		//ImGui
		PX_CORE_WARN("Begin ImGUi");
		VkCommandBuffer imGuicmd = m_ImGui->BeginRender(m_CurrentImageIndex, { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Width });
		m_ImGui->RenderDrawData(imGuicmd);
		m_ImGui->EndRender(imGuicmd);
		PX_CORE_WARN("End ImGUi");
	}

	void VulkanRenderer::EndFrame()
	{
		m_CurrentFrame = (m_CurrentFrame++) % m_Specification.MaxFramesInFlight;
		m_DebugInfo.TotalFrames++;
		PX_CORE_WARN("Current Frame: '{0}'", m_CurrentFrame);
		PX_CORE_WARN("Total Frames: '{0}'", m_DebugInfo.TotalFrames);
	}

	void VulkanRenderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		OnResize(width, height);
	}

	void VulkanRenderer::UpdateCamera(Ref<Buffer> cameraUniformBuffer)
	{
		Ref<VulkanBuffer> cameraBuffer = std::dynamic_pointer_cast<VulkanBuffer>(cameraUniformBuffer);

		CameraUniformBuffer ubo;
		ubo.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ProjectionMatrix = glm::perspective(glm::radians(45.0f), m_Swapchain->GetProperties().Width / (float)m_Swapchain->GetProperties().Height, 0.1f, 10.0f);
		ubo.ProjectionMatrix[1][1] *= -1; //flips Y
		ubo.ViewProjMatrix = ubo.ProjectionMatrix * ubo.ViewMatrix;

		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), cameraBuffer->GetAllocation().Allocation, &data);
		memcpy(data, &ubo, sizeof(CameraUniformBuffer));
		vmaUnmapMemory(VulkanContext::GetAllocator(), cameraBuffer->GetAllocation().Allocation);

		m_SceneParameter.AmbientColor = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f);

		char* sceneData;
		vmaMapMemory(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Allocation, (void**)&sceneData);
		//uint32_t frameIndex = m_CurrentFrame % Application::Get()->GetSpecification().MaxFramesInFlight;
		sceneData += PadUniformBuffer(sizeof(SceneUniformBufferD), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
		memcpy(sceneData, &m_SceneParameter, sizeof(SceneUniformBufferD));
		vmaUnmapMemory(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Allocation);
	}
	size_t VulkanRenderer::PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment)
	{
		size_t alignedSize = inputSize;
		if (minGPUBufferOffsetAlignment > 0)
		{
			alignedSize = (alignedSize + minGPUBufferOffsetAlignment - 1) & ~(minGPUBufferOffsetAlignment - 1);
		}
		return alignedSize;
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		PX_CORE_WARN("Start swapchain recreation!");
			

		vkDeviceWaitIdle(m_Device);

		VkExtent2D extent{ Application::Get()->GetWindow().GetSwapchain()->GetProperties().Width, Application::Get()->GetWindow().GetSwapchain()->GetProperties().Height };
		m_ImGui->OnSwapchainRecreate(Application::Get()->GetWindow().GetSwapchain()->GetImageViews(), extent);
	}


	FrameData& VulkanRenderer::GetFrame(uint32_t index)
	{
		PX_CORE_ASSERT(index < m_Frames.size(), "Index too big!");
		return m_Frames[index];
	}

	void VulkanRenderer::InitCommands()
	{
		m_CommandControl = CreateScope<VulkanCommandControl>();

		m_UploadContext = m_CommandControl->CreateUploadContext();

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_UploadContext->Fence), VK_SUCCESS, "Failed to create upload fence!");

	}

	void VulkanRenderer::InitFrameData()
	{
		uint32_t maxFrames = m_Specification.MaxFramesInFlight;
		if (m_Frames.size() != maxFrames)
		{
			m_Frames.resize(maxFrames);

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

		//Commands
			VkCommandPoolCreateInfo poolci{};
			poolci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolci.pNext = nullptr;
			poolci.queueFamilyIndex = VulkanContext::GetDevice()->FindQueueFamilies().GraphicsFamily.value();
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
		}
	}

	void VulkanRenderer::InitImGui()
	{
		m_ImGui->Init(Application::Get()->GetWindow().GetSwapchain()->GetImageViews(), Application::Get()->GetWindow().GetSwapchain()->GetProperties().Width, 
																						Application::Get()->GetWindow().GetSwapchain()->GetProperties().Height);
	}
	void VulkanRenderer::BeginImGuiFrame()
	{
		m_ImGui->BeginFrame();
	}
	void VulkanRenderer::EndImGuiFrame()
	{
		m_ImGui->EndFrame();
	}

	void VulkanRenderer::BeginCommandBuffer(const void* commBuf)
	{
		VkCommandBuffer cmd = (VkCommandBuffer)commBuf; //TODO: check same as the currently set commandbuffer inside the current swapchain frame ????? 
		m_ActiveCommandBuffer = cmd;
		VkCommandBufferBeginInfo cmdBegin{};
		cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBegin.pNext = nullptr;
		cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");
	}

	void VulkanRenderer::EndCommandBuffer()
	{
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(m_ActiveCommandBuffer), VK_SUCCESS, "Failed to record graphics command buffer!");
		m_ActiveCommandBuffer = VK_NULL_HANDLE;
	}

	void VulkanRenderer::BeginRenderPass(Ref<RenderPass> renderPass)
	{
		m_ActiveRenderPass = std::dynamic_pointer_cast<VulkanRenderPass>(renderPass);
		Ref<VulkanFramebuffer> fb = std::dynamic_pointer_cast<VulkanFramebuffer>(m_ActiveRenderPass->GetSpecification().TargetFramebuffer);


		//TODO: Get from the fb
		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext = nullptr;
		info.renderPass = m_ActiveRenderPass->GetVulkanObj();
		info.framebuffer = fb->GetVulkanObj();
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height };

		info.clearValueCount = static_cast<uint32_t>(clearColor.size());
		info.pClearValues = clearColor.data();

		vkCmdBeginRenderPass(m_ActiveCommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_ActiveCommandBuffer);
		m_ActiveRenderPass = nullptr;
	}

	void VulkanRenderer::BindPipeline(Ref<Pipeline> pipeline)
	{
		m_ActivePipeline = std::dynamic_pointer_cast<VulkanPipeline>(pipeline);

		if (pipeline->GetSpecification().DynamicViewAndScissors)
		{
			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height };

			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)m_ActiveRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Width;
			viewport.height = (float)m_ActiveRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Height;

			vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);
			vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);
		}

		vkCmdBindPipeline(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetVulkanObj());
	}

	void VulkanRenderer::Submit(const Renderable& object)
	{

	}

}
