#include "pxpch.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"


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
		PX_CORE_TRACE("VulkanRenderer:: Got Device!");
		m_Swapchain = Application::Get()->GetWindow().GetSwapchain();
		PX_CORE_TRACE("VulkanRenderer:: Got Swapchain!");

		InitCommandControl();
		InitFrameData();

		m_ImGui = CreateScope<VulkanImGui>(Application::Get()->GetWindow().GetSwapchain()->GetImageFormat(), (uint8_t)m_Specification.MaxFramesInFlight);
		PX_CORE_TRACE("VulkanRenderer:: Created VKImGui!");
		
		//m_SceneObjects = new Renderable[m_Specification.MaxSceneObjects];


		
		
		PX_CORE_INFO("Finished initializing VulkanRenderer!");
	}

	void VulkanRenderer::InitCommandControl()
	{
		m_CommandControl = CreateScope<VulkanCommandControl>();

		m_UploadContext = m_CommandControl->CreateUploadContext();

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_UploadContext->Fence), VK_SUCCESS, "Failed to create upload fence!");

		PX_CORE_TRACE("VulkanRenderer::InitCommandControl Initialized Commandcontrol!");
	}

	//content should maybe get seperated out into functions that set up the frame data according to the used shaders? potentially 
	void VulkanRenderer::InitFrameData()
	{
		uint32_t maxFrames = m_Specification.MaxFramesInFlight;
		if (m_Frames.size() != maxFrames)
		{
			m_Frames.resize(maxFrames);
			PX_CORE_INFO("VulkanRenderer::InitFrameData: Started for '{0}' frames in flight!", maxFrames);

			//Semaphores
			VkSemaphoreCreateInfo createSemaphoreInfo{};
			createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for (size_t i = 0; i < maxFrames; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_Frames[i].Semaphores.RenderSemaphore), VK_SUCCESS, "Failed to create RenderSemaphore!");
				PX_CORE_VK_ASSERT(vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_Frames[i].Semaphores.PresentSemaphore), VK_SUCCESS, "Failed to create PresentSemaphore!");
			}
			PX_CORE_INFO("VulkanRenderer::InitFrameData: Created '{0}' Semaphores", maxFrames);
			//Fences
			VkFenceCreateInfo createFenceInfo{};
			createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (size_t i = 0; i < maxFrames; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_Frames[i].Fence), VK_SUCCESS, "Failed to create Fence!");
			}
			PX_CORE_INFO("VulkanRenderer::InitFrameData: Created '{0}' Fences", maxFrames);


			//Commands
			VkCommandPoolCreateInfo poolci{};
			poolci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolci.pNext = nullptr;
			poolci.queueFamilyIndex = VulkanContext::GetDevice()->FindQueueFamilies(VulkanContext::GetDevice()->GetPhysicalDevice()).GraphicsFamily.value();
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
			PX_CORE_INFO("VulkanRenderer::InitFrameData: Created '{0}' Commandpools and buffers", maxFrames);


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
			//TextureDescriptorLayoutCreation
			VkDescriptorSetLayoutBinding samplerBinding{};
			{
				samplerBinding.binding = 0;
				samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				samplerBinding.descriptorCount = 1;
				samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				samplerBinding.pImmutableSamplers = nullptr;

				std::vector<VkDescriptorSetLayoutBinding> textureBindings = { samplerBinding };

				VkDescriptorSetLayoutCreateInfo textureInfo{};
				textureInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				textureInfo.pNext = nullptr;

				textureInfo.flags = 0;
				textureInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
				textureInfo.pBindings = textureBindings.data();

				m_TextureDescriptorSetLayout = VulkanContext::GetDescriptorLayoutCache()->CreateDescriptorLayout(&textureInfo);
			}

			const size_t align = VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			const size_t sceneUniformSize = maxFrames * PadUniformBuffer(sizeof(SceneUniform), align);

			m_SceneParameterBuffer = VulkanBuffer::CreateAllocation(sceneUniformSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);


			for (uint32_t i = 0; i < maxFrames; i++)
			{
				m_Frames[i].CamUniformBuffer = VulkanBuffer::CreateAllocation(sizeof(CameraUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
				PX_CORE_WARN("Created Descriptor uniform buffer!");

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
				
				builder.Build(m_Frames[i].GlobalDescriptorSet, m_GlobalDescriptorSetLayout);
				
				
				/*VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = m_Textures["Logo"].ImageView;
				imageInfo.sampler = m_TextureSampler;
				builder.BindImage(samplerBinding, &imageInfo);*/
				
				PX_CORE_WARN("Created Descriptor set!");
			}

			//Finale Image
			ImageSpecification imageSpec{};
			imageSpec.Width = Application::Get()->GetWindow().GetSwapchain()->GetProperties().Width;
			imageSpec.Height = Application::Get()->GetWindow().GetSwapchain()->GetProperties().Height;
			imageSpec.ChannelCount = 4;
			imageSpec.Format = ImageFormat::RGBA8;
			imageSpec.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
			imageSpec.MipLevels = 1;
			imageSpec.Tiling = ImageTiling::LINEAR;
			imageSpec.Usages = { ImageUsage::SAMPLED, ImageUsage::COLOR_ATTACHMENT, ImageUsage::COPY_DST };
			for (uint32_t i = 0; i < maxFrames; i++)
			{
				m_FinalImage = CreateRef<VulkanImage2D>(imageSpec);

				PX_CORE_WARN("Created Final images!");
			}
		}
		PX_CORE_INFO("VulkanRenderer::InitFrameData: Finished!");
	}

	void VulkanRenderer::InitSamplers()
	{
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
		}
		
		
	
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

			vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Frames[i].CamUniformBuffer.Buffer, m_Frames[i].CamUniformBuffer.Allocation);
			vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Frames[i].ObjectBuffer.Buffer, m_Frames[i].ObjectBuffer.Allocation);
		}
		vkDestroyFence(m_Device, m_UploadContext->Fence, nullptr);

		vmaDestroyBuffer(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Buffer, m_SceneParameterBuffer.Allocation);

		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolGfx, nullptr);
		vkDestroyCommandPool(m_Device, m_UploadContext->CmdPoolTrsf, nullptr);

		m_ImGui->Destroy();

		//This might be happening in the LayoutCache::Cleanup function during VulkanContext::Shutdown
		//vkDestroyDescriptorSetLayout(m_Device, m_GlobalDescriptorSetLayout, nullptr);
		//vkDestroyDescriptorSetLayout(m_Device, m_TextureDescriptorSetLayout, nullptr);
		
		vkDestroySampler(m_Device, m_TextureSampler, nullptr);
	}



	bool VulkanRenderer::BeginFrame()
	{
		PX_CORE_WARN("Start swap buffers!");


		//ATTANTION: Possible, that I need to render before the clearing, maybe, possibly
		auto& currentFreeQueue = VulkanContext::GetResourceFreeQueue()[m_CurrentFrameIndex];
		for (auto& func : currentFreeQueue)
		//	func();
		//currentFreeQueue.clear();
		vkWaitForFences(m_Device, 1, &GetCurrentFrame().Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &GetCurrentFrame().Fence);


		vkResetCommandPool(m_Device, GetCurrentFrame().Commands.Pool, 0);


		m_SwapchainFrame = m_Swapchain->AcquireNextImageIndex(GetCurrentFrame().Semaphores.PresentSemaphore);
		m_CurrentSwapchainImageIndex = m_SwapchainFrame->CurrentImageIndex;
		if (!m_SwapchainFrame)
			return false;
		m_SwapchainFrame->CurrentFence = GetCurrentFrame().Fence;
		m_SwapchainFrame->PresentSemaphore = GetCurrentFrame().Semaphores.PresentSemaphore;
		m_SwapchainFrame->RenderSemaphore = GetCurrentFrame().Semaphores.RenderSemaphore;

		return true;
	}

	//void VulkanRenderer::Draw(const std::vector<Renderable>& drawList)

	void VulkanRenderer::DrawRenderable(const Renderable& renderable)
	{
		PX_PROFILE_FUNCTION();


		Ref<VulkanShader> vkShader = std::dynamic_pointer_cast<VulkanShader>(renderable.Material.Shader);

		uint32_t frameIndex = m_CurrentFrameIndex % m_Specification.MaxFramesInFlight;
		uint32_t uniformOffset = PadUniformBuffer(sizeof(SceneUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;

		vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 0, 1, &GetCurrentFrame().GlobalDescriptorSet, 1, &uniformOffset);


		VkBuffer vertexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(renderable.MeshData.VertexBuffer)->GetAllocation().Buffer;
		VkBuffer indexBuffer = std::dynamic_pointer_cast<VulkanBuffer>(renderable.MeshData.IndexBuffer)->GetAllocation().Buffer;
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(m_ActiveCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &m_Frames[m_CurrentFrame].ObjectDescriptorSet, 0, nullptr);


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
		 /*
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
		 */
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


		vkCmdDrawIndexed(m_ActiveCommandBuffer, 6, 1, 0, 0, 0);
	}

	void VulkanRenderer::Draw()
	{
		PX_PROFILE_FUNCTION();


		//PX_CORE_WARN("Started Draw!");


		//PX_CORE_WARN("Here!!");
		//uint32_t frameIndex = m_CurrentFrame % m_Specification.MaxFramesInFlight;
		//uint32_t uniformOffset = PadUniformBuffer(sizeof(SceneUniformBufferD), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * (size_t)frameIndex;
		//// Bind command for descriptor-sets |needs (bindpoint), pipeline-layout, firstSet, setCount, setArray, dynOffsetCount, dynOffsets
		//PX_CORE_WARN("Here!!");
		////Get the descriptor set from the bound shader/material
		//vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 0, 1, &GetCurrentFrame().GlobalDescriptorSet, 1, &uniformOffset);	
		//PX_CORE_WARN("Here!!");
		//
		//std::vector<VkBuffer> vertexBuffers;
		//vertexBuffers.resize(drawList.size());
		//std::vector<VkBuffer> indexBuffers;
		//indexBuffers.resize(drawList.size());
		//
		//VkDeviceSize offsets[] = { 0 };


		//for (size_t i = 0; i < drawList.size(); i++)
		//{
		//	vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &(std::dynamic_pointer_cast<VulkanBuffer>(drawList[i].MeshData.VertexBuffer)->GetAllocation().Buffer), offsets);		// Bind command for vertex buffer	|needs (first binding, binding count), buffer array and offset
		//	vkCmdBindIndexBuffer(m_ActiveCommandBuffer, std::dynamic_pointer_cast<VulkanBuffer>(drawList[i].MeshData.IndexBuffer)->GetAllocation().Buffer, 0, VK_INDEX_TYPE_UINT32);	// Bind command for index buffer	|needs indexbuffer, offset, type		


		//	vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &m_Frames[m_CurrentFrame].ObjectDescriptorSet, 0, nullptr);
		//}

		//static auto startTime = std::chrono::high_resolution_clock::now();
		//auto currentTime = std::chrono::high_resolution_clock::now();
		//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		//float x = 0.0f + time;
		//glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(x, 0.0f, 1.0f));
		////PushConstants pushConstant;
		////pushConstant.ModelMatrix = model;
		////vkCmdPushConstants(cmd, m_GraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstant);

		////mapping of model data into the SSBO object buffer
		///* Instead of using memcpy here, we are doing a different trick. It is possible to cast the void* from mapping the buffer into another type, and write into it normally.
		// * This will work completely fine, and makes it easier to write complex types into a buffer.
		// **/
		//void* objData;
		//vmaMapMemory(VulkanContext::GetAllocator(), m_Frames[m_CurrentFrame].ObjectBuffer.Allocation, &objData);
		//GPUBufferObject* objectSSBO = (GPUBufferObject*)objData;
		//int count = 5;
		//for (int i = 0; i < count; i++)
		//{
		//	objectSSBO[i].ModelMatrix = model;
		//}
		//vmaUnmapMemory(VulkanContext::GetAllocator(), m_Frames[m_CurrentFrame].ObjectBuffer.Allocation);
		//vkCmdBindDescriptorSets(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline->GetLayout(), 1, 1, &m_Frames[m_CurrentFrame].ObjectDescriptorSet, 0, nullptr);
		////the pipeline layout is later part of the obj material
	}
	void VulkanRenderer::DrawGUI()
	{
		m_ImGui->RenderDrawData();
	}


	void VulkanRenderer::EndFrame()
	{
		// Now retrieve the final image from the last renderpass and copy it into the current swapchain image?
		// Either here or in the Application/Layer -> maybe build a render flow manager

		m_CurrentFrameIndex = (m_CurrentFrameIndex++) % m_Specification.MaxFramesInFlight;
		m_DebugInfo.TotalFrames++;
		PX_CORE_WARN("Current Frame: '{0}'", m_CurrentFrameIndex);
		PX_CORE_WARN("Total Frames: '{0}'", m_DebugInfo.TotalFrames);
	}



	void VulkanRenderer::UpdateCamera(const CameraUniform& cam)
	{

		CameraUniform camOut;
		camOut.ViewMatrix = cam.ViewMatrix;
		camOut.ProjectionMatrix = cam.ProjectionMatrix;
		camOut.ProjectionMatrix[1][1] *= -1; // either this or flip the viewport
		camOut.ViewProjMatrix = camOut.ProjectionMatrix * camOut.ViewMatrix;

		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().CamUniformBuffer.Allocation, &data);
		memcpy(data, &camOut, sizeof(CameraUniform));
		vmaUnmapMemory(VulkanContext::GetAllocator(), GetCurrentFrame().CamUniformBuffer.Allocation);


		m_SceneParameter.AmbientColor = glm::vec4(0.1f, 0.2f, 1.0f, 0.5f);
		char* sceneData;
		vmaMapMemory(VulkanContext::GetAllocator(), m_SceneParameterBuffer.Allocation, (void**)&sceneData);
		sceneData += PadUniformBuffer(sizeof(SceneUniform), VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment) * m_CurrentFrameIndex;
		memcpy(sceneData, &m_SceneParameter, sizeof(SceneUniform));
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

	

	void VulkanRenderer::BeginCommandBuffer(const void* commBuf)
	{
		m_ActiveCommandBuffer = (VkCommandBuffer)commBuf;
		m_SwapchainFrame->Commands.push_back(m_ActiveCommandBuffer);
		
		VkCommandBufferBeginInfo cmdBegin{};
		cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBegin.pNext = nullptr;
		cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(m_ActiveCommandBuffer, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");
		
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
		nameInfo.objectHandle = (uint64_t)m_ActiveCommandBuffer;
		nameInfo.pObjectName = "Frame CommandBuffer Frame";
		NameVkObject(m_Device, nameInfo);		
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
		std::array<VkClearValue, 3> clearColor = {};
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearColor[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearColor[2].depthStencil = { 1.0f, 0 };

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

	void VulkanRenderer::BeginGUIRenderPass()
	{
		m_ImGui->BeginRenderPass(m_ActiveCommandBuffer, m_CurrentSwapchainImageIndex, { m_Swapchain->GetProperties().Width, m_Swapchain->GetProperties().Height });
	}
	void VulkanRenderer::EndGUIRenderPass()
	{
		m_ImGui->EndRenderPass();
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
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
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


	void VulkanRenderer::PrepareSwapchainImage(Ref<Image2D> finalImage)
	{
		Ref<VulkanImage2D> finalImageVK = std::dynamic_pointer_cast<VulkanImage2D>(finalImage);

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
			barrier.image = finalImageVK->GetImage();
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
				imageCopyRegion.extent.width = finalImageVK->GetSpecification().Width;
				imageCopyRegion.extent.height = finalImageVK->GetSpecification().Height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, finalImageVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_SwapchainFrame->CurrentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
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
			barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
	}

	void VulkanRenderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		OnResize(width, height);
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		PX_CORE_WARN("Start swapchain recreation!");


		vkDeviceWaitIdle(m_Device);

		VkExtent2D extent{ Application::Get()->GetWindow().GetSwapchain()->GetProperties().Width, Application::Get()->GetWindow().GetSwapchain()->GetProperties().Height };
		m_ImGui->OnSwapchainRecreate(Application::Get()->GetWindow().GetSwapchain()->GetImageViews(), extent);
	}

	void VulkanRenderer::CreateFinalImage(Ref<Image2D> finalImage)
	{
		Ref<VulkanImage2D> finalImageVK = std::dynamic_pointer_cast<VulkanImage2D>(finalImage);

		//Transition swapchain image
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = m_FinalImage->GetImage();
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
			barrier.image = finalImageVK->GetImage();
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
				imageCopyRegion.extent.width = finalImageVK->GetSpecification().Width;
				imageCopyRegion.extent.height = finalImageVK->GetSpecification().Height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, finalImageVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_FinalImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
			});
		//Transition swapchain image back into present
		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = m_FinalImage->GetImage();
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
	}

	FrameData& VulkanRenderer::GetFrame(uint32_t index)
	{
		PX_CORE_ASSERT(index < m_Frames.size(), "Index too big!");
		return m_Frames[index];
	}


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
	
	
	void* VulkanRenderer::GetGUIDescriptorSet(Ref<Image2D> image)
	{
		Ref<VulkanImage2D> vkImage = std::dynamic_pointer_cast<VulkanImage2D>(image);

		m_CommandControl->ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = vkImage->GetImage();
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
			barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		return m_ImGui->GetImGUIDescriptorSet(vkImage->GetImageView(), vkImage->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

}
