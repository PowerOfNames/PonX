#include "pxpch.h"
#include "VulkanContext.h"

#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanCommands.h"
#include "VulkanInitializers.h"
#include "VulkanRendererAPI.h"


#include "vulkan/vulkan.h"
#pragma warning(push, 0)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma warning(pop)




#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {


	VulkanContext::VulkanContext()
	{
		PX_CORE_TRACE("VulkanContext created!");
	}

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		PX_CORE_TRACE("VulkanContext created with window!");
		PX_CORE_ASSERT(windowHandle, "Window handle is null");

		//glfwSetWindowUserPointer(windowHandle, this);
		//glfwSetFramebufferSizeCallback(windowHandle, FramebufferResizeCallback);
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();

		VulkanRendererAPI::SetContext(this);

		CreateInstance();
		if(PX_ENABLE_VK_VALIDATION_LAYERS)
			SetupDebugMessenger();
		CreateSurface();
		// Device picking and creation -> happens automatically
		VulkanDevice::PickPhysicalDevice(m_CoreObjects, m_DeviceExtensions);
		m_PhysicalDeviceProperties = VulkanDevice::GetPhysicalDeviceProperties(m_CoreObjects.PhysicalDevice);
		VulkanDevice::CreateLogicalDevice(m_CoreObjects, m_DeviceExtensions, m_ValidationLayers);
		PX_CORE_WARN("Finished Core!");

		// Swapchain creation -> needs window size either automatically or maybe by setting values inside the vulkan renderer
		//TODO: refactor
		m_Swapchain = CreateRef<VulkanSwapchain>();
		glfwGetFramebufferSize(m_WindowHandle, &m_WindowWidth, &m_WindowHeight);
		m_Swapchain->Create(m_CoreObjects, VulkanDevice::QuerySwapchainSupport(m_CoreObjects), m_WindowWidth, m_WindowHeight);
		m_Swapchain->CreateImagesAndViews(m_CoreObjects.Device);
		PX_CORE_WARN("Finished Swapchain!");

		CreateSyncObjects();
		PX_CORE_WARN("Finished sync!");

		// descriptorpools and sets are dependent on the set layout which will be dynamic dependend on the use case of the vulkan renderer (atm its just the ubo and texture sampler)
		CreateMemAllocator();
		PX_CORE_WARN("Finished MemAllocator!");

		InitCommands();
		PX_CORE_WARN("Finished Commands!");

		// Renderpass creation -> automatically after swapchain creation
		InitDefaultRenderPass();
		InitOffscreenRenderPass();
		PX_CORE_WARN("Finished Renderpass!");

		CreateViewportImagesAndViews();

		CreateTextureSampler();
		PX_CORE_WARN("Finished Sampler!");

		LoadTextures();
		PX_CORE_WARN("Finished loading textures!");

		InitDescriptors();
		PX_CORE_WARN("Finished descriptors!");

		// default pipeline automatic, need to further look up when the pipeline changes inside the app during runtime
		InitPipelines();
		PX_CORE_WARN("Finished pipeline!");

		CreateDepthResources();
		// default initially, but can change, framebuffer main bridge to after effects stuff
		CreatePresentFramebuffers();
		CreateOffscreenFramebuffers();
		PX_CORE_WARN("Finished fbs!");

		LoadMeshes();
		PX_CORE_WARN("Finished loading meshes!");

		m_ImGui = CreateScope<VulkanImGui>(&m_CoreObjects, &m_UploadContext, m_Swapchain->GetImageFormat(), (uint8_t)MAX_FRAMES_IN_FLIGHT);
		
		
		
		PX_CORE_TRACE("VulkanContext initialization finished!");
	}

	void VulkanContext::RecreateSwapchain()
	{
		PX_CORE_WARN("Start swapchain recreation!");

		int width = 0, height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_CoreObjects.Device);

		CleanupSwapchain();
		m_Swapchain->Create(m_CoreObjects, VulkanDevice::QuerySwapchainSupport(m_CoreObjects), width, height);
		m_Swapchain->CreateImagesAndViews(m_CoreObjects.Device);
		
		InitDefaultRenderPass();
		CreateDepthResources();
		CreatePresentFramebuffers();

		CreateViewportImagesAndViews();

		InitOffscreenRenderPass();
		for (auto& framebuffer : m_OffscreenFramebuffers)
		{
			framebuffer.Resize(m_OffscreenRenderPass, width, height);
		}
		m_ImGui->OnSwapchainRecreate(m_Swapchain->GetImageViews(), m_Swapchain->GetExtent2D());

		InitPipelines();
		CreateDepthResources();
		InitDescriptors();
		PX_CORE_WARN("Finished swapchain recreation!");
	}

	void VulkanContext::CleanupSwapchain()
	{
		vkDeviceWaitIdle(m_CoreObjects.Device);
		PX_CORE_TRACE("Cleanup swapchain begin!");

		vkDestroyImageView(m_CoreObjects.Device, m_DepthImageView, nullptr);
		vmaDestroyImage(m_CoreObjects.Allocator, m_DepthImage.Image, m_DepthImage.Allocation);
		
		vkDestroyImageView(m_CoreObjects.Device, m_ViewportImageView, nullptr);
		vmaDestroyImage(m_CoreObjects.Allocator, m_ViewportImage.Image, m_ViewportImage.Allocation);


		for (auto& framebuffer : m_SwapchainFramebuffers)
		{
			vkDestroyFramebuffer(m_CoreObjects.Device, framebuffer, nullptr);
		}
		m_ImGui->DestroyFramebuffers();
		/*for (int i = 0; i <= MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkFreeCommandBuffers(m_CoreObjects.Device, m_Frames[i].CommandPoolGfx, 1, &m_Frames[i].MainCommandBuffer);
		}*/
		vkDestroyPipeline(m_CoreObjects.Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_CoreObjects.Device, m_GraphicsPipelineLayout, nullptr);
		m_ImGui->DestroyRenderPass();
		vkDestroyRenderPass(m_CoreObjects.Device, m_DefaultRenderPass, nullptr);
		vkDestroyRenderPass(m_CoreObjects.Device, m_OffscreenRenderPass, nullptr);

		for (auto& imageView : m_Swapchain->GetImageViews())
		{
			vkDestroyImageView(m_CoreObjects.Device, imageView, nullptr);
		}
		for (auto& framebuffer : m_OffscreenFramebuffers)
		{
			framebuffer.Destroy();
		}

		m_Swapchain->Destroy(m_CoreObjects.Device);
		PX_CORE_TRACE("Cleanup swapchain finshed!");
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		CleanupSwapchain();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vmaDestroyBuffer(m_CoreObjects.Allocator, m_Frames[i].CamUniformBuffer.Buffer, m_Frames[i].CamUniformBuffer.Allocation);
		}
		vmaDestroyBuffer(m_CoreObjects.Allocator, m_SceneParameterBuffer.Buffer, m_SceneParameterBuffer.Allocation);
		vkDestroyDescriptorPool(m_CoreObjects.Device, m_DescriptorPool, nullptr);
		m_ImGui->Destroy();

		vkDestroySampler(m_CoreObjects.Device, m_TextureSampler, nullptr);

		for (auto& [name, texture] : m_Textures)
		{
			vkDestroyImageView(m_CoreObjects.Device, texture.ImageView, nullptr);
			vmaDestroyImage(m_CoreObjects.Allocator, texture.Image.Image, texture.Image.Allocation);
		}

		vkDestroyDescriptorSetLayout(m_CoreObjects.Device, m_GlobalDescriptorSetLayout, nullptr);
		vkDestroyFence(m_CoreObjects.Device, m_UploadContext.Fence, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_CoreObjects.Device, m_Frames[i].PresentSemaphore, nullptr);
			vkDestroySemaphore(m_CoreObjects.Device, m_Frames[i].RenderSemaphore, nullptr);
			vkDestroyFence(m_CoreObjects.Device, m_Frames[i].RenderFence, nullptr);

			//vkDestroyCommandPool(m_CoreObjects.Device, m_Frames[i].CommandPoolTrsf, nullptr);
			vkDestroyCommandPool(m_CoreObjects.Device, m_Frames[i].CommandPoolGfx, nullptr);
		}
		vkDestroyCommandPool(m_CoreObjects.Device, m_UploadContext.CmdPoolGfx, nullptr);
		vkDestroyCommandPool(m_CoreObjects.Device, m_UploadContext.CmdPoolTrsf, nullptr);
		m_ImGui->DestroyCommands();

		vkDestroyDevice(m_CoreObjects.Device, nullptr);

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_CoreObjects.Instance, m_DebugMessenger, nullptr);
		vkDestroySurfaceKHR(m_CoreObjects.Instance, m_CoreObjects.Surface, nullptr);
		vkDestroyInstance(m_CoreObjects.Instance, nullptr);

		//glfw termination and window deletion at last
	}

	void VulkanContext::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_WARN("Start swap buffers!");
		vkWaitForFences(m_CoreObjects.Device, 1, &GetCurrentFrame().RenderFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_CoreObjects.Device, 1, &GetCurrentFrame().RenderFence);

		vkResetCommandBuffer(GetCurrentFrame().MainCommandBuffer, 0);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_CoreObjects.Device, m_Swapchain->Get(), UINT64_MAX /*disables timeout*/, GetCurrentFrame().PresentSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}
		else
		{
			PX_CORE_ASSERT(!((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)), "Failed to acquire swap chain image!");
		}

		PX_CORE_WARN("Start update uniform buffer!");
		UpdateUniformBuffer();			// Update function
		PX_CORE_WARN("Finished update uniform buffer!");


		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
		VkCommandBufferBeginInfo cmdBegin = VulkanInits::BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");

		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };																			// clearcolor (framebuffer)
		clearColor[1].depthStencil = { 1.0f, 0 };

		if (m_GuiPipelineEnabled)
		{
			VkRenderPassBeginInfo renderPassBegin = VulkanInits::BeginRenderPass(m_OffscreenRenderPass, m_Swapchain->GetExtent2D(), m_OffscreenFramebuffers[imageIndex].Get());
			renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
			renderPassBegin.pClearValues = clearColor.data();
			vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		}
		else
		{
			
		}

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);																	// Bind command for pipeline		|needs (bind point (graphisc here)), pipeline

		int frameIndex = m_CurrentFrame % MAX_FRAMES_IN_FLIGHT;
		uint32_t uniformOffset = PadUniformBuffer(sizeof(SceneUniformBufferD), m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment) * frameIndex;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineLayout, 0, 1, &GetCurrentFrame().GlobalDescriptorSet, 1, &uniformOffset);	// Bind command for descriptor-sets |needs (bindpoint), pipeline-layout, firstSet, setCount, setArray, dynOffsetCount, dynOffsets 		

		VkBuffer vertexBuffers[] = { m_DoubleTriangleMesh.VertexBuffer.Buffer };																			// VertexBuffer needed
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);																							// Bind command for vertex buffer	|needs (first binding, binding count), buffer array and offset
		vkCmdBindIndexBuffer(cmd, m_DoubleTriangleMesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);																			// Bind command for index buffer	|needs indexbuffer, offset, type		

		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		float x = 0.0 + time;
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(x, 0.0f, 1.0f));
		PushConstants pushConstant;
		pushConstant.ModelMatrix = model;
		vkCmdPushConstants(cmd, m_GraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstant);

		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_DoubleTriangleMesh.Indices.size()), 1, 0, 0, 0);				// Draw indexed command		|needs index count, instance count, firstIndex, vertex offset, first instance 
		vkCmdEndRenderPass(cmd); // End Render Pass
		
		VkRenderPassBeginInfo renderPassBegin = VulkanInits::BeginRenderPass(m_DefaultRenderPass, m_Swapchain->GetExtent2D(), m_SwapchainFramebuffers[imageIndex]);
		renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
		renderPassBegin.pClearValues = clearColor.data();
		vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(cmd); // End Render Pass

		PX_CORE_VK_ASSERT(vkEndCommandBuffer(cmd), VK_SUCCESS, "Failed to record graphics command buffer!");

		CopyOffscreenToViewportImage(m_OffscreenFramebuffers[frameIndex].GetColorAttachment(0).Image.Image);

		//ImGui
		PX_CORE_WARN("Begin ImGUi");
		VkCommandBuffer imGuicmd = m_ImGui->BeginRender(imageIndex, m_Swapchain->GetExtent2D());
		m_ImGui->RenderDrawData(imGuicmd);
		m_ImGui->EndRender(imGuicmd);
		PX_CORE_WARN("End ImGUi");


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		std::array<VkCommandBuffer, 2> cmds{ cmd, imGuicmd };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.pWaitSemaphores = &GetCurrentFrame().PresentSemaphore;
		submitInfo.commandBufferCount = cmds.size();
		submitInfo.pCommandBuffers = cmds.data();		// command buffer usage

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &GetCurrentFrame().RenderSemaphore;

		PX_CORE_WARN("Start Begin Submit");
		PX_CORE_VK_ASSERT(vkQueueSubmit(m_CoreObjects.QueueFamily.GraphicsQueue, 1, &submitInfo, GetCurrentFrame().RenderFence), VK_SUCCESS, "Failed to submit draw render buffer!");
		PX_CORE_WARN("Start End Submit");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		VkSwapchainKHR swapchains[] = { m_Swapchain->Get() };
		presentInfo.pSwapchains = swapchains;

		presentInfo.pWaitSemaphores = &GetCurrentFrame().RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;		// optional

		result = vkQueuePresentKHR(m_CoreObjects.QueueFamily.PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
		{
			m_FramebufferResized = false;
			RecreateSwapchain();
		}
		else
		{
			PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");
		}

		m_CurrentFrame++;
		PX_CORE_WARN("Current Frame: '{0}'", m_CurrentFrame);
		PX_CORE_WARN("Current Frame: '{0}'", m_CurrentFrame % MAX_FRAMES_IN_FLIGHT);
		PX_CORE_WARN("Finished swap buffers!");
	}

	void VulkanContext::AddImGuiImage(float width, float height)
	{
		//later change to "First frame presented, or something"
		//ImGui is dependend on the first frame being rendered (cause of viewport)
		if (m_CurrentFrame > 0)
		{
			if (m_PresentImGui == nullptr || m_FramebufferResized)
			{
				vkFreeDescriptorSets(m_CoreObjects.Device, m_ImGui->GetDescriptorPool(), 1, &m_PresentImGuiSet);
				m_PresentImGuiSet = ImGui_VulkanImpl_AddTexture(m_TextureSampler, m_ViewportImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
			ImGui::Image((ImTextureID)m_PresentImGuiSet, { width, height }); //TODO: -> move to Example
		}
		PX_CORE_TRACE("Current frame: {0}", m_CurrentFrame);
	}


	void VulkanContext::CreateViewportImagesAndViews()
	{
		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(m_Swapchain->GetExtent2D().width);
		extent.height = static_cast<uint32_t>(m_Swapchain->GetExtent2D().height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(m_Swapchain->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		m_ViewportImage = VulkanImage::Create(m_CoreObjects, imageInfo, VMA_MEMORY_USAGE_CPU_ONLY);

		VkImageViewCreateInfo ivInfo = VulkanInits::CreateImageViewInfo(m_Swapchain->GetImageFormat(), m_ViewportImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(m_CoreObjects.Device, &ivInfo, nullptr, &m_ViewportImageView);
	}

	void VulkanContext::CopyOffscreenToViewportImage(VkImage& srcImage)
	{
		//Transitioning
		VulkanCommands::ImmidiateSubmitGfx(m_CoreObjects, m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_ViewportImage.Image;
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
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		VulkanCommands::ImmidiateSubmitGfx(m_CoreObjects, m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = srcImage;
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
		VulkanCommands::ImmidiateSubmitTrsf(m_CoreObjects, m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageCopy imageCopyRegion{};
				imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.srcSubresource.layerCount = 1;
				imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.dstSubresource.layerCount = 1;
				imageCopyRegion.extent.width = m_Swapchain->GetExtent2D().width;
				imageCopyRegion.extent.height = m_Swapchain->GetExtent2D().height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_ViewportImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
			});
		//Transitioning
		VulkanCommands::ImmidiateSubmitGfx(m_CoreObjects, m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_ViewportImage.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
	}

	void VulkanContext::UpdateUniformBuffer()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		CameraUniformBuffer ubo;
		ubo.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ProjectionMatrix = glm::perspective(glm::radians(45.0f), m_Swapchain->GetExtent2D().width / (float)m_Swapchain->GetExtent2D().height, 0.1f, 10.0f);
		ubo.ProjectionMatrix[1][1] *= -1; //flips Y
		ubo.ViewProjMatrix = ubo.ProjectionMatrix * ubo.ViewMatrix;

		void* data;
		vmaMapMemory(m_CoreObjects.Allocator, GetCurrentFrame().CamUniformBuffer.Allocation, &data);
		memcpy(data, &ubo, sizeof(CameraUniformBuffer));
		vmaUnmapMemory(m_CoreObjects.Allocator, GetCurrentFrame().CamUniformBuffer.Allocation);

		m_SceneParameter.AmbientColor = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f);

		char* sceneData;
		vmaMapMemory(m_CoreObjects.Allocator, m_SceneParameterBuffer.Allocation, (void**)&sceneData);
		int frameIndex = m_CurrentFrame % MAX_FRAMES_IN_FLIGHT;
		sceneData += PadUniformBuffer(sizeof(SceneUniformBufferD), m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
		memcpy(sceneData, &m_SceneParameter, sizeof(SceneUniformBufferD));
		vmaUnmapMemory(m_CoreObjects.Allocator, m_SceneParameterBuffer.Allocation);
	}
	size_t VulkanContext::PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment)
	{
		size_t alignedSize = inputSize;
		if (minGPUBufferOffsetAlignment > 0)
		{
			alignedSize = (alignedSize + minGPUBufferOffsetAlignment - 1) & ~(minGPUBufferOffsetAlignment - 1);
		}
		return alignedSize;
	}

	// ImGui
	void VulkanContext::InitImGui()
	{
		m_ImGui->Init(m_Swapchain->GetImageViews(), m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height);
	}
	void VulkanContext::BeginImGuiFrame()
	{
		m_ImGui->BeginFrame();
	}
	void VulkanContext::EndImGuiFrame()
	{
		m_ImGui->EndFrame();
	}

	// Instance
	void VulkanContext::CreateInstance()
	{
		if (PX_ENABLE_VK_VALIDATION_LAYERS && !CheckValidationLayerSupport())
		{
			PX_CORE_WARN("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Povox Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Povox";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		PX_CORE_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_CoreObjects.Instance), VK_SUCCESS, "Failed to create Vulkan Instance");
	}
	// Surface
	void VulkanContext::CreateSurface()
	{
		PX_CORE_VK_ASSERT(glfwCreateWindowSurface(m_CoreObjects.Instance, m_WindowHandle, nullptr, &m_CoreObjects.Surface), VK_SUCCESS, "Failed to create window surface!");
	}
	// Memory Allocator
	void VulkanContext::CreateMemAllocator()
	{
		VmaAllocatorCreateInfo vmaAllocatorInfo = {};
		vmaAllocatorInfo.physicalDevice = m_CoreObjects.PhysicalDevice;
		vmaAllocatorInfo.device = m_CoreObjects.Device;
		vmaAllocatorInfo.instance = m_CoreObjects.Instance;
		vmaCreateAllocator(&vmaAllocatorInfo, &m_CoreObjects.Allocator);
	}

	void VulkanContext::InitDefaultRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Swapchain->GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;		// format of swapchain images
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// load and store ops determine what to do with the data before rendering (apply to depth data)
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// we want the image to be ready for presentation in the swapchain, so final is present layout
		// Layout is importent to know, because the next operation the image is involved in needs that
		// initial is before render pass, final is the layout the imiga is automatically transitioned to after the render pass finishes

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VulkanRenderPass::Attachment color{};
		color.Description = colorAttachment;
		color.Reference = colorRef;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtils::FindDepthFormat(m_CoreObjects.PhysicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VulkanRenderPass::Attachment depth{};
		depth.Description = depthAttachment;
		depth.Reference = depthRef;

		VkSubpassDependency dependency{}; // dependencies between subpasses
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// refers to before or after the render pass depending on it being specified on src or dst
		dependency.dstSubpass = 0;						// index of subpass, dstSubpass must always be higher then src subpass unless one of them is VK_SUBPASS_EXTERNAL
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // operation to wait on
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::vector<VulkanRenderPass::Attachment> attachments{ color, depth };
		std::vector<VkSubpassDependency> dependiencies{ dependency };
		m_DefaultRenderPass = VulkanRenderPass::CreateColorAndDepth(m_CoreObjects, attachments, dependiencies);
	}

	void VulkanContext::InitOffscreenRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Swapchain->GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtils::FindDepthFormat(m_CoreObjects.PhysicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VulkanRenderPass::Attachment color{};
		color.Description = colorAttachment;
		color.Reference = colorRef;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VulkanRenderPass::Attachment depth{};
		depth.Description = depthAttachment;
		depth.Reference = depthRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		std::vector<VulkanRenderPass::Attachment> attachments{ color, depth };
		std::vector<VkSubpassDependency> dependiencies{ dependency };
		m_OffscreenRenderPass = VulkanRenderPass::CreateColorAndDepth(m_CoreObjects, attachments, dependiencies);
	}

	void VulkanContext::LoadMeshes()
	{
		const std::vector<VertexData> vertices = {
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
		};
		m_DoubleTriangleMesh.Vertices = vertices;

		const std::vector<uint16_t> indices = {
			 0, 1, 2, 2, 3, 0
		};
		m_DoubleTriangleMesh.Indices = indices;

		UploadMeshes();
	}
	void VulkanContext::UploadMeshes()
	{
		VulkanVertexBuffer::Create(m_CoreObjects, m_UploadContext, m_DoubleTriangleMesh);
		VulkanIndexBuffer::Create(m_CoreObjects, m_UploadContext, m_DoubleTriangleMesh);
	}

	void VulkanContext::LoadTextures()
	{
		Texture logo;
		logo.Image = VulkanImage::LoadFromFile(m_CoreObjects, m_UploadContext, "assets/textures/newLogo.png", VK_FORMAT_R8G8B8A8_SRGB);

		VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(VK_FORMAT_R8G8B8A8_SRGB, logo.Image.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		PX_CORE_VK_ASSERT(vkCreateImageView(m_CoreObjects.Device, &imageInfo, nullptr, &logo.ImageView), VK_SUCCESS, "Failed to create image view!");

		m_Textures["Logo"] = logo;
	}

	void VulkanContext::InitCommands()
	{
		VkCommandPoolCreateInfo commandPoolInfoGfx = VulkanInits::CreateCommandPoolInfo(m_CoreObjects.QueueFamilyIndices.GraphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VkCommandPoolCreateInfo commandPoolInfoTrsf = VulkanInits::CreateCommandPoolInfo(m_CoreObjects.QueueFamilyIndices.TransferFamily.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_Frames[i].CommandPoolGfx = VulkanCommandPool::Create(m_CoreObjects, commandPoolInfoGfx);
			//m_Frames[i].CommandPoolTrsf = VulkanCommandPool::Create(m_CoreObjects, commandPoolInfoTrsf);

			m_Frames[i].MainCommandBuffer = VulkanCommandBuffer::Create(m_CoreObjects.Device, m_Frames[i].CommandPoolGfx, VulkanInits::CreateCommandBufferAllocInfo(m_Frames[i].CommandPoolGfx, 1));
		}
		VkCommandPoolCreateInfo uploadPoolInfo = VulkanInits::CreateCommandPoolInfo(m_CoreObjects.QueueFamilyIndices.GraphicsFamily.value());
		m_UploadContext.CmdPoolGfx = VulkanCommandPool::Create(m_CoreObjects, uploadPoolInfo);
		VkCommandBufferAllocateInfo uploadCmdBufferGfxInfo = VulkanInits::CreateCommandBufferAllocInfo(m_UploadContext.CmdPoolGfx, 1);
		m_UploadContext.CmdBufferGfx = VulkanCommandBuffer::Create(m_CoreObjects.Device, m_UploadContext.CmdPoolGfx, uploadCmdBufferGfxInfo);

		m_UploadContext.CmdPoolTrsf = VulkanCommandPool::Create(m_CoreObjects, commandPoolInfoTrsf);
		VkCommandBufferAllocateInfo uploadCmdBufferTrsfInfo = VulkanInits::CreateCommandBufferAllocInfo(m_UploadContext.CmdPoolTrsf, 1);
		m_UploadContext.CmdBufferTrsf = VulkanCommandBuffer::Create(m_CoreObjects.Device, m_UploadContext.CmdPoolTrsf, uploadCmdBufferTrsfInfo);
	}

	void VulkanContext::InitPipelines()
	{
		VkPipelineLayoutCreateInfo layoutInfo = VulkanInits::CreatePipelineLayoutInfo();
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &m_GlobalDescriptorSetLayout;

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(PushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;
		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(m_CoreObjects.Device, &layoutInfo, nullptr, &m_GraphicsPipelineLayout), VK_SUCCESS, "Failed to create GraphicsPipelineLayout!");

		VulkanPipeline pipelineBuilder;

		pipelineBuilder.m_VertexInputStateInfo = VulkanInits::CreateVertexInputStateInfo();
		VertexInputDescription description = VertexData::GetVertexDescription();
		pipelineBuilder.m_VertexInputStateInfo.vertexBindingDescriptionCount = description.Bindings.size();
		pipelineBuilder.m_VertexInputStateInfo.pVertexBindingDescriptions = description.Bindings.data();
		pipelineBuilder.m_VertexInputStateInfo.vertexAttributeDescriptionCount = description.Attributes.size();
		pipelineBuilder.m_VertexInputStateInfo.pVertexAttributeDescriptions = description.Attributes.data();

		VkShaderModule vertexShaderModule = VulkanShader::Create(m_CoreObjects.Device, "assets/shaders/defaultVS.vert");
		VkShaderModule fragmentShaderModule = VulkanShader::Create(m_CoreObjects.Device, "assets/shaders/defaultLit.frag");
		VulkanInits::CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
		VulkanInits::CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
		pipelineBuilder.m_ShaderStageInfos.push_back(VulkanInits::CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule));
		pipelineBuilder.m_ShaderStageInfos.push_back(VulkanInits::CreateShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule));

		pipelineBuilder.m_AssemblyStateInfo = VulkanInits::CreateAssemblyStateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// Viewports and scissors
		pipelineBuilder.m_Viewport.x = 0.0f;
		pipelineBuilder.m_Viewport.y = 0.0f;
		pipelineBuilder.m_Viewport.width = (float)m_Swapchain->GetExtent2D().width;
		pipelineBuilder.m_Viewport.height = (float)m_Swapchain->GetExtent2D().height;
		pipelineBuilder.m_Viewport.minDepth = 0.0f;
		pipelineBuilder.m_Viewport.maxDepth = 1.0f;

		pipelineBuilder.m_Scissor.offset = { 0, 0 };
		pipelineBuilder.m_Scissor.extent = m_Swapchain->GetExtent2D();

		pipelineBuilder.m_RasterizationStateInfo = VulkanInits::CreateRasterizationStateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
		pipelineBuilder.m_MultisampleStateInfo = VulkanInits::CreateMultisampleStateInfo();
		pipelineBuilder.m_DepthStencilStateInfo = VulkanInits::CreateDepthStencilSTateInfo();
		pipelineBuilder.m_ColorBlendAttachmentStateInfo = VulkanInits::CreateColorBlendAttachment();
		pipelineBuilder.m_PipelineLayout = m_GraphicsPipelineLayout;

		m_GraphicsPipeline = pipelineBuilder.Create(m_CoreObjects.Device, m_DefaultRenderPass);

		vkDestroyShaderModule(m_CoreObjects.Device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_CoreObjects.Device, fragmentShaderModule, nullptr);
	}

	void VulkanContext::InitDescriptors()
	{
		m_DescriptorPool = VulkanDescriptor::CreatePool(m_CoreObjects.Device);
		PX_CORE_WARN("Created Descriptor pool!");

		VkDescriptorSetLayoutBinding camUBOBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
		VkDescriptorSetLayoutBinding sceneBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
		VkDescriptorSetLayoutBinding samplerLayoutBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

		std::vector<VkDescriptorSetLayoutBinding> bindings = { camUBOBinding , sceneBinding, samplerLayoutBinding };
		m_GlobalDescriptorSetLayout = VulkanDescriptor::CreateLayout(m_CoreObjects.Device, bindings);
		PX_CORE_WARN("Created Descriptor set layout!");

		size_t sceneParameterBuffersize = PadUniformBuffer(sizeof(SceneUniformBufferD), m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment) * MAX_FRAMES_IN_FLIGHT;
		m_SceneParameterBuffer = VulkanBuffer::Create(m_CoreObjects, sceneParameterBuffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_Frames[i].CamUniformBuffer = VulkanBuffer::Create(m_CoreObjects, sizeof(CameraUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			PX_CORE_WARN("Created Descriptor uniform buffer!");

			m_Frames[i].GlobalDescriptorSet = VulkanDescriptor::CreateSet(m_CoreObjects.Device, m_DescriptorPool, &m_GlobalDescriptorSetLayout);
			PX_CORE_WARN("Created Descriptor set!");

			VkDescriptorBufferInfo camInfo{};
			camInfo.buffer = m_Frames[i].CamUniformBuffer.Buffer;
			camInfo.offset = 0;
			camInfo.range = sizeof(CameraUniformBuffer);

			VkDescriptorBufferInfo sceneInfo{};
			sceneInfo.buffer = m_SceneParameterBuffer.Buffer;
			sceneInfo.offset = 0;
			sceneInfo.range = sizeof(SceneUniformBufferD);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_Textures["Logo"].ImageView;
			imageInfo.sampler = m_TextureSampler;


			std::array<VkWriteDescriptorSet, 3> descriptorWrites
			{
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Frames[i].GlobalDescriptorSet, &camInfo, nullptr, 0),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].GlobalDescriptorSet, &sceneInfo, nullptr, 1),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Frames[i].GlobalDescriptorSet, nullptr, &imageInfo, 2)
			};

			vkUpdateDescriptorSets(m_CoreObjects.Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

	}

	// Framebuffers
		// move to VulkanFramebuffer
	void VulkanContext::CreatePresentFramebuffers()
	{
		m_SwapchainFramebuffers.resize(m_Swapchain->GetImageViews().size());

		VkFramebufferCreateInfo framebufferInfo = VulkanInits::CreateFramebufferInfo(m_DefaultRenderPass, m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height);

		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); i++)
		{
			std::array<VkImageView, 2> attachments = { m_Swapchain->GetImageViews()[i], m_DepthImageView };
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());					// The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
			framebufferInfo.pAttachments = attachments.data();
			PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_CoreObjects.Device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}
	}

	void VulkanContext::CreateOffscreenFramebuffers()
	{
		m_OffscreenFramebuffers.resize(m_Swapchain->GetImageViews().size());
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); i++)
		{
			VulkanFramebuffer framebuffer(&m_CoreObjects, m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height);
			FramebufferAttachmentCreateInfo info{ m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height, m_Swapchain->GetImageFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT };
			framebuffer.AddAttachment(info);

			info.Format = VulkanUtils::FindDepthFormat(m_CoreObjects.PhysicalDevice);
			info.ImageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			framebuffer.AddAttachment(info);

			framebuffer.Create(m_OffscreenRenderPass);
			m_OffscreenFramebuffers[i] = std::move(framebuffer);
		}
	}

	void VulkanContext::CreateDepthResources()
	{
		VkFormat depthFormat = VulkanUtils::FindDepthFormat(m_CoreObjects.PhysicalDevice);
		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(m_Swapchain->GetExtent2D().width);
		extent.height = static_cast<uint32_t>(m_Swapchain->GetExtent2D().height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, extent);
		m_DepthImage = VulkanImage::Create(m_CoreObjects, imageInfo);

		VkImageViewCreateInfo viewInfo = VulkanInits::CreateImageViewInfo(depthFormat, m_DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);
		PX_CORE_VK_ASSERT(vkCreateImageView(m_CoreObjects.Device, &viewInfo, nullptr, &m_DepthImageView), VK_SUCCESS, "Failed to create ImageView!");

		VulkanCommands::ImmidiateSubmitGfx(m_CoreObjects, m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_DepthImage.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;

				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
	}

	void VulkanContext::CreateTextureSampler()
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
		vkGetPhysicalDeviceProperties(m_CoreObjects.PhysicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.minLod = 0.0f;

		PX_CORE_VK_ASSERT(vkCreateSampler(m_CoreObjects.Device, &samplerInfo, nullptr, &m_TextureSampler), VK_SUCCESS, "Failed to create texture sampler!");
	}

	// Semaphores
		// move to VulkanUtils or VulkanSyncObjects
	void VulkanContext::CreateSyncObjects()
	{
		//m_ImagesInFlight.resize(m_Swapchain->GetImages().size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo createSemaphoreInfo{};
		createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			bool result = false;
			if ((vkCreateSemaphore(m_CoreObjects.Device, &createSemaphoreInfo, nullptr, &m_Frames[i].PresentSemaphore) == VK_SUCCESS)
				&& (vkCreateSemaphore(m_CoreObjects.Device, &createSemaphoreInfo, nullptr, &m_Frames[i].RenderSemaphore) == VK_SUCCESS)
				&& (vkCreateFence(m_CoreObjects.Device, &createFenceInfo, nullptr, &m_Frames[i].RenderFence) == VK_SUCCESS))
				result = true;
			PX_CORE_ASSERT(result, "Failed to create synchronization objects for a frame!");
		}
		createFenceInfo.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateFence(m_CoreObjects.Device, &createFenceInfo, nullptr, &m_UploadContext.Fence), VK_SUCCESS, "Failed to create upload fence!");
	}

	//TODO: move outside and create an event?
//Framebuffer resize callback
	void VulkanContext::OnFramebufferResize(uint32_t width, uint32_t height)
	{
		//auto app = reinterpret_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
		//app->m_FramebufferResized = true;

		m_FramebufferResized = true;
		m_WindowWidth = (uint32_t)width;
		m_WindowHeight = (uint32_t)height;
	}

	// Extensions and layers
	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* vLayer : m_ValidationLayers)
		{
			//PX_CORE_INFO("Validation layer: '{0}'", vLayer);
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				//PX_CORE_INFO("available layers: '{0}'", layerProperties.layerName);

				if (strcmp(vLayer, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound) // return false at the first not found layer
				return false;
		}
		return true;
	}
	std::vector<const char*> VulkanContext::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		CheckRequiredExtensions(glfwExtensions, glfwExtensionCount);

		return extensions;
	}
	void VulkanContext::CheckRequiredExtensions(const char** glfwExtensions, uint32_t glfWExtensionsCount)
	{
		//PX_CORE_INFO("Required Extensioncount: {0}", glfWExtensionsCount);

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		for (uint32_t i = 0; i < glfWExtensionsCount; i++)
		{
			bool extensionFound = false;
			//PX_CORE_INFO("Required Extension #{0}: {1}", i, glfwExtensions[i]);
			uint32_t index = 1;
			for (const auto& extension : availableExtensions)
			{
				if (strcmp(glfwExtensions[i], extension.extensionName))
				{
					extensionFound = true;
					//PX_CORE_INFO("Extension requested and included {0}", glfwExtensions[i]);
					break;
				}
			}
			if (!extensionFound)
				PX_CORE_WARN("Extension requested but not included: {0}", glfwExtensions[i]);
		}
	}
	// Debug
	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_CoreObjects.Instance, &createInfo, nullptr, &m_DebugMessenger);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
	}
	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}
}
