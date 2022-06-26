#include "pxpch.h"
#include "VulkanImage2D.h"

#include "VulkanDebug.h"
#include "VulkanInitializers.h"

#include "VulkanContext.h"

#include "VulkanBuffer.h"
#include "VulkanCommands.h"

#include <stb_image.h>


namespace Povox {

	AllocatedImage VulkanImageDepr::Create(VkImageCreateInfo imageInfo, VmaMemoryUsage memoryUsage)
	{
		AllocatedImage newImage;

		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memoryUsage;

		PX_CORE_VK_ASSERT(vmaCreateImage(s_Core->Allocator, &imageInfo, &allocationInfo, &newImage.Image, &newImage.Allocation, nullptr), VK_SUCCESS, "Failed to create Image!");

		return newImage;
	}

	AllocatedImage VulkanImageDepr::LoadFromFile(UploadContext& uploadContext, const char* path, VkFormat format)
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
		PX_CORE_ASSERT(pixels, "Failed to load texture!");
		VkDeviceSize imageSize = width * height * 4;

		PX_CORE_TRACE("Image width : '{0}', height '{1}'", width, height);

		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		void* data;
		vmaMapMemory(core.Allocator, stagingBuffer.Allocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(core.Allocator, stagingBuffer.Allocation);

		stbi_image_free(pixels);

		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(width);
		extent.height = static_cast<uint32_t>(height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);

		AllocatedImage output;
		output = VulkanImageDepr::Create(imageInfo);

		VulkanCommands::ImmidiateSubmitGfx(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = output.Image;
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
		VulkanCommands::ImmidiateSubmitTrsf(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferImageHeight = 0;
				region.bufferRowLength = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

				vkCmdCopyBufferToImage(cmd, stagingBuffer.Buffer, output.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});
		VulkanCommands::ImmidiateSubmitGfx(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = output.Image;
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
		
		vmaDestroyBuffer(core.Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		return output;
	}

	VulkanImage2D::VulkanImage2D(const ImageSpecification& spec)
	{
		VkExtent3D extent;
		extent.width = spec.Width;
		extent.height = spec.Height;
		extent.depth = 1;

		VkImageUsageFlags usages = VulkanUtils::GetVulkanImageUsages(spec.Usages);

		VkImageCreateInfo imageci = VulkanInits::CreateImageInfo(VulkanUtils::GetVulkanImageFormat(spec.Format), VulkanUtils::PovoxTilingToVk(spec.Tiling), VulkanUtils::GetVulkanImageUsages(spec.Usages), extent);
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VulkanUtils::PovoxMemoryUsageToVMA(spec.Memory);
		if (allocationInfo.usage == VMA_MEMORY_USAGE_GPU_ONLY)
			allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//PX_CORE_VK_ASSERT(vmaCreateImage(s_Core->Allocator, &imageci, &allocationInfo, &m_Allocation.Image2D, &m_Allocation.Allocation, nullptr), VK_SUCCESS, "Failed to create Image!");
		CreateImageView();
	}

	VulkanImage2D::~VulkanImage2D()
	{
		//vmaDestroyImage(m_CoreObjects.Allocator, m_Allocation.Image2D, m_Allocation.Allocation);
	}

	void VulkanImage2D::Destroy()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		if(m_Sampler)
			vkDestroySampler(device, m_Sampler, nullptr);
		if(m_View)
			vkDestroyImageView(device, m_View, nullptr);
		if (m_Allocation.Image)
			vmaDestroyImage(VulkanContext::GetAllocator(), m_Allocation.Image, m_Allocation.Allocation);
	}

	void VulkanImage2D::CreateImageView()
	{
		VkImageAspectFlags mask = 0;
		for (auto& usage : m_Specification.Usages.Usages)
		{
			switch (usage)
			{
			case ImageUsage::COLOR_ATTACHMENT: mask |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; break;
			case ImageUsage::DEPTH_ATTACHMENT: mask |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; break;
			case ImageUsage::INPUT_ATTACHMENT: mask |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; break;
			case ImageUsage::STORAGE: mask |= VK_IMAGE_USAGE_STORAGE_BIT; break;
			case ImageUsage::SAMPLED: mask |= VK_IMAGE_USAGE_SAMPLED_BIT; break;
			default: PX_CORE_ASSERT(true, "Usage not covered");
			}
		}
		if (m_Specification.CopySrc)
			mask |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (m_Specification.CopyDst)
			mask |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(VulkanUtils::GetVulkanImageFormat(m_Specification.Format), m_Allocation.Image, mask);
		PX_CORE_VK_ASSERT(vkCreateImageView(VulkanContext::GetDevice()->GetVulkanDevice(), &imageInfo, nullptr, &m_View), VK_SUCCESS, "Failed to create image view!");
	}

	void VulkanImage2D::CreateSampler()
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

		PX_CORE_VK_ASSERT(vkCreateSampler(VulkanContext::GetDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_Sampler), VK_SUCCESS, "Failed to create texture sampler!");
	}

}
