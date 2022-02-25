#include "pxpch.h"
#include "VulkanImGui.h"

#include "VulkanDebug.h"
#include "VulkanCommands.h"

#include "VulkanFramebuffer.h"

#include <imgui.h>

#include <backends/imgui_impl_vulkan.h>

namespace Povox {

	VulkanImGui::VulkanImGui(VulkanCoreObjects* core, UploadContext* uploadContext, VkFormat swapchainImageFormat, uint8_t maxFrames)
		: m_Core(core), m_UploadContext(uploadContext), m_SwapchainImageFormat(swapchainImageFormat), m_MaxFramesInFlight(maxFrames)
	{
		m_FrameData.resize(maxFrames);
	}

	void VulkanImGui::Init(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height)
	{
		InitRenderPass();
		InitCommandBuffers();
		InitFrameBuffers(swapchainViews, width, height);

		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		
		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(m_Core->Device, &pool_info, nullptr, &m_DescriptorPool), VK_SUCCESS, "Failed to create ImGuiDescriptorPool");

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_Core->Instance;
		init_info.PhysicalDevice = m_Core->PhysicalDevice;
		init_info.Device = m_Core->Device;
		init_info.Queue = m_Core->QueueFamily.GraphicsQueue;
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.MinImageCount = (uint32_t)swapchainViews.size();
		init_info.ImageCount = (uint32_t)swapchainViews.size();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_Init(&init_info, m_RenderPass);
		
		VulkanCommands::ImmidiateSubmitGfx(*m_Core, *m_UploadContext, [=](VkCommandBuffer cmd)
			{
				ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});
		//VkCommandBuffer cmd = VulkanCommandBuffer::BeginSingleTimeCommands(m_Core->Device, m_UploadContext->CmdPoolGfx);
		//VulkanCommandBuffer::EndSingleTimeCommands(m_Core->Device, cmd, m_Core->QueueFamily.GraphicsQueue, m_UploadContext->CmdPoolGfx, m_UploadContext->Fence);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
	void VulkanImGui::OnSwapchainRecreate(const std::vector<VkImageView>& swapchainImageViews, VkExtent2D newExtent)
	{
		InitRenderPass();
		InitFrameBuffers(swapchainImageViews, newExtent.width, newExtent.height);
	}

	void VulkanImGui::Destroy()
	{
		vkDestroyDescriptorPool(m_Core->Device, m_DescriptorPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	}
	void VulkanImGui::DestroyRenderPass()
	{
		vkDestroyRenderPass(m_Core->Device, m_RenderPass, nullptr);
	}
	void VulkanImGui::DestroyFramebuffers()
	{
		for(size_t i = 0; i < m_Framebuffers.size(); i++)
			vkDestroyFramebuffer(m_Core->Device, m_Framebuffers[i], nullptr);
	}
	void VulkanImGui::DestroyCommands()
	{
		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			vkFreeCommandBuffers(m_Core->Device, m_FrameData[i].CommandPool, 1, &m_FrameData[i].CommandBuffer);
			vkDestroyCommandPool(m_Core->Device, m_FrameData[i].CommandPool, nullptr);
		}
	}

	void VulkanImGui::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
	}
	void VulkanImGui::EndFrame()
	{
		ImGui::Render();
	}

	VkCommandBuffer VulkanImGui::BeginRender(uint32_t imageIndex, VkExtent2D swapchainExtent)
	{
		vkResetCommandPool(m_Core->Device, GetFrame(imageIndex).CommandPool, 0);

		VkCommandBuffer cmd = GetFrame(imageIndex).CommandBuffer;
		VkCommandBufferBeginInfo cmdBegin = VulkanInits::BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBegin), VK_SUCCESS, "Failed to begin command buffer!");

		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassBegin = VulkanInits::BeginRenderPass(m_RenderPass, swapchainExtent, m_Framebuffers[imageIndex]);
		renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
		renderPassBegin.pClearValues = clearColor.data();

		vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		return cmd;
	}

	void VulkanImGui::RenderDrawData(VkCommandBuffer cmd)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	}

	void VulkanImGui::EndRender(VkCommandBuffer cmd)
	{
		vkCmdEndRenderPass(cmd);
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(cmd), VK_SUCCESS, "Failed to record graphics command buffer!");
	}	

	VulkanImGui::FrameData& VulkanImGui::GetFrame(uint32_t index)
	{
		if (index > m_FrameData.size())
		{
			PX_CORE_ASSERT(false, "FrameDatas index out of bounds!");
		}
		return m_FrameData[index];
	}

	void VulkanImGui::InitRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference reference{};
		reference.attachment = 0;
		reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VulkanRenderPass::Attachment color{};
		color.Description = colorAttachment;
		color.Reference = reference;

		std::vector<VulkanRenderPass::Attachment> attachments{ color };

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::vector<VkSubpassDependency> dependencies{ dependency };
		m_RenderPass = VulkanRenderPass::CreateColor(*m_Core, attachments, dependencies);
	}
	void VulkanImGui::InitCommandBuffers()
	{
		VkCommandPoolCreateInfo poolInfo = VulkanInits::CreateCommandPoolInfo(m_Core->QueueFamilyIndices.GraphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (uint8_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			m_FrameData[i].CommandPool = VulkanCommandPool::Create(*m_Core, poolInfo);
			m_FrameData[i].CommandBuffer = VulkanCommandBuffer::Create(m_Core->Device, m_FrameData[i].CommandPool, VulkanInits::CreateCommandBufferAllocInfo(m_FrameData[i].CommandPool, 1));
		}
	}
	void VulkanImGui::InitFrameBuffers(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height)
	{
		m_Framebuffers.resize(swapchainViews.size());
		VkFramebufferCreateInfo info = VulkanInits::CreateFramebufferInfo(m_RenderPass, width, height);
		for (size_t i = 0; i < m_Framebuffers.size(); i++)
		{
			info.attachmentCount = 1;
			info.pAttachments = &swapchainViews[i];
			PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_Core->Device, &info, nullptr, &m_Framebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}
	}

}
