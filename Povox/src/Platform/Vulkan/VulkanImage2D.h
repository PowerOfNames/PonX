#pragma once
#include "Povox/Renderer/Image2D.h"



#include "VulkanUtility.h"




namespace Povox {

	namespace VulkanUtils {
		static VkFormat GetVulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RED_INTEGER: return VK_FORMAT_R16_UINT;
			}
			PX_CORE_ASSERT(true, "ImageFormat not covered!");
		}

		static VkImageUsageFlags GetVulkanImageUsages(ImageUsages usages)
		{
			VkImageUsageFlags out;
			for (auto& usage : usages.Usages)
			{
				out |= GetVulkanImageUsage(usage);
			}
			return out;
		}
		static VkImageUsageFlagBits GetVulkanImageUsage(ImageUsage usage)
		{
			switch (usage)
			{
			case ImageUsage::COLOR_ATTACHMENT: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			case ImageUsage::DEPTH_ATTACHMENT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			case ImageUsage::INPUT_ATTACHMENT: return VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

			case ImageUsage::STORAGE: return VK_IMAGE_USAGE_STORAGE_BIT;
			case ImageUsage::SAMPLED: return VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			PX_CORE_ASSERT(true, "ImageUsage not covered!");
			return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
		}

		static VkImageTiling GetVulkanTiling(ImageTiling tiling)
		{
			switch (tiling)
			{
			case ImageTiling::OPTIMAL: return VK_IMAGE_TILING_OPTIMAL;
			case ImageTiling::LINEAR: return VK_IMAGE_TILING_LINEAR;
			}
		}

		static VmaMemoryUsage GetVmaUsage(MemoryUsage usage)
		{
			switch (usage)
			{
			case MemoryUsage::Unknown: return VMA_MEMORY_USAGE_UNKNOWN;
			case MemoryUsage::GPU_ONLY: return VMA_MEMORY_USAGE_GPU_ONLY;
			case MemoryUsage::CPU_ONLY: return VMA_MEMORY_USAGE_CPU_ONLY;
			case MemoryUsage::UPLOAD: return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case MemoryUsage::DOWNLOAD: return VMA_MEMORY_USAGE_GPU_TO_CPU;
			case MemoryUsage::CPU_COPY: return VMA_MEMORY_USAGE_CPU_COPY;
			}
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
		}
	}

	struct AllocatedImage
	{
		VkImage Image;
		VmaAllocation Allocation;
	};

	class VulkanImageDepr
	{
	public:
		static AllocatedImage LoadFromFile(UploadContext& uploadContext, const char* path, VkFormat format);
		static AllocatedImage Create(VkImageCreateInfo imageInfo, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY);

	};

	class VulkanImage2D : public Image2D
	{
	public:
		VulkanImage2D(const ImageSpecification& spec);
		virtual ~VulkanImage2D();

		virtual void Destroy() override;

		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }

		inline VkImageView GetImageView() const { return m_View; }

	private: 
		void CreateImageView();
		void CreateSampler();
	private:
		ImageSpecification m_Specification;

		AllocatedImage m_Allocation{};
		VkImageView m_View = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};
}
