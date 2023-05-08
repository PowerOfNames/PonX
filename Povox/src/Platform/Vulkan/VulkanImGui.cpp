#include "pxpch.h"
#include "VulkanImGui.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanCommands.h"

#include "VulkanFramebuffer.h"

#include <imgui.h>

#include <backends/imgui_impl_vulkan.h>

namespace Povox {

	VulkanImGui::VulkanImGui(VkFormat swapchainImageFormat, uint8_t maxFrames)
		:m_SwapchainImageFormat(swapchainImageFormat), m_MaxFramesInFlight(maxFrames)
	{
		m_FrameData.resize(maxFrames);
	}

	VulkanImGui::~VulkanImGui()
	{
		Destroy();
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
		
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(device, &pool_info, nullptr, &m_DescriptorPool), VK_SUCCESS, "Failed to create ImGuiDescriptorPool");

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanContext::GetInstance();
		init_info.PhysicalDevice = VulkanContext::GetDevice()->GetPhysicalDevice();
		init_info.Device = device;
		init_info.Queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.MinImageCount = (uint32_t)swapchainViews.size();
		init_info.ImageCount = (uint32_t)swapchainViews.size();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_Init(&init_info, m_RenderPass);
		
		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd)
			{
				ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
	void VulkanImGui::OnSwapchainRecreate(const std::vector<VkImageView>& swapchainImageViews, VkExtent2D newExtent)
	{
		InitRenderPass();
		InitFrameBuffers(swapchainImageViews, newExtent.width, newExtent.height);
	}

	void VulkanImGui::Destroy()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		if (m_RenderPass)
		{
			vkDestroyRenderPass(device, m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
		}

		for (size_t i = 0; i < m_Framebuffers.size(); i++)
			vkDestroyFramebuffer(device, m_Framebuffers[i], nullptr);

		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			vkFreeCommandBuffers(device, m_FrameData[i].CommandPool, 1, &m_FrameData[i].CommandBuffer);
			vkDestroyCommandPool(device, m_FrameData[i].CommandPool, nullptr);
		}

		if (m_DescriptorPool)
		{
			vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
			ImGui_ImplVulkan_Shutdown();
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

	void VulkanImGui::BeginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex, VkExtent2D swapchainExtent)
	{
		m_CurrentActiveCmd = cmd;

		VkCommandBufferBeginInfo cmdBegin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		cmdBegin.pNext = nullptr;
		cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		std::array<VkClearValue, 2> clearColor = {};
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext = nullptr;
		info.renderPass = m_RenderPass;
		info.framebuffer = m_Framebuffers[imageIndex];
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = swapchainExtent;
		info.clearValueCount = static_cast<uint32_t>(clearColor.size());
		info.pClearValues = clearColor.data();

		vkCmdBeginRenderPass(m_CurrentActiveCmd, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanImGui::RenderDrawData()
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CurrentActiveCmd);
	}

	void VulkanImGui::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CurrentActiveCmd);
	}	

	VkDescriptorSet VulkanImGui::GetImGUIDescriptorSet(VkImageView view, VkSampler sampler, VkImageLayout layout)
	{
		return ImGui_VulkanImpl_AddTexture(sampler, view, layout);
	}

	void VulkanImGui::FreeImGuiDescriptorSet(VkDescriptorSet set)
	{
		ImGui_VulkanImpl_FreeTexture(set);
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
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		if (m_RenderPass)
			vkDestroyRenderPass(device, m_RenderPass, nullptr);

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		PX_CORE_VK_ASSERT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass), VK_SUCCESS, "Failed to create renderpass!");
	}
	void VulkanImGui::InitCommandBuffers()
	{
		VkCommandPoolCreateInfo poolci{};
		poolci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolci.pNext = nullptr;
		poolci.queueFamilyIndex = VulkanContext::GetDevice()->GetQueueFamilies().GraphicsFamilyIndex;
		poolci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkCommandBufferAllocateInfo bufferci{};
		bufferci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		bufferci.commandBufferCount = 1;

		for (uint8_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			PX_CORE_VK_ASSERT(vkCreateCommandPool(VulkanContext::GetDevice()->GetVulkanDevice(), &poolci, nullptr, &m_FrameData[i].CommandPool), VK_SUCCESS, "Failed to create graphics command pool!");
			bufferci.commandPool = m_FrameData[i].CommandPool;
			PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(VulkanContext::GetDevice()->GetVulkanDevice(), &bufferci, &m_FrameData[i].CommandBuffer), VK_SUCCESS, "Failed to create CommandBuffer!");
		}
	}
	void VulkanImGui::InitFrameBuffers(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		for (auto& fb : m_Framebuffers)
		{
			vkDestroyFramebuffer(device, fb, nullptr);
		}
		m_Framebuffers.clear();
		m_Framebuffers.resize(swapchainViews.size());

		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.pNext = nullptr;
		info.width = width;
		info.height = height;
		info.renderPass = m_RenderPass;
		info.layers = 1;

		for (size_t i = 0; i < m_Framebuffers.size(); i++)
		{
			info.attachmentCount = 1;
			info.pAttachments = &swapchainViews[i];
			PX_CORE_VK_ASSERT(vkCreateFramebuffer(device, &info, nullptr, &m_Framebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}
	}

}
