#pragma once
#include "Povox/Renderer/Image2D.h"
#include "Povox/Renderer/Texture.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanUtilities.h"


namespace Povox {

	namespace VulkanUtils {
		static VkFormat GetVulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
				case ImageFormat::RED_INTEGER: return VK_FORMAT_R32_SINT;
				case ImageFormat::DEPTH24STENCIL8: return VulkanUtils::FindDepthFormat(VulkanContext::GetDevice()->GetPhysicalDevice());
			}
			PX_CORE_ASSERT(true, "ImageFormat not covered!");
		}

		static VkImageUsageFlagBits GetVulkanImageUsage(ImageUsage usage)
		{
			switch (usage)
			{
				case ImageUsage::COLOR_ATTACHMENT: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				case ImageUsage::DEPTH_ATTACHMENT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				case ImageUsage::INPUT_ATTACHMENT: return VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

				case ImageUsage::COPY_SRC: return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				case ImageUsage::COPY_DST: return VK_IMAGE_USAGE_TRANSFER_DST_BIT;

				case ImageUsage::STORAGE: return VK_IMAGE_USAGE_STORAGE_BIT;
				case ImageUsage::SAMPLED: return VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			PX_CORE_ASSERT(true, "ImageUsage not covered!");
		}

		static VkImageUsageFlags GetVulkanImageUsages(ImageUsages usages)
		{
			VkImageUsageFlags out = 0;
			for (auto& usage : usages.Usages)
			{
				out |= GetVulkanImageUsage(usage);
			}
			return out;
		}

		static VkImageTiling GetVulkanTiling(ImageTiling tiling)
		{
			switch (tiling)
			{
			case ImageTiling::OPTIMAL: return VK_IMAGE_TILING_OPTIMAL;
			case ImageTiling::LINEAR: return VK_IMAGE_TILING_LINEAR;
			}
			PX_CORE_ASSERT(true, "ImageTiling not covered!");
		}

		static bool IsVulkanDepthFormat(VkFormat format)
		{
			switch (format)
			{
			case VK_FORMAT_D32_SFLOAT: return true;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return true;
			case VK_FORMAT_D24_UNORM_S8_UINT: return true;
			default: return false;
			}
			PX_CORE_ASSERT(true, "Not a viable DepthFormat!");
		}
	}

	struct AllocatedImage
	{
		VkImage Image;
		VmaAllocation Allocation;
	};

	class VulkanImage2D : public Image2D
	{
	public:
		VulkanImage2D(uint32_t width, uint32_t height);
		VulkanImage2D(const ImageSpecification& spec);
		VulkanImage2D(const char* path, VkFormat format);
		virtual ~VulkanImage2D() override;

		virtual void Destroy() override;

		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		virtual void* GetDescriptorSet() const override { return (void*)m_DescriptorSet; }
		virtual void SetData(void* data, size_t size) override;

		static AllocatedImage CreateAllocation(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);

		inline VkImageView GetImageView() const { return m_View; }
		inline VkImage GetImage() { return m_Allocation.Image; }

	private: 
		void CreateImageView();
		void CreateDescriptorSet();
		void CreateSampler();
	private:
		ImageSpecification m_Specification;

		AllocatedImage m_Allocation{};
		VkImageView m_View = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};

	class VulkanTexture : public Texture
	{
		VulkanTexture();
		~VulkanTexture() = default;


	private:
		Ref<VulkanImage2D> m_Image;

	};
}
