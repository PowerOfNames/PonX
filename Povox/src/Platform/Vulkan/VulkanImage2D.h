#pragma once
#include "Povox/Renderer/Image2D.h"
#include "Povox/Renderer/Texture.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanUtilities.h"


namespace Povox {

	namespace VulkanUtils {
		static VkImageAspectFlags GetAspectFlagsFromUsages(ImageUsages usages)
		{
			VkImageAspectFlags mask = 0;
			for (auto& usage : usages.Usages)
			{
				switch (usage)
				{
				case ImageUsage::COLOR_ATTACHMENT:
				case ImageUsage::SAMPLED:			mask |= VK_IMAGE_ASPECT_COLOR_BIT; break;
				case ImageUsage::DEPTH_ATTACHMENT:	mask |= VK_IMAGE_ASPECT_DEPTH_BIT; break;
				default: PX_CORE_ASSERT(true, "Usage not covered");
				}
			}
			return mask;
		}

		static VkFormat GetVulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
				case ImageFormat::RGB8: return VK_FORMAT_R8G8B8_SRGB;
				case ImageFormat::RG8: return VK_FORMAT_R8G8_SRGB;
				case ImageFormat::RED_INTEGER_U32: return VK_FORMAT_R32_UINT;
				case ImageFormat::RED_INTEGER_U64: return VK_FORMAT_R64_UINT;
				case ImageFormat::RED_FLOAT: return VK_FORMAT_R32_SFLOAT;
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

		static VkImageLayout GetImageLayout(ImageUsages usages)
		{
			if (usages.ContainsUsage(ImageUsage::SAMPLED))
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			return VK_IMAGE_LAYOUT_UNDEFINED;
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
		VulkanImage2D(uint32_t width, uint32_t height, uint32_t channels = 4);
		VulkanImage2D(const ImageSpecification& spec);
		virtual ~VulkanImage2D() = default;
		virtual void Free() override;

		virtual uint64_t GetRendererID() const override { return m_RID; }

		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		virtual void SetData(void* data) override;

		virtual void* GetDescriptorSet() override { return m_DescriptorSet; }
		void SetDescriptorSet(VkDescriptorSet set) { m_DescriptorSet = set; }

		virtual int ReadPixel(int posX, int posY) override;

		static AllocatedImage CreateAllocation(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, QueueFamilyOwnership ownership = QueueFamilyOwnership::QFO_UNDEFINED, std::string debugName = std::string());

		void TransitionImageLayout(
			VkImageLayout initialLayout, VkImageLayout finalLayout,
			VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcMask,
			VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstMask,
			QueueFamilyOwnership targetOwner = QueueFamilyOwnership::QFO_UNDEFINED);

		inline VkImageView GetImageView() const { return m_View; }
		inline VkSampler GetSampler() const { return m_Sampler; }
		inline VkImage GetImage() { return m_Allocation.Image; }
		inline VkDescriptorImageInfo& GetImageInfo() { return m_DescriptorInfo; };
		

		void CreateDescriptorSet();
		void CreateSampler();

		virtual const std::string& GetDebugName() const { return m_Specification.DebugName; }

	private:
		void CreateImage();
		void CreateImageView();
		VkDescriptorImageInfo CreateDescriptorInfo();

	private:
		RendererUID m_RID;

		ImageSpecification m_Specification;

		AllocatedImage m_Allocation{};
		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageView m_View = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		bool m_OwnsSampler = false;
		VkWriteDescriptorSet m_Write{};

		VkDescriptorImageInfo m_DescriptorInfo{};

		VkDescriptorSetLayoutBinding m_Binding{};
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;

		QueueFamilyOwnership m_Ownership = QueueFamilyOwnership::QFO_UNDEFINED;
	};



	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(uint32_t width, uint32_t height, uint32_t channels = 4, const std::string& debugName = "DebugName");
		VulkanTexture2D(const std::string& path, const std::string& debugName = "DebugName");
		virtual ~VulkanTexture2D() = default;
		virtual void Free() override;

		virtual inline uint32_t GetWidth() const override { return m_Image->GetSpecification().Width; };
		virtual inline uint32_t GetHeight() const override { return m_Image->GetSpecification().Height; };

		virtual void SetData(void* data) override;

		virtual const Ref<Image2D> GetImage() const override { return m_Image; }
		virtual Ref<Image2D> GetImage() override { return m_Image; }

		virtual uint64_t GetRendererID() const override { return m_RUID; }
		virtual inline const std::string& GetDebugName() const override { return m_Image->GetDebugName(); }

		virtual bool operator==(const Texture& other) const override { return m_RUID == ((VulkanTexture2D&)other).m_RUID; }

	private:
		RendererUID m_RUID;

		Ref<VulkanImage2D> m_Image = nullptr;

		std::string m_Path = "";
	};
}
