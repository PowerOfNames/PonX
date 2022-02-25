#pragma once
#include "VulkanUtility.h"
#include "VulkanInitializers.h"
#include "VulkanImage.h"

namespace Povox {

	namespace VulkanUtils {

		static bool IsDepthAttachment(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_D32_SFLOAT: return true;
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return true;
				case VK_FORMAT_D24_UNORM_S8_UINT: return true;
				default: return false;
			}
		}

	}

	struct FramebufferAttachmentCreateInfo
	{
		uint32_t Width = 0, Height = 0;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags ImageUsage;
		VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	};

	struct FramebufferAttachment
	{
		uint32_t Width, Height = 0;
		AllocatedImage Image = {};
		VkImageView ImageView = VK_NULL_HANDLE;
		VkFormat Format = VK_FORMAT_UNDEFINED;

		VkAttachmentDescription Description{};

		bool HasDepth()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT
			};
			return std::find(formats.begin(), formats.end(), Format) != std::end(formats);
		}

		bool HasStencil()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT
			};
			return std::find(formats.begin(), formats.end(), Format) != std::end(formats);
		}

		bool HasDepthStencil()
		{
			return (HasDepth() || HasStencil());
		}
	};

	class VulkanFramebuffer
	{
	public:
		VulkanFramebuffer();
		VulkanFramebuffer(VulkanCoreObjects* coreObjects, uint32_t width = 0, uint32_t height = 0);
		~VulkanFramebuffer() = default;

		void Create(VkRenderPass renderPass);
		void Destroy();

		void Resize(VkRenderPass renderPass, uint32_t width, uint32_t height);
		void AddAttachment(FramebufferAttachmentCreateInfo& attachmentCreateInfo);
		VkRenderPass CreateDefaultRenderpass();

		FramebufferAttachment& GetColorAttachment(size_t attachmentIndex);
		
		inline VkFramebuffer Get() { return m_Framebuffer; }
		inline uint32_t GetWidth() { return m_Width; }
		inline uint32_t GetHeight() { return m_Height; }
		inline FramebufferAttachment& GetDepthAttachment() { return m_DepthStencilAttachment; }


	private:
		void CreateAttachment(uint32_t index);
	private:
		VulkanCoreObjects* m_Core = nullptr;
		uint32_t m_Width = 0, m_Height = 0;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
		std::vector<FramebufferAttachmentCreateInfo> m_CreateInfos = {};
		std::vector<FramebufferAttachment> m_ColorAttachments = {};
		FramebufferAttachment m_DepthStencilAttachment = {};
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		bool m_HasDepthStencil = false;
		bool m_Alive = false;
	};

	class VulkanRenderPassBuilder
	{
	public: 
		static VulkanRenderPassBuilder Begin(VulkanCoreObjects* core);

		VulkanRenderPassBuilder& AddAttachment(VkAttachmentDescription attachment);
		VulkanRenderPassBuilder& CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkImageLayout>& colorLayouts, VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED);
		VulkanRenderPassBuilder& AddDependency(VkSubpassDependency dependency);

		VkRenderPass Build();

	private:
		VulkanCoreObjects* m_Core;
		std::vector<VkAttachmentDescription> m_Attachments;
		std::vector<VkSubpassDescription> m_Subpasses;
		std::vector<VkSubpassDependency> m_Dependencies;
	};


	class VulkanRenderPass
	{
	public:
		struct Attachment
		{
			VkAttachmentDescription Description{};
			VkAttachmentReference Reference{};
		};

		static VkRenderPass CreateColorAndDepth(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, const std::vector<VkSubpassDependency>& dependencies);
		static VkRenderPass CreateColor(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, const std::vector<VkSubpassDependency>& dependencies);
	};
}
