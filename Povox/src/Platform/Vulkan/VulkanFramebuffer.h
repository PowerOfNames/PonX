#pragma once
#include "VulkanUtilities.h"
#include "VulkanInitializers.h"
#include "VulkanImage2D.h"

#include "Povox/Renderer/Framebuffer.h"

namespace Povox {

	struct FramebufferAttachment
	{
		Ref<VulkanImage2D> Image = nullptr;

		VkAttachmentDescription Description{};

		bool HasDepth()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT
			};
			return std::find(formats.begin(), formats.end(), VulkanUtils::GetVulkanImageFormat(Image->GetSpecification().Format)) != std::end(formats);
		}

		bool HasStencil()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT
			};
			return std::find(formats.begin(), formats.end(), VulkanUtils::GetVulkanImageFormat(Image->GetSpecification().Format)) != std::end(formats);
		}

		bool HasDepthStencil()
		{
			return (HasDepth() || HasStencil());
		}
	};

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer();
		VulkanFramebuffer(const FramebufferSpecification& specs);
		virtual ~VulkanFramebuffer();
				
		virtual void Resize(uint32_t width = 0, uint32_t height = 0) override;
		void Destroy();
		
		//move to image/texture/buffer
		virtual int ReadPixel(uint32_t attachmentIndex, int posX, int posY) override { return 0; };

		virtual inline const FramebufferSpecification& GetSpecification() const override { return m_Specification; };

		virtual inline std::vector<Ref<Image2D>>& GetColorAttachments() override { return m_ColorAttachments; }
		virtual const Ref<Image2D> GetColorAttachment(size_t index = 0) override;
		virtual inline const Ref<Image2D> GetDepthAttachment() override { return m_DepthAttachment; };
		
		inline uint32_t GetWidth() { return m_Specification.Width; }
		inline uint32_t GetHeight() { return m_Specification.Height; }

		inline bool Resizable() const { return m_Specification.Resizable; }

		inline VkFramebuffer GetVulkanObj() { return m_Framebuffer; }
		inline VkRenderPass GetRenderPass() { return m_RenderPass; }
		inline VkRenderPass SetRenderPass(VkRenderPass renderPass) { m_RenderPass = renderPass; Construct(); }
		
		//helper for renderpass to actually create the framebuffer
		void Construct(VkRenderPass renderpass = VK_NULL_HANDLE);
	private:
		void CreateAttachments();
	private:
		FramebufferSpecification m_Specification;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

		std::vector<Ref<Image2D>> m_ColorAttachments;
		Ref<Image2D> m_DepthAttachment = nullptr;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}
