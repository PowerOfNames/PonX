#include "pxpch.h"
#include "VulkanContext.h"

#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanImage2D.h"
#include "VulkanCommands.h"
#include "VulkanInitializers.h"
#include "VulkanRendererAPI.h"

#include "Povox/Core/Application.h"

#include "vulkan/vulkan.h"
#pragma warning(push, 0)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma warning(pop)




#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	Ref<VulkanDevice> VulkanContext::s_Device = nullptr;
	VkInstance VulkanContext::s_Instance = nullptr;
	VmaAllocator VulkanContext::s_Allocator = nullptr;

	VulkanContext::VulkanContext()
	{
		PX_CORE_TRACE("VulkanContext created!");
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();

		VulkanRendererAPI::SetContext(this); //make context static in the future

		CreateInstance();
#ifdef PX_ENABLE_VK_VALIDATION_LAYERS
			SetupDebugMessenger();
#endif
		// Device picking and creation -> happens automatically
		s_Device = CreateRef<VulkanDevice>();
		s_Device->PickPhysicalDevice(m_DeviceExtensions);
		m_PhysicalDeviceProperties = s_Device->GetPhysicalDeviceProperties(s_Device->GetPhysicalDevice());
		s_Device->CreateLogicalDevice(m_DeviceExtensions, m_ValidationLayers);
		PX_CORE_WARN("Finished Core!");

		// descriptorpools and sets are dependent on the set layout which will be dynamic dependent on the use case of the vulkan renderer (atm its just the ubo and texture sampler)
		CreateMemAllocator();
		PX_CORE_WARN("Finished MemAllocator!");

		m_RenderPassPool = VulkanRenderPassPool();

		//set the coreObjects for all vulkan object abstractions and helper classes
		VulkanFramebuffer::Init(&m_FramebufferPool);
		//VulkanIndexBuffer::Init(&m_CoreObjects);
		//VulkanVertexBuffer::Init(&m_CoreObjects);

		InitCommands();
		PX_CORE_WARN("Finished Commands!");

		// Renderpass creation -> automatically after swapchain creation
		InitOffscreenRenderPass();
		PX_CORE_WARN("Finished Renderpass!");

		CreateViewportImagesAndViews();

		LoadTextures();
		PX_CORE_WARN("Finished loading textures!");

		InitDescriptors();
		PX_CORE_WARN("Finished descriptors!");

		// default pipeline automatic, need to further look up when the pipeline changes inside the app during runtime
		InitPipelines();
		PX_CORE_WARN("Finished pipeline!");

		CreateDepthResources();
		// default initially, but can change, framebuffer main bridge to after effects stuff
		//CreatePresentFramebuffers();
		//CreateOffscreenFramebuffers();
		PX_CORE_WARN("Finished fbs!");

		LoadMeshes();
		PX_CORE_WARN("Finished loading meshes!");

		m_ImGui = CreateScope<VulkanImGui>(&m_UploadContext, Application::Get().GetWindow().GetSwapchain().GetImageFormat(), (uint8_t)Application::GetSpecification().RendererProps.MaxFramesInFlight);
		

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

		vkDeviceWaitIdle(s_Device->GetVulkanDevice());
				
		CreateDepthResources();

		CreateViewportImagesAndViews();

		InitOffscreenRenderPass();
		for (auto& framebuffer : m_OffscreenFramebuffers)
		{
			framebuffer.Resize(width, height);
		}
		VkExtent2D extent{ Application::Get().GetWindow().GetSwapchain().GetProperties().Width, Application::Get().GetWindow().GetSwapchain().GetProperties().Height };
		m_ImGui->OnSwapchainRecreate(Application::Get().GetWindow().GetSwapchain().GetImageViews(), extent);

		InitPipelines();
		CreateDepthResources();
		InitDescriptors();
		PX_CORE_WARN("Finished swapchain recreation!");
	}

	void VulkanContext::CleanupSwapchain()
	{
		vkDeviceWaitIdle(s_Device->GetVulkanDevice());
		PX_CORE_TRACE("Cleanup swapchain begin!");

		vkDestroyImageView(s_Device->GetVulkanDevice(), m_DepthImageView, nullptr);
		vmaDestroyImage(s_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);
		
		vkDestroyImageView(s_Device->GetVulkanDevice(), m_ViewportImageView, nullptr);
		vmaDestroyImage(s_Allocator, m_ViewportImage.Image, m_ViewportImage.Allocation);

		m_ImGui->DestroyFramebuffers();
		vkDestroyPipeline(s_Device->GetVulkanDevice(), m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(s_Device->GetVulkanDevice(), m_GraphicsPipelineLayout, nullptr);
		m_ImGui->DestroyRenderPass();
		vkDestroyRenderPass(s_Device->GetVulkanDevice(), m_RenderPassPool.RemoveRenderPass("OffscreenPass"), nullptr);

		for (auto& framebuffer : m_OffscreenFramebuffers)
		{
			framebuffer.Destroy();
		}
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		CleanupSwapchain();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vmaDestroyBuffer(s_Allocator, m_Frames[i].CamUniformBuffer.Buffer, m_Frames[i].CamUniformBuffer.Allocation);
		}
		vmaDestroyBuffer(s_Allocator, m_SceneParameterBuffer.Buffer, m_SceneParameterBuffer.Allocation);
		vkDestroyDescriptorPool(s_Device->GetVulkanDevice(), m_DescriptorPool, nullptr);
		m_ImGui->Destroy();

		vkDestroySampler(s_Device->GetVulkanDevice(), m_TextureSampler, nullptr);

		for (auto& [name, texture] : m_Textures)
		{
			vkDestroyImageView(s_Device->GetVulkanDevice(), texture.ImageView, nullptr);
			vmaDestroyImage(s_Allocator, texture.Image2D.Image, texture.Image2D.Allocation);
		}

		vkDestroyDescriptorSetLayout(s_Device->GetVulkanDevice(), m_GlobalDescriptorSetLayout, nullptr);
		vkDestroyFence(s_Device->GetVulkanDevice(), m_UploadContext.Fence, nullptr);
		
		vkDestroyCommandPool(s_Device->GetVulkanDevice(), m_UploadContext.CmdPoolGfx, nullptr);
		vkDestroyCommandPool(s_Device->GetVulkanDevice(), m_UploadContext.CmdPoolTrsf, nullptr);
		m_ImGui->DestroyCommands();

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(s_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(s_Instance, nullptr);
	}


	//Swap buffers can be called by a swapchain. Same goes for:
	// AcquireNextImage -> return image index
	// QueueSubmit stuff -> needs to be researched, if I ned to collect every command buffer before and put them here. THen, It may be managed by the CommandBUfferQueue upon Execute?
	// Present -> sets up the QueuePresent stuff
	// 
	//It should exclude everything that is CommanBuffer recoding, renderpass management and material/pipeline managment, but they need to happen in th right order in the App.Run loop
	//Uniformbuffer related stuff, swapchain recreation, I will have to see if I can manage that via the layers and window
	// also, GUI (ImgUi) management
	void VulkanContext::SwapBuffers() //technically this function just does the presenting. The command recording and frame prep needs to be separated
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_WARN("Start update uniform buffer!");
		UpdateUniformBuffer();			// Update function
		PX_CORE_WARN("Finished update uniform buffer!");


		//combine the beginRender with a renderpass. Renderpass holds the framebuffer it is targeted to. this way I dont need a framebuffer pool in particular


		VkCommandBuffer cmd = GetCurrentFrameIndex().MainCommandBuffer;
		VkCommandBufferBeginInfo cmdBegin = VulkanInits::BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");

		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };			// clearcolor (framebuffer)
		clearColor[1].depthStencil = { 1.0f, 0 };

		if (Application::GetSpecification().ImGuiEnabled)
		{
			VkRenderPassBeginInfo renderPassBegin = VulkanInits::BeginRenderPass(	m_RenderPassPool.GetRenderPass("OffscreenPass"), 
																					m_Swapchain->GetExtent2D(), 
																					m_FramebufferPool.GetFramebuffer("OffscreenFB")->GetCurrent(imageIndex));
			//Framebuffer clear colors!
			renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
			renderPassBegin.pClearValues = clearColor.data();
			vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		}
		else
		{		
			//Render without GUI to default framebuffer

		}

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);																	// Bind command for pipeline		|needs (bind point (graphisc here)), pipeline

		int frameIndex = m_CurrentFrame % MAX_FRAMES_IN_FLIGHT;
		uint32_t uniformOffset = PadUniformBuffer(sizeof(SceneUniformBufferD), m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment) * frameIndex;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineLayout, 0, 1, &GetCurrentFrameIndex().GlobalDescriptorSet, 1, &uniformOffset);	// Bind command for descriptor-sets |needs (bindpoint), pipeline-layout, firstSet, setCount, setArray, dynOffsetCount, dynOffsets 		

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
		
		/*VkRenderPassBeginInfo renderPassBegin = VulkanInits::BeginRenderPass(m_RenderPassPool.GetRenderPass("DefaultPass"), m_Swapchain->GetExtent2D(), m_SwapchainFramebuffers[imageIndex]);
		renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
		renderPassBegin.pClearValues = clearColor.data();
		vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(cmd);*/ // End Render Pass

		PX_CORE_VK_ASSERT(vkEndCommandBuffer(cmd), VK_SUCCESS, "Failed to record graphics command buffer!");

		CopyOffscreenToViewportImage(m_OffscreenFramebuffers[frameIndex].GetColorAttachment(0).Image.Image);

		//ImGui
		PX_CORE_WARN("Begin ImGUi");
		VkCommandBuffer imGuicmd = m_ImGui->BeginRender(imageIndex, m_Swapchain->GetExtent2D());
		m_ImGui->RenderDrawData(imGuicmd);
		m_ImGui->EndRender(imGuicmd);
		PX_CORE_WARN("End ImGUi");		
	}


	// this is only used in the next function!
	void VulkanContext::CreateViewportImagesAndViews()
	{
		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(m_Swapchain->GetExtent2D().width);
		extent.height = static_cast<uint32_t>(m_Swapchain->GetExtent2D().height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(m_Swapchain->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		m_ViewportImage = VulkanImageDepr::Create(imageInfo, VMA_MEMORY_USAGE_CPU_ONLY);

		VkImageViewCreateInfo ivInfo = VulkanInits::CreateImageViewInfo(m_Swapchain->GetImageFormat(), m_ViewportImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(s_Device->GetVulkanDevice(), &ivInfo, nullptr, &m_ViewportImageView);
	}

	//maybe refactor this in a ways, that I can call a function in the layer, which results to me the image as Image2D or Texture2D, so I can use it. Need to make sure
	// I reuse the resource, and not create a new one every time I call this
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

	// will have to see what happens with this stuff
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
		vmaMapMemory(m_CoreObjects.Allocator, GetCurrentFrameIndex().CamUniformBuffer.Allocation, &data);
		memcpy(data, &ubo, sizeof(CameraUniformBuffer));
		vmaUnmapMemory(m_CoreObjects.Allocator, GetCurrentFrameIndex().CamUniformBuffer.Allocation);

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

	// ImGui -> questionable where this should go
	void VulkanContext::InitImGui()
	{
		m_ImGui->Init(Application::Get().GetWindow().GetSwapchain().GetImageViews(), m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height);
	}
	void VulkanContext::BeginImGuiFrame()
	{
		m_ImGui->BeginFrame();
	}
	void VulkanContext::EndImGuiFrame()
	{
		m_ImGui->EndFrame();
	}
	void VulkanContext::AddImGuiImage(float width, float height)
	{
		//later change to "First frame presented, or something"
		//ImGui is dependend on the first frame being rendered (cause of viewport)
		if (m_CurrentFrame > 0)
		{
			if (m_PresentImGui == nullptr || m_FramebufferResized)
			{
				vkFreeDescriptorSets(s_Device->GetVulkanDevice(), m_ImGui->GetDescriptorPool(), 1, &m_PresentImGuiSet);
				m_PresentImGuiSet = ImGui_VulkanImpl_AddTexture(m_TextureSampler, m_ViewportImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
			ImGui::Image((ImTextureID)m_PresentImGuiSet, { width, height }); //TODO: -> move to Example
		}
		PX_CORE_TRACE("Current frame: {0}", m_CurrentFrame);
	}


	//Core stuff -> those will get called upon init, and stored as statics, so I can call the statically from where I need them (mostly vulkan specific files)
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

		PX_CORE_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &s_Instance), VK_SUCCESS, "Failed to create Vulkan Instance");
	}	
		
	void VulkanContext::CreateMemAllocator()
	{
		VmaAllocatorCreateInfo vmaAllocatorInfo = {};
		vmaAllocatorInfo.physicalDevice = s_Device->GetPhysicalDevice();
		vmaAllocatorInfo.device = s_Device->GetVulkanDevice();
		vmaAllocatorInfo.instance = s_Instance;
		vmaCreateAllocator(&vmaAllocatorInfo, &s_Allocator);
	}

	//layer managed
	void VulkanContext::InitOffscreenRenderPass()
	{
		VulkanRenderPassBuilder builder = VulkanRenderPassBuilder::Begin();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Swapchain->GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		builder.AddAttachment(colorAttachment);


		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtils::FindDepthFormat(s_Device->GetPhysicalDevice());
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		builder.AddAttachment(depthAttachment);

		
		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::vector<VkAttachmentReference> colorRefs{ colorRef };
		builder.CreateAndAddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colorRefs, depthRef);

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		builder.AddDependency(dependency);
		m_RenderPassPool.AddRenderPass("OffscreenPass", builder.Build());		 
	}

	//external stuff, all managed by the layers via the different renderers
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
	// same as shaders, get created an stored inside the layers, and then linked via a game object or material?
	void VulkanContext::LoadTextures()
	{
		Texture logo;
		logo.Image2D = VulkanImageDepr::LoadFromFile(m_CoreObjects, m_UploadContext, "assets/textures/newLogo.png", VK_FORMAT_R8G8B8A8_SRGB);

		VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(VK_FORMAT_R8G8B8A8_SRGB, logo.Image2D.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		PX_CORE_VK_ASSERT(vkCreateImageView(s_Device->GetVulkanDevice(), &imageInfo, nullptr, &logo.ImageView), VK_SUCCESS, "Failed to create image view!");

		m_Textures["Logo"] = logo;
	}

	// one of the more tricky ones -> create CommandBuffers and RenderCommandBuffer? RenderCommandqueue which then gets executed from the rendered
	// commands get collected inside the queue
	// create pools for specific tasks? Geometry, Shading...? Or keep it as is...
	void VulkanContext::InitCommands()
	{
		VkCommandPoolCreateInfo commandPoolInfoTrsf = VulkanInits::CreateCommandPoolInfo(s_Device->FindQueueFamilies().TransferFamily.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		
		VkCommandPoolCreateInfo uploadPoolInfo = VulkanInits::CreateCommandPoolInfo(s_Device->FindQueueFamilies().GraphicsFamily.value());
		m_UploadContext.CmdPoolGfx = VulkanCommandPool::Create(m_CoreObjects, uploadPoolInfo);
		VkCommandBufferAllocateInfo uploadCmdBufferGfxInfo = VulkanInits::CreateCommandBufferAllocInfo(m_UploadContext.CmdPoolGfx, 1);
		m_UploadContext.CmdBufferGfx = VulkanCommandBuffer::Create(s_Device->GetVulkanDevice(), m_UploadContext.CmdPoolGfx, uploadCmdBufferGfxInfo);

		m_UploadContext.CmdPoolTrsf = VulkanCommandPool::Create(m_CoreObjects, commandPoolInfoTrsf);
		VkCommandBufferAllocateInfo uploadCmdBufferTrsfInfo = VulkanInits::CreateCommandBufferAllocInfo(m_UploadContext.CmdPoolTrsf, 1);
		m_UploadContext.CmdBufferTrsf = VulkanCommandBuffer::Create(s_Device->GetVulkanDevice(), m_UploadContext.CmdPoolTrsf, uploadCmdBufferTrsfInfo);
	}

	// Pipeline creation also called from the layers. References a renderpass and shaders
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
		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(s_Device->GetVulkanDevice(), &layoutInfo, nullptr, &m_GraphicsPipelineLayout), VK_SUCCESS, "Failed to create GraphicsPipelineLayout!");

		VulkanPipeline pipelineBuilder;

		pipelineBuilder.m_VertexInputStateInfo = VulkanInits::CreateVertexInputStateInfo();
		VertexInputDescription description = VertexData::GetVertexDescription();
		pipelineBuilder.m_VertexInputStateInfo.vertexBindingDescriptionCount = description.Bindings.size();
		pipelineBuilder.m_VertexInputStateInfo.pVertexBindingDescriptions = description.Bindings.data();
		pipelineBuilder.m_VertexInputStateInfo.vertexAttributeDescriptionCount = description.Attributes.size();
		pipelineBuilder.m_VertexInputStateInfo.pVertexAttributeDescriptions = description.Attributes.data();

		VkShaderModule vertexShaderModule = VulkanShader::Create(s_Device->GetVulkanDevice(), "assets/shaders/defaultVS.vert");
		VkShaderModule fragmentShaderModule = VulkanShader::Create(s_Device->GetVulkanDevice(), "assets/shaders/defaultLit.frag");
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

		m_GraphicsPipeline = pipelineBuilder.Create(s_Device->GetVulkanDevice(), m_RenderPassPool.GetRenderPass("OffscreenPass"));

		vkDestroyShaderModule(s_Device->GetVulkanDevice(), vertexShaderModule, nullptr);
		vkDestroyShaderModule(s_Device->GetVulkanDevice(), fragmentShaderModule, nullptr);
	}

	// might be completely dependent on shaders. Upon startup, I run through the shader lib, and compile every shader, which should also reflect on them and then create 
	// descriptors -> ideally chache them and check if descriptors are the same and can be reused or not
	void VulkanContext::InitDescriptors()
	{
		m_DescriptorPool = VulkanDescriptor::CreatePool(s_Device->GetVulkanDevice());
		PX_CORE_WARN("Created Descriptor pool!");

		VkDescriptorSetLayoutBinding camUBOBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
		VkDescriptorSetLayoutBinding sceneBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
		VkDescriptorSetLayoutBinding samplerLayoutBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

		std::vector<VkDescriptorSetLayoutBinding> bindings = { camUBOBinding , sceneBinding, samplerLayoutBinding };
		m_GlobalDescriptorSetLayout = VulkanDescriptor::CreateLayout(s_Device->GetVulkanDevice(), bindings);
		PX_CORE_WARN("Created Descriptor set layout!");

		size_t sceneParameterBuffersize = PadUniformBuffer(sizeof(SceneUniformBufferD), m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment) * MAX_FRAMES_IN_FLIGHT;
		m_SceneParameterBuffer = VulkanBuffer::Create(m_CoreObjects, sceneParameterBuffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_Frames[i].CamUniformBuffer = VulkanBuffer::Create(m_CoreObjects, sizeof(CameraUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			PX_CORE_WARN("Created Descriptor uniform buffer!");

			m_Frames[i].GlobalDescriptorSet = VulkanDescriptor::CreateSet(s_Device->GetVulkanDevice(), m_DescriptorPool, &m_GlobalDescriptorSetLayout);
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

			vkUpdateDescriptorSets(s_Device->GetVulkanDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

	}

	//still need to figure out a way, when to determine the count of framebuffers -> dependent on the swapchain images, frames in flight or singe FBs?
	void VulkanContext::CreateOffscreenFramebuffers()
	{
		/*FramebufferSpecification specs;
		specs.Attachements = { {FramebufferTextureFormat::RGBA8, {FramebufferTextureUsage::COLOR, FramebufferTextureUsage::TRANSFER_SRC} }, {FramebufferTextureFormat::Depth, {FramebufferTextureUsage::DEPTH_STENCIL}} };
		specs.Width = m_Swapchain->GetExtent2D().width;
		specs.Height = m_Swapchain->GetExtent2D().height;
		specs.RenderPass = "OffscreenPass";
		m_OffscreenFramebuffers.resize(m_Swapchain->GetImageViews().size());
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); i++)
		{			
			VulkanFramebuffer framebuffer(specs);
			FramebufferAttachmentCreateInfo info{ m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height, m_Swapchain->GetImageFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT };
			framebuffer.AddAttachment(info);

			info.Format = VulkanUtils::FindDepthFormat(s_Device->GetPhysicalDevice());
			info.ImageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			framebuffer.AddAttachment(info);

			framebuffer.Create();
			m_OffscreenFramebuffers[i] = std::move(framebuffer);
		}*/
	}

	void VulkanContext::CreateDepthResources()
	{
		VkFormat depthFormat = VulkanUtils::FindDepthFormat(s_Device->GetPhysicalDevice());
		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(m_Swapchain->GetExtent2D().width);
		extent.height = static_cast<uint32_t>(m_Swapchain->GetExtent2D().height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, extent);
		m_DepthImage = VulkanImageDepr::Create(imageInfo);

		VkImageViewCreateInfo viewInfo = VulkanInits::CreateImageViewInfo(depthFormat, m_DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);
		PX_CORE_VK_ASSERT(vkCreateImageView(s_Device->GetVulkanDevice(), &viewInfo, nullptr, &m_DepthImageView), VK_SUCCESS, "Failed to create ImageView!");

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

	// Upload context stuff -> should only be available and used if an exclusive TransferQueue is available
	void VulkanContext::CreateSyncObjects()
	{
		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateFence(s_Device->GetVulkanDevice(), &createFenceInfo, nullptr, &m_UploadContext.Fence), VK_SUCCESS, "Failed to create upload fence!");
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

		VkResult result = CreateDebugUtilsMessengerEXT(s_Instance, &createInfo, nullptr, &m_DebugMessenger);
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
