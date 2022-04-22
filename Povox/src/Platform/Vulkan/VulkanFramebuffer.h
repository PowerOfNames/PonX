#pragma once
#include "VulkanUtility.h"
#include "VulkanInitializers.h"
#include "VulkanImage2D.h"
#include "VulkanRenderPass.h"

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
			return std::find(formats.begin(), formats.end(), Image->GetSpecification().Format) != std::end(formats);
		}

		bool HasStencil()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT
			};
			return std::find(formats.begin(), formats.end(), Image->GetSpecification().Format) != std::end(formats);
		}

		bool HasDepthStencil()
		{
			return (HasDepth() || HasStencil());
		}
	};	

	class VulkanFramebuffer;
	class VulkanFramebufferPool
	{
	public:
		VulkanFramebufferPool();
		~VulkanFramebufferPool();

		void Clear();
		void Resize(uint32_t width, uint32_t hight);

		void AddFramebuffer(const std::string& name, VulkanFramebuffer* framebuffer);
		VulkanFramebuffer* GetFramebuffer(const std::string& name);
		VulkanFramebuffer* RemoveFramebuffer(const std::string& name);
		bool HasFramebuffer(const std::string& name);
	private:
		VulkanFramebuffer* m_CurrentRenderTarget = nullptr;
		std::unordered_map<const std::string, VulkanFramebuffer*> m_Framebuffers;
	};

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer();
		VulkanFramebuffer(const FramebufferSpecification& specs);
		virtual ~VulkanFramebuffer();
				
		virtual void Resize(uint32_t width = 0, uint32_t height = 0) override;
		void Destroy();
		
		virtual void Bind() const override {};
		virtual void Unbind() const override {};


		virtual void ClearColorAttachment(uint32_t attachmentIndex, int value) override {};

		virtual int ReadPixel(uint32_t attachmentIndex, int posX, int posY) override { return 0; };

		virtual inline const FramebufferSpecification& GetSpecification() const override { return m_Specification; };

		Ref<Image2D> GetColorImage(size_t index = 0);
		inline std::vector<Ref<Image2D>>& GetColorImages() { return m_ColorAttachments; }
		inline Ref<Image2D> GetDepthImage() { return m_DepthAttachment; };
		inline uint32_t GetWidth() { return m_Specification.Width; }
		inline uint32_t GetHeight() { return m_Specification.Height; }

		inline bool Resizable() const { return m_Specification.Resizable; }

		inline VkFramebuffer GetFramebuffer() { return m_Framebuffer; }
		inline VkRenderPass GetRenderPass() { return m_RenderPass; }
	private:
		void CreateAttachments();
	private:
		uint32_t m_ID;
		FramebufferSpecification m_Specification;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

		std::vector<Ref<Image2D>> m_ColorAttachments;
		Ref<Image2D> m_DepthAttachment = nullptr;
	};
}
